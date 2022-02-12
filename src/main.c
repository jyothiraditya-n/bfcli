/* Bfcli: The Interactive Brainfuck Command-Line Interpreter
 * Copyright (C) 2021 Jyothiraditya Nellakra
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <https://www.gnu.org/licenses/>. */

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <termios.h>
#include <unistd.h>
#include <signal.h>

#include <LC_args.h>
#include <LC_editor.h>
#include <LC_lines.h>

#include "arch.h"
#include "clidata.h"
#include "errors.h"
#include "files.h"
#include "interpreter.h"
#include "main.h"
#include "optims.h"
#include "printing.h"
#include "translator.h"

size_t BFm_insertion_point;

static void init(int argc, char **argv);
static int check(char *BFi_program_str, size_t len);

#define CODE_OK 1
#define CODE_INCOMPLETE 2
#define CODE_ERROR 3

static void handle_int();
static void handle_sig(int signum);

static void about();
static void help();
static void version();

int main(int argc, char **argv) {
	if(!strcmp(argv[0], "bfc")) BFt_translate = true;
	init(argc, argv);

	if(BFc_minimal_mode) {
		printf("Bfcli Version %d.%d: %s\n"
			"Memory Size: %zu Chars\n\n",
			BF_VERSION, BF_SUBVERSION, BF_VERNAME,
			BFi_mem_size);
	}

	else BFp_print_banner();

	static char line[BF_LINE_SIZE];
	static char old_file[BF_FILENAME_SIZE];

	while(true) {
		int ret = tcsetattr(STDIN_FILENO, TCSANOW, &BFc_cooked);
		if(ret == -1) BFe_report_err(BFE_UNKNOWN_ERROR);

		if(BFc_minimal_mode) printf("%% "); else BFp_print_prompt();
		LCl_buffer = line + BFm_insertion_point;
		LCl_length = BF_LINE_SIZE - BFm_insertion_point;

		if(BFc_no_ansi) ret = LCl_bread(line, BF_LINE_SIZE);
		else ret = LCl_read();

		switch(ret) {
		case LCL_OK: 	break;
		case LCL_CUT: 	BFe_report_err(BFE_LINE_TOO_LONG); continue;

		case LCL_INT:
			handle_int();
			BFm_insertion_point = 0;
			continue;

		case LCL_CUT_INT:
			BFm_insertion_point = 0;
			putchar('\n');
			BFe_report_err(BFE_LINE_TOO_LONG);
			continue;

		default:
			BFe_report_err(BFE_UNKNOWN_ERROR);
		}

		strcpy(old_file, BFf_mainfile_name);
		strcpy(BFf_mainfile_name, line);
		ret = BFf_load_file();

		if(ret == FILE_OK) {
			printf("loaded '%s'.\n", BFf_mainfile_name);
			Bfi_do_recompile = true;
			continue;
		}

		else strcpy(BFf_mainfile_name, old_file);
		
		size_t len = strlen(line);
		ret = check(line, len);

		switch(ret) {
		case CODE_OK:
			ret = tcsetattr(STDIN_FILENO, TCSANOW, &BFc_raw);
			if(ret == -1) BFe_report_err(BFE_UNKNOWN_ERROR);
			
			BFi_is_running = true;

			BFi_last_output = '\n';
			if(BFc_minimal_mode) BFi_main(line);
			else BFi_main(line);
			if(BFi_last_output != '\n') putchar('\n');

			BFm_insertion_point = 0;
			BFi_is_running = false;
			break;

		case CODE_INCOMPLETE:
			BFm_insertion_point = strlen(line);
			break;

		case CODE_ERROR:
			fprintf(stderr, "error: unmatched ']'\n");
			BFm_insertion_point = 0;
			break;
		}
	}

	exit(0);
}

