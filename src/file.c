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

#include "file.h"
#include "print.h"
#include "run.h"
#include "size.h"

bool transpile;
const char *imm_fname;
char filename[FILENAME_SIZE];
char outname[FILENAME_SIZE];

struct termios cooked, raw;

static void get_file();
static void convert();
static void _convert(size_t i, FILE *file);

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

	if(transpile) convert();
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

	size_t size = ftell(file) + 1;
	rewind(file);

	if(size > CODE_SIZE) {
		ret = fclose(file);
		if(ret == EOF) print_error(UNKNOWN_ERROR);
		return FILE_TOO_BIG;
	}

	for(size_t i = 0; i < size; i++) code[i] = fgetc(file);
	code[size - 1] = 0;

	ret = fclose(file);
	if(ret == EOF) print_error(UNKNOWN_ERROR);

	ret = check_file(size - 1);
	if(ret == BAD_CODE) return BAD_CODE;

	return FILE_OK;
}

static void convert() {
	FILE *file = stdout;
	if(strlen(outname)) {
		file = fopen(outname, "w");
		if(!file) print_error(BAD_OUTPUT);
	}

	fputs("#include <stddef.h>\n", file);
	fputs("#include <stdio.h>\n", file);
	fputs("#include <termios.h>\n", file);
	fputs("#include <unistd.h>\n", file);

	fprintf(file, "char cells[%d];\n", MEM_SIZE);
	fputs("size_t ptr = 0;\n", file);
	fputs("struct termios cooked, raw;\n", file);

	fputs("int main() {\n", file);
	fputs("tcgetattr(STDIN_FILENO, &cooked);\n", file);
	fputs("raw = cooked;\n", file);
	fputs("raw.c_lflag &= ~ICANON;\n", file);
	fputs("raw.c_lflag |= ECHO;\n", file);
	fputs("raw.c_cc[VINTR] = 3;\n", file);
	fputs("raw.c_lflag |= ISIG;\n", file);
	fputs("tcsetattr(STDIN_FILENO, TCSANOW, &raw);\n", file);

	size_t len = strlen(code);
	for(size_t i = 0; i < len; i++) _convert(i, file);

	fputs("tcsetattr(STDIN_FILENO, TCSANOW, &cooked);\n", file);
	fputs("}\n", file);

	if(strlen(outname)) {
		int ret = fclose(file);
		if(ret == EOF) print_error(UNKNOWN_ERROR);
	}

	exit(0);
}

static void _convert(size_t i, FILE *file) {
	switch(code[i]) {
	case '<':	fprintf(file, "if(ptr) { ptr--; } else { ptr = %d; }\n",
			MEM_SIZE - 1); break;

	case '>':	fprintf(file, "ptr++; if(ptr >= %d) { ptr = 0; }\n",
			MEM_SIZE); break;

	case '+':	fputs("cells[ptr]++;\n", file); break;
	case '-':	fputs("cells[ptr]--;\n", file); break;

	case '[':	fputs("while(cells[ptr]) {\n", file); break;
	case ']':	fputs("}\n", file); break;

	case '.':	fputs("putchar(cells[ptr]);\n", file); break;
	case ',':	fputs("cells[ptr] = getchar();\n", file); break;
	}
}