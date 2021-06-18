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
#include "size.h"

struct termios cooked, raw;

char filename[FILENAME_SIZE];
char *filecode;

static int check_file(size_t len) {
	int loops_open = 0;

	for(size_t i = 0; i < len; i++) {
		switch(filecode[i]) {
		case '[':
			loops_open++;
			continue;

		case ']':
			loops_open--;
			continue;

		case '\t':
		case '\n':
		case '\r':
			continue;
		}

		if(!isprint(filecode[i])) return BAD_CODE;
	}

	return loops_open ? BAD_CODE : FILE_OK;
}

void init_files() {
	if(strlen(filename)) {
		int ret = load_file();
		
		if(ret != FILE_OK) {
			print_error(ret);
			exit(ret);
		}
	}

	int ret = tcgetattr(STDIN_FILENO, &cooked);
	if(ret == -1) print_error(UNKNOWN_ERROR);

	raw = cooked;
	raw.c_lflag &= ~ICANON;
	raw.c_lflag |= ECHO;
	raw.c_cc[VINTR] = 3;
	raw.c_lflag |= ISIG;
}

int load_file() {
	FILE *file = fopen(filename, "r");
	if(!file) return BAD_FILE;
	
	int ret = fseek(file, 0, SEEK_END);
	if(ret) print_error(UNKNOWN_ERROR);

	size_t size = ftell(file) + 1;
	rewind(file);

	char *oldcode = filecode;

	filecode = malloc(size);
	if(!filecode) print_error(UNKNOWN_ERROR);

	for(size_t i = 0; i < size; i++) filecode[i] = fgetc(file);
	filecode[size - 1] = 0;

	ret = fclose(file);
	if(ret == EOF) print_error(UNKNOWN_ERROR);

	ret = check_file(size - 1);

	if(ret == BAD_CODE) {
		free(filecode);
		filecode = oldcode;
		return BAD_CODE;
	}

	if(oldcode) free(oldcode);
	return FILE_OK;
}