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

#include "../inc/file.h"
#include "../inc/main.h"
#include "../inc/print.h"
#include "../inc/run.h"
#include "../inc/trans.h"
#include "../inc/size.h"

const char *imm_fname;
char filename[FILENAME_SIZE];
char outname[FILENAME_SIZE];
char savename[LINE_SIZE];

struct termios cooked, raw;

static void get_file();

int check_file(size_t len) {
	int loops_open = 0;

	for(size_t i = 0; i < len; i++) {
		switch(code[i]) {
		case '[': loops_open++; continue;
		case ']': loops_open--; continue;

		case '\t': case '\n': case '\r':
			continue;
		}

		if(!isprint(code[i])) return BAD_CODE;
	}

	return loops_open ? BAD_CODE : FILE_OK;
}

static void get_file() {
	int ret = load_file();

	if(ret != FILE_OK) {
		print_error(ret);
		exit(ret);
	}

	if(transpile) convert_file();
}

void save_file(char *buffer, size_t size) {
	printf("filename: ");
	FILE *file = stdin; int ret;

	LCl_buffer = savename;
	LCl_length = LINE_SIZE;

	while(!file || file == stdin) {
		if(no_ansi) ret = LCl_bread();
		else ret = LCl_read();

		switch(ret) {
		case LCL_OK:
			break;

		case LCL_CUT:
			print_error(LINE_TOO_LONG);
			continue;

		case LCL_INT:
			return;

		case LCL_CUT_INT:
			putchar('\n');
			print_error(LINE_TOO_LONG);
			return;

		default:
			print_error(UNKNOWN_ERROR);
		}

		file = fopen(savename, "w");
		if(!file) print_error(BAD_SAVEFILE);
	}

	for(size_t i = 0; i < size; i++) fputc(buffer[i], file);

	ret = fclose(file);
	if(ret == EOF) print_error(UNKNOWN_ERROR);
}

void init_files() {
	int ret = tcgetattr(STDIN_FILENO, &cooked);
	if(ret == -1) print_error(UNKNOWN_ERROR);

	raw = cooked;
	raw.c_lflag &= ~ICANON;
	raw.c_lflag |= ECHO;
	raw.c_cc[VINTR] = 3;
	raw.c_lflag |= ISIG;

	if(strlen(filename) && imm_fname) {
		fprintf(stderr, "%s: error: file supplied both with and "
			"without -f.\n", progname);

		print_error(BAD_ARGS);
	}

	if(imm_fname) {
		strncat(filename, imm_fname, FILENAME_SIZE - 1);
		get_file();

		ret = tcsetattr(STDIN_FILENO, TCSANOW, &raw);
		if(ret == -1) print_error(UNKNOWN_ERROR);

		running = true;
		lastch = '\n';
		run(code, strlen(code), true);
		if(lastch != '\n') putchar('\n');

		ret = tcsetattr(STDIN_FILENO, TCSANOW, &cooked);
		if(ret == -1) print_error(UNKNOWN_ERROR);

		exit(0);
	}

	if(strlen(filename)) get_file();
}

int load_file() {
	FILE *file = fopen(filename, "r");
	if(!file) return BAD_FILE;
	
	int ret = fseek(file, 0, SEEK_END);
	if(ret) print_error(UNKNOWN_ERROR);

	size_t size = (ftell(file) / sizeof(char)) + 1;
	rewind(file);

	if(!code) {
		code_size += size;
		code = calloc(code_size, sizeof(char));
		if(!code) print_error(UNKNOWN_ERROR);
	}

	else if(size > code_size) {
		ret = fclose(file);
		if(ret == EOF) print_error(UNKNOWN_ERROR);
		return FILE_TOO_BIG;
	}

	size_t old_size = strlen(code);
	char *old_code = malloc(old_size * sizeof(char));
	if(!old_code) print_error(UNKNOWN_ERROR);

	for(size_t i = 0; i <= old_size; i++) old_code[i] = code[i];
	for(size_t i = 0; i < size; i++) code[i] = fgetc(file);
	code[size - 1] = 0;

	ret = fclose(file);
	if(ret == EOF) print_error(UNKNOWN_ERROR);

	ret = check_file(size - 1);
	if(ret == BAD_CODE) {
		for(size_t i = 0; i <= old_size; i++) code[i] = old_code[i];
		free(old_code);
		return BAD_CODE;
	}

	if(LCe_dirty && !no_ansi) {
		printf("save unsaved changes? [Y/n]: ");
		char ans = LCl_readch();
		if(ans == LCLCH_ERR) print_error(UNKNOWN_ERROR);

		if(ans == 'Y' || ans == 'y' || ans == '\n')
			save_file(old_code, old_size);
	}

	else if(LCe_dirty) {
		puts("you have unsaved changes.");
		save_file(old_code, old_size);
	}

	free(old_code);
	LCe_dirty = false;
	return FILE_OK;
}

void print_mem_file() {
	FILE *file = fopen(outname, "w");
	if(!file) { print_error(BAD_OUTPUT); return; }

	const size_t cols = (80 - 8 - MEM_SIZE_DIGITS) / 4;
	for(size_t i = 0; i < MEM_SIZE; i += cols) {
		fprintf(file, "  " MEM_SIZE_PRI ":", i);

		for(size_t j = 0; j < cols; j++) {
			if(i + j >= MEM_SIZE) { printf("   "); continue; }
			unsigned char byte = mem[i + j];

			fprintf(file, " %02x", byte);
		}
		
		fprintf(file, " |");

		for(size_t j = 0; j < cols; j++) {
			if(i + j >= MEM_SIZE) { printf("."); continue; }
			unsigned char byte = mem[i + j];

			if(isprint(byte)) fputc(byte, file);
			else fputc('.', file);
		}

		fputs("|\n", file);
	}

	int ret = fclose(file);
	if(ret == EOF) print_error(UNKNOWN_ERROR);
}