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

#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <termios.h>
#include <unistd.h>

#include <LC_editor.h>
#include <LC_lines.h>

#include "clidata.h"
#include "errors.h"
#include "files.h"
#include "main.h"
#include "interpreter.h"
#include "printing.h"
#include "translator.h"

char BFf_mainfile_name[BF_FILENAME_SIZE];
char BFf_outfile_name[BF_FILENAME_SIZE];
char BFf_savefile_name[BF_LINE_SIZE];

static int check_file(size_t len);
static void get_file();

static int hex_digits();

void BFf_init() {
	if(strlen(BFf_mainfile_name) && BFc_immediate) {
		fprintf(stderr, "%s: error: file supplied both with and "
			"without -f.\n", BFc_cmd_name);

		BFe_report_err(BFE_BAD_ARGS);
	}

	if(BFc_immediate) {
		strncat(BFf_mainfile_name, BFc_immediate,
			BF_FILENAME_SIZE - 1);
		get_file();

		int ret = tcsetattr(STDIN_FILENO, TCSANOW, &BFc_raw);
		if(ret == -1) BFe_report_err(BFE_UNKNOWN_ERROR);

		BFi_is_running = true;
		BFi_last_output = '\n';
		BFi_exec();

		if(BFi_last_output != '\n') putchar('\n');

		ret = tcsetattr(STDIN_FILENO, TCSANOW, &BFc_cooked);
		if(ret == -1) BFe_report_err(BFE_UNKNOWN_ERROR);

		exit(0);
	}

	if(strlen(BFf_mainfile_name)) get_file();

	if(BFt_translate) {
		BFe_report_err(BFE_NO_FILE);
		exit(BFE_NO_FILE);
	}
}

int BFf_load_file() {
	FILE *file = fopen(BFf_mainfile_name, "r");
	if(!file) {
		BFe_file_name = BFf_mainfile_name;
		return BFE_FILE_UNREADABLE;
	}
	
	int ret = fseek(file, 0, SEEK_END);
	if(ret) BFe_report_err(BFE_UNKNOWN_ERROR);

	size_t size = (ftell(file) / sizeof(char)) + 1;
	rewind(file);

	size_t old_size = BF_CODE_SIZE;
	char *old_code = NULL;

	if(!BFi_program_str) {
		old_size = 0;
		BFi_code_size += size;
		BFi_program_str = calloc(BFi_code_size, sizeof(char));
		if(!BFi_program_str) BFe_report_err(BFE_UNKNOWN_ERROR);
	}

	else if(size > BFi_code_size) {
		old_size = BFi_code_size;

		BFi_code_size = size + BF_CODE_SIZE;
		LCe_length = BFi_code_size;

		old_code = BFi_program_str;

		BFi_program_str = calloc(BFi_code_size, sizeof(char));
		if(!BFi_program_str) BFe_report_err(BFE_UNKNOWN_ERROR);

		LCe_buffer = BFi_program_str;
	}

	else {
		old_size = strlen(BFi_program_str);

		old_code = calloc(old_size, sizeof(char));
		if(!old_code) BFe_report_err(BFE_UNKNOWN_ERROR);

		for(size_t i = 0; i <= old_size; i++) old_code[i] = BFi_program_str[i];
	}

	for(size_t i = 0; i < size; i++) BFi_program_str[i] = fgetc(file);
	BFi_program_str[size - 1] = 0;

	ret = fclose(file);
	if(ret == EOF) BFe_report_err(BFE_UNKNOWN_ERROR);

	ret = check_file(size - 1);
	if(ret == BFE_BAD_CODE) {
		BFi_code_size = old_size;
		LCe_length = BFi_code_size;

		for(size_t i = 0; i < old_size; i++)
			BFi_program_str[i] = old_code[i];

		if(old_size) BFi_program_str[old_size] = 0;

		free(old_code);
		return BFE_BAD_CODE;
	}

	if(LCe_dirty && !BFc_no_ansi) {
		printf("save unsaved changes? [Y/n]: ");

		char ans = LCl_readch();
		if(ans == LCLCH_ERR) BFe_report_err(BFE_UNKNOWN_ERROR);

		if(ans == 'Y' || ans == 'y' || ans == '\n')
			BFf_save_file(old_code, old_size);
	}

	else if(LCe_dirty) {
		puts("you have unsaved changes.");
		BFf_save_file(old_code, old_size);
	}

	free(old_code);
	LCe_dirty = false;
	return FILE_OK;
}