static void init(int argc, char **argv) {
	BFc_cmd_name = argv[0];
	signal(SIGINT, handle_sig);

	LCa_t *arg = LCa_new();
	if(!arg) BFe_report_err(BFE_UNKNOWN_ERROR);
	arg -> long_flag = "about";
	arg -> short_flag = 'a';
	arg -> pre = about;

	arg = LCa_new();
	if(!arg) BFe_report_err(BFE_UNKNOWN_ERROR);
	arg -> long_flag = "help";
	arg -> short_flag = 'h';
	arg -> pre = help;

	arg = LCa_new();
	if(!arg) BFe_report_err(BFE_UNKNOWN_ERROR);
	arg -> long_flag = "version";
	arg -> short_flag = 'v';
	arg -> pre = version;

	LCv_t *var = LCv_new();
	if(!var) BFe_report_err(BFE_UNKNOWN_ERROR);
	var -> id = "colour";
	var -> data = &BFc_use_colour;

	arg = LCa_new();
	if(!arg) BFe_report_err(BFE_UNKNOWN_ERROR);
	arg -> long_flag = "colour";
	arg -> short_flag = 'c';
	arg -> var = var;
	arg -> value = true;

	var = LCv_new();
	if(!var) BFe_report_err(BFE_UNKNOWN_ERROR);
	var -> id = "no-ansi";
	var -> data = &BFc_no_ansi;

	arg = LCa_new();
	if(!arg) BFe_report_err(BFE_UNKNOWN_ERROR);
	arg -> long_flag = "no-ansi";
	arg -> short_flag = 'n';
	arg -> var = var;
	arg -> value = true;

	var = LCv_new();
	if(!var) BFe_report_err(BFE_UNKNOWN_ERROR);
	var -> id = "minimal";
	var -> data = &BFc_minimal_mode;

	arg = LCa_new();
	if(!arg) BFe_report_err(BFE_UNKNOWN_ERROR);
	arg -> long_flag = "minimal";
	arg -> short_flag = 'm';
	arg -> var = var;
	arg -> value = true;

	var = LCv_new();
	if(!var) BFe_report_err(BFE_UNKNOWN_ERROR);
	var -> id = "BFf_mainfile_name";
	var -> fmt = BF_FILENAME_SCN;
	var -> data = BFf_mainfile_name;

	arg = LCa_new();
	if(!arg) BFe_report_err(BFE_UNKNOWN_ERROR);
	arg -> long_flag = "file";
	arg -> short_flag = 'f';
	arg -> var = var;

	var = LCv_new();
	if(!var) BFe_report_err(BFE_UNKNOWN_ERROR);
	var -> id = "length";
	var -> fmt = "%zu";
	var -> data = &BFi_code_size;

	arg = LCa_new();
	if(!arg) BFe_report_err(BFE_UNKNOWN_ERROR);
	arg -> long_flag = "length";
	arg -> short_flag = 'l';
	arg -> var = var;

	var = LCv_new();
	if(!var) BFe_report_err(BFE_UNKNOWN_ERROR);
	var -> id = "translate";
	var -> data = &BFt_translate;

	arg = LCa_new();
	if(!arg) BFe_report_err(BFE_UNKNOWN_ERROR);
	arg -> long_flag = "translate";
	arg -> short_flag = 't';
	arg -> var = var;
	arg -> value = true;

	var = LCv_new();
	if(!var) BFe_report_err(BFE_UNKNOWN_ERROR);
	var -> id = "BFf_outfile_name";
	var -> fmt = BF_FILENAME_SCN;
	var -> data = BFf_outfile_name;

	arg = LCa_new();
	if(!arg) BFe_report_err(BFE_UNKNOWN_ERROR);
	arg -> long_flag = "output";
	arg -> short_flag = 'o';
	arg -> var = var;

	var = LCv_new();
	if(!var) BFe_report_err(BFE_UNKNOWN_ERROR);
	var -> id = "compile";
	var -> data = &BFt_compile;

	arg = LCa_new();
	if(!arg) BFe_report_err(BFE_UNKNOWN_ERROR);
	arg -> long_flag = "compile";
	arg -> short_flag = 'x';
	arg -> var = var;
	arg -> value = true;

	var = LCv_new();
	if(!var) BFe_report_err(BFE_UNKNOWN_ERROR);
	var -> id = "ram";
	var -> fmt = "%zu";
	var -> data = &BFi_mem_size;

	arg = LCa_new();
	if(!arg) BFe_report_err(BFE_UNKNOWN_ERROR);
	arg -> long_flag = "ram";
	arg -> short_flag = 'r';
	arg -> var = var;

	var = LCv_new();
	if(!var) BFe_report_err(BFE_UNKNOWN_ERROR);
	var -> id = "BFc_direct_inp";
	var -> data = &BFc_direct_inp;

	arg = LCa_new();
	if(!arg) BFe_report_err(BFE_UNKNOWN_ERROR);
	arg -> long_flag = "direct-inp";
	arg -> short_flag = 'd';
	arg -> var = var;
	arg -> value = true;

	var = LCv_new();
	if(!var) BFe_report_err(BFE_UNKNOWN_ERROR);
	var -> id = "BFo_level";
	var -> fmt = "%d";
	var -> data = &BFo_level;

	arg = LCa_new();
	if(!arg) BFe_report_err(BFE_UNKNOWN_ERROR);
	arg -> long_flag = "optim";
	arg -> short_flag = 'O';
	arg -> var = var;

	var = LCv_new();
	if(!var) BFe_report_err(BFE_UNKNOWN_ERROR);
	var -> id = "BFt_standalone";
	var -> data = &BFt_standalone;

	arg = LCa_new();
	if(!arg) BFe_report_err(BFE_UNKNOWN_ERROR);
	arg -> long_flag = "standalone";
	arg -> short_flag = 's';
	arg -> var = var;
	arg -> value = true;

	var = LCv_new();
	if(!var) BFe_report_err(BFE_UNKNOWN_ERROR);
	var -> id = "BFa_target_arch";
	var -> fmt = BF_FILENAME_SCN;
	var -> data = &BFa_target_arch;

	arg = LCa_new();
	if(!arg) BFe_report_err(BFE_UNKNOWN_ERROR);
	arg -> long_flag = "arch";
	arg -> short_flag = 'A';
	arg -> var = var;
	arg -> value = true;

	LCa_noflags = &BFc_immediate;
	LCa_max_noflags = 1;

	int ret = LCa_read(argc, argv);
	if(ret != LCA_OK) BFp_print_minihelp();

	if(BFt_standalone) BFt_compile = true;
	if(BFt_compile) BFt_translate = true;

	if(!strlen(BFa_target_arch)) {
		#if defined(__i386__)
		strcpy(BFa_target_arch, "i386");
		#elif defined(__amd64__)
		strcpy(BFa_target_arch, "amd64");
		#else
		strcpy(BFa_target_arch, "other");
		#endif
	}

	BFc_init(); BFi_init(); BFf_init();
	if(BFc_no_ansi) BFc_use_colour = false;

	if(!BFi_program_str) {
		BFi_program_str = calloc(BFi_code_size, sizeof(char));
		if(!BFi_program_str) BFe_report_err(BFE_UNKNOWN_ERROR);
	}

	LCe_banner = "Bfcli: The Interactive Brainfuck "
		"Command-Line Interpreter";

	LCe_buffer = BFi_program_str;
	LCe_length = BFi_code_size;
}

static int check(char *BFi_program_str, size_t len) {
	int loops_open = 0;
	bool wait = false;

	for(size_t i = 0; i < len; i++) {
		switch(BFi_program_str[i]) {
			case '[': loops_open++; break;
			case ']': loops_open--; break;
			case '!': wait = true; break;
			case ';': wait = false; break;
		}

		if(loops_open < 0) return CODE_ERROR;
	}

	if(wait || loops_open) return CODE_INCOMPLETE;
	return CODE_OK;
}

static void handle_int() {
	if(!BFm_insertion_point && LCe_dirty && !BFc_no_ansi) {
		printf("save unsaved changes? [Y/n]: ");

		char ans = LCl_readch();
		if(ans == LCLCH_ERR) BFe_report_err(BFE_UNKNOWN_ERROR);

		if(ans == 'Y' || ans == 'y' || ans == '\n')
			BFf_save_file(BFi_program_str,
				strlen(BFi_program_str));

		exit(0);
	}

	else if(!BFm_insertion_point && LCe_dirty && BFc_no_ansi) {
		puts("you have unsaved changes.");
		BFf_save_file(BFi_program_str, strlen(BFi_program_str));
		exit(0);
	}

	else if(!BFm_insertion_point) exit(0);
}

static void handle_sig(int signum) {
	signal(signum, handle_sig);
	LCl_sigint = true;
	LCe_sigint = true;
	BFi_is_running = false;
	BFi_last_output = 0;
}

static void about() {
	putchar('\n');
	BFp_print_about();
	putchar('\n');
	exit(0);
}

static void help() {
	putchar('\n');
	BFp_print_help();
	putchar('\n');
	BFp_print_usage();
	putchar('\n');
	exit(0);
}

static void version() {
	printf("Bfcli Version %d.%d: %s\n",
		BF_VERSION, BF_SUBVERSION, BF_VERNAME);
	exit(0);
}