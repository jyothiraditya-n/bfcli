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

#include "file.h"
#include "init.h"
#include "main.h"
#include "print.h"
#include "run.h"
#include "size.h"

#define NXOR(A, B) ((A && B) || (!A && !B))

size_t insertion_point;

static int check(char *code, size_t len);

#define CODE_OK 1
#define CODE_INCOMPLETE 2
#define CODE_ERROR 3

static int check(char *code, size_t len) {
	int loops_open = 0;

	size_t start = 0;
	size_t end = 0;

	for(size_t i = 0; i < len; i++) {
		switch(code[i]) {
		case '[':
			loops_open++;
			break;

		case ']':
			loops_open--;
			break;

		case '{':
			if(start) {
				print_error(NESTED_BRACES);
				return CODE_ERROR;
			}

			start = i + 1;
			break;

		case '}':
			if(end) {
				print_error(NESTED_BRACES);
				return CODE_ERROR;
			}

			if(!loops_open) end = i;

			else {
				print_error(BAD_BRACKETS);
				return CODE_ERROR;
			}

			break;
		}
	}

	if(!loops_open && NXOR(start, end)) return CODE_OK;
	else return CODE_INCOMPLETE;
}

int main(int argc, char **argv) {
	init(argc, argv);
	print_banner();

	static char line[LINE_SIZE];

	while(!feof(stdin)) {
		bool interrupt = insertion_point ? true : false;

		int ret = tcsetattr(STDIN_FILENO, TCSANOW, &cooked);
		if(ret == -1) print_error(UNKNOWN_ERROR);

		print_prompt(insertion_point);

		if(CODE_SIZE - insertion_point < LINE_SIZE) {
			insertion_point = 0;

			print_error(CODE_TOO_LONG);
			continue;
		}

		char endl;

		ret = scanf(" " LINE_SCN "%c", line, &endl);

		if(interrupt && !insertion_point) {
			putchar('\n');
			continue;
		}

		if(ret != 2) print_error(UNKNOWN_ERROR);

		if(endl != '\n' && !feof(stdin)) {
			print_error(LINE_TOO_LONG);
			continue;
		}

		strcpy(filename, line);
		ret = load_file();

		if(ret == FILE_OK) {
			printf("loaded '%s'.\n", filename);
			continue;
		}

		strcpy(&code[insertion_point], line);

		size_t len = strlen(code);
		ret = check(code, len);

		switch(ret) {
		case CODE_OK:
			tcsetattr(STDIN_FILENO, TCSANOW, &raw);
			running = true;

			run(code, len, false);

			insertion_point = 0;
			running = false;
			break;

		case CODE_INCOMPLETE:
			insertion_point += strlen(line);
			break;

		case CODE_ERROR:
			insertion_point = 0;
			break;
		}
	}

	exit(0);
}