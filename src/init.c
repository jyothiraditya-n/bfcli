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

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include <LC_args.h>
#include <LC_editor.h>
#include <LC_vars.h>

#include "../inc/file.h"
#include "../inc/init.h"
#include "../inc/main.h"
#include "../inc/print.h"
#include "../inc/run.h"
#include "../inc/signal.h"
#include "../inc/size.h"
#include "../inc/trans.h"

static void about();
static void help();
static void version();

static void about() {
	putchar('\n');
	print_about();
	putchar('\n');
	exit(0);
}

void gethw() {
	if(no_ansi) return;

	raw.c_lflag &= ~ECHO;
	int ret = tcsetattr(STDIN_FILENO, TCSANOW, &raw);
	if(ret == -1) print_error(UNKNOWN_ERROR);
	
	printf("\e[s\e[999;999H\e[6n\e[u");
	while(getchar() != '\e');

	ret = scanf("[%zu;%zuR", &height, &width);
	if(ret != 2) print_error(UNKNOWN_ERROR);

	raw.c_lflag |= ECHO;
	ret = tcsetattr(STDIN_FILENO, TCSANOW, &cooked);
	if(ret == -1) print_error(UNKNOWN_ERROR);

	if(minimal && width > 80) width = 80;
}

static void help() {
	putchar('\n');
	print_help();
	putchar('\n');
	print_usage();
	putchar('\n');
	exit(0);
}

void init(int argc, char **argv) {
	progname = argv[0];
	signal(SIGINT, on_interrupt);

	LCa_t *arg = LCa_new();
	if(!arg) print_error(UNKNOWN_ERROR);
	arg -> long_flag = "about";
	arg -> short_flag = 'a';
	arg -> pre = about;

	arg = LCa_new();
	if(!arg) print_error(UNKNOWN_ERROR);
	arg -> long_flag = "help";
	arg -> short_flag = 'h';
	arg -> pre = help;

	arg = LCa_new();
	if(!arg) print_error(UNKNOWN_ERROR);
	arg -> long_flag = "version";
	arg -> short_flag = 'v';
	arg -> pre = version;

	LCv_t *var = LCv_new();
	if(!var) print_error(UNKNOWN_ERROR);
	var -> id = "colour";
	var -> data = &colour;

	arg = LCa_new();
	if(!arg) print_error(UNKNOWN_ERROR);
	arg -> long_flag = "colour";
	arg -> short_flag = 'c';
	arg -> var = var;
	arg -> value = true;

	arg = LCa_new();
	if(!arg) print_error(UNKNOWN_ERROR);
	arg -> long_flag = "monochrome";
	arg -> short_flag = 'm';
	arg -> var = var;
	arg -> value = false;

	var = LCv_new();
	if(!var) print_error(UNKNOWN_ERROR);
	var -> id = "no-ansi";
	var -> data = &no_ansi;

	arg = LCa_new();
	if(!arg) print_error(UNKNOWN_ERROR);
	arg -> long_flag = "no-ansi";
	arg -> short_flag = 'n';
	arg -> var = var;
	arg -> value = true;

	var = LCv_new();
	if(!var) print_error(UNKNOWN_ERROR);
	var -> id = "minimal";
	var -> data = &minimal;

	arg = LCa_new();
	if(!arg) print_error(UNKNOWN_ERROR);
	arg -> long_flag = "minimal";
	arg -> short_flag = '0';
	arg -> var = var;
	arg -> value = true;

	var = LCv_new();
	if(!var) print_error(UNKNOWN_ERROR);
	var -> id = "filename";
	var -> fmt = FILENAME_SCN;
	var -> data = filename;

	arg = LCa_new();
	if(!arg) print_error(UNKNOWN_ERROR);
	arg -> long_flag = "file";
	arg -> short_flag = 'f';
	arg -> var = var;

	var = LCv_new();
	if(!var) print_error(UNKNOWN_ERROR);
	var -> id = "length";
	var -> fmt = "%zu";
	var -> data = &code_size;

	arg = LCa_new();
	if(!arg) print_error(UNKNOWN_ERROR);
	arg -> long_flag = "length";
	arg -> short_flag = 'l';
	arg -> var = var;

	var = LCv_new();
	if(!var) print_error(UNKNOWN_ERROR);
	var -> id = "transpile";
	var -> data = &transpile;

	arg = LCa_new();
	if(!arg) print_error(UNKNOWN_ERROR);
	arg -> long_flag = "transpile";
	arg -> short_flag = 't';
	arg -> var = var;
	arg -> value = true;

	var = LCv_new();
	if(!var) print_error(UNKNOWN_ERROR);
	var -> id = "output-filename";
	var -> fmt = FILENAME_SCN;
	var -> data = outname;

	arg = LCa_new();
	if(!arg) print_error(UNKNOWN_ERROR);
	arg -> long_flag = "output";
	arg -> short_flag = 'o';
	arg -> var = var;

	var = LCv_new();
	if(!var) print_error(UNKNOWN_ERROR);
	var -> id = "safe-code";
	var -> data = &safe_code;

	arg = LCa_new();
	if(!arg) print_error(UNKNOWN_ERROR);
	arg -> long_flag = "safe-code";
	arg -> short_flag = 's';
	arg -> var = var;
	arg -> value = true;
	arg -> var = var;

	var = LCv_new();
	if(!var) print_error(UNKNOWN_ERROR);
	var -> id = "assembly";
	var -> data = &assembly;

	arg = LCa_new();
	if(!arg) print_error(UNKNOWN_ERROR);
	arg -> long_flag = "assembly";
	arg -> short_flag = 'x';
	arg -> var = var;
	arg -> value = true;

	var = LCv_new();
	if(!var) print_error(UNKNOWN_ERROR);
	var -> id = "bytes";
	var -> fmt = "%zu";
	var -> data = &bytes;

	arg = LCa_new();
	if(!arg) print_error(UNKNOWN_ERROR);
	arg -> long_flag = "bytes";
	arg -> short_flag = 'b';
	arg -> var = var;

	var = LCv_new();
	if(!var) print_error(UNKNOWN_ERROR);
	var -> id = "direct_inp";
	var -> data = &direct_inp;

	arg = LCa_new();
	if(!arg) print_error(UNKNOWN_ERROR);
	arg -> long_flag = "direct-inp";
	arg -> short_flag = 'd';
	arg -> var = var;
	arg -> value = true;

	LCa_noflags = &imm_fname;
	LCa_max_noflags = 1;

	int ret = LCa_read(argc, argv);
	if(ret != LCA_OK) print_minihelp();

	init_files();
	if(no_ansi) colour = false;

	if(!code) {
		code = calloc(code_size, sizeof(char));
		if(!code) print_error(UNKNOWN_ERROR);
	}

	LCe_banner = "Bfcli: The Interactive Brainfuck "
		"Command-Line Interpreter";

	LCe_buffer = code;
	LCe_length = code_size;
}

void print_minihelp() {
	printf("\n Usage: %s [ARGS] [FILE]\n\n", progname);
	
	puts("  Valid arguments are:\n");

	puts("    -a, --about      | -h, --help       | -v, --version");
	puts("    -c, --colour     | -m, --monochrome | -n, --no-ansi");
	puts("    -0, --minimal    |\n");

	puts("    -f, --file FILE  | -l, --length LEN | -o, --output OUT");
	puts("    -b, --bytes SIZE | -s, --safe-code  | -t, --transpile");
	puts("    -x, --assembly   | -d, --direct-inp |\n");

	puts("  Happy coding! :)\n");
	exit(BAD_ARGS);
}

static void version() {
	printf("Bfcli Version %d.%d: %s\n", VERSION, SUBVERSION, VERNAME);
	exit(0);
}