void BFf_save_file(char *buffer, size_t size) {
	FILE *file = stdin;
	int ret;
	
	LCl_buffer = BFf_savefile_name;
	LCl_length = BF_LINE_SIZE;

	while(!file || file == stdin) {
		printf("Savefile name: ");
		if(BFc_no_ansi) ret = LCl_bread();
		else ret = LCl_read();

		switch(ret) {
		case LCL_OK:
			break;

		case LCL_CUT:
			BFe_report_err(BFE_LINE_TOO_LONG);
			continue;

		case LCL_INT:
			return;

		case LCL_CUT_INT:
			putchar('\n');
			BFe_report_err(BFE_LINE_TOO_LONG);
			return;

		default:
			BFe_report_err(BFE_UNKNOWN_ERROR);
		}

		file = fopen(BFf_savefile_name, "w");
		if(!file) {
			BFe_file_name = BFf_savefile_name;
			BFe_report_err(BFE_FILE_UNWRITABLE);
		}
	}

	for(size_t i = 0; i < size; i++) fputc(buffer[i], file);

	ret = fclose(file);
	if(ret == EOF) BFe_report_err(BFE_UNKNOWN_ERROR);
}

void BFf_dump_mem() {
	FILE *file = fopen(BFf_outfile_name, "w");
	if(!file) {
		BFe_file_name = BFf_outfile_name;
		BFe_report_err(BFE_FILE_UNWRITABLE);
		return;
	}

	int width = hex_digits(BFi_mem_size);
	size_t cols = (80 - 8 - width) / 4;

	for(size_t i = 0; i < BFi_mem_size; i += cols) {
		fprintf(file, "  %0*zx:", width, i);

		for(size_t j = 0; j < cols; j++) {
			if(i + j >= BFi_mem_size) { printf("   "); continue; }
			unsigned char byte = BFi_mem[i + j];

			fprintf(file, " %02x", byte);
		}
		
		fprintf(file, " |");

		for(size_t j = 0; j < cols; j++) {
			if(i + j >= BFi_mem_size) { printf("."); continue; }
			unsigned char byte = BFi_mem[i + j];

			if(isprint(byte)) fputc(byte, file);
			else fputc('.', file);
		}

		fputs("|\n", file);
	}

	int ret = fclose(file);
	if(ret == EOF) BFe_report_err(BFE_UNKNOWN_ERROR);
}

static int check_file(size_t len) {
	int loops_open = 0;

	for(size_t i = 0; i < len; i++) {
		switch(BFi_program_str[i]) {
		case '[': loops_open++; break;
		case ']': loops_open--; break;

		case '\t': break;
		case '\n': break;
		case '\r': break;

		default: if(!isprint(BFi_program_str[i])) {
			BFe_code_error = "non-ascii characters in file.";
			return BFE_BAD_CODE;
		}}

		if(loops_open < 0) {
			BFe_code_error = "unmatched ']'.";
			return BFE_BAD_CODE;
		}
	}

	if(loops_open > 0) {
		BFe_code_error = "unmatched '['.";
		return BFE_BAD_CODE;
	}

	return FILE_OK;
}

static void get_file() {
	int ret = BFf_load_file();

	if(ret != FILE_OK) {
		BFe_file_name = BFf_mainfile_name;
		BFe_report_err(ret);
		exit(ret);
	}

	if(BFt_translate) BFt_translate_c();
}

static int hex_digits(size_t n) {
	int ret = 0;
	while(n) { n /= 16; ret++; }
	return ret;
}