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

#include <LC_lines.h>

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
	bool wait = false;

	for(size_t i = 0; i < len; i++) {
		switch(code[i]) {
		case '[':
			loops_open++;
			break;

		case ']':
			loops_open--;
			break;

		case '!':
			wait = true;
			break;

		case ';':
			wait = false;
			break;
		}
	}

	if(wait || loops_open) return CODE_INCOMPLETE;
	return CODE_OK;
}

int main(int argc, char **argv) {
	init(argc, argv);
	print_banner();

	static char line[LINE_SIZE];
	LCl_t lcl;

	lcl.data = line;
	lcl.length = LINE_SIZE;

	while(!feof(stdin)) {
		int ret = tcsetattr(STDIN_FILENO, TCSANOW, &cooked);
		if(ret == -1) print_error(UNKNOWN_ERROR);

		print_prompt(insertion_point);

		if(CODE_SIZE - insertion_point < LINE_SIZE) {
			insertion_point = 0;

			print_error(CODE_TOO_LONG);
			continue;
		}

		ret = LCl_read(&lcl);

		switch(ret) {
		case LCL_OK:
			break;

		case LCL_CUT:
			print_error(LINE_TOO_LONG);
			continue;

		case LCL_EOF:
			puts("^D");
			exit(0);

		case LCL_INT:
			insertion_point = 0;
			putchar('\n');
			continue;

		case LCL_CUT_EOF:
			print_error(LINE_TOO_LONG);
			puts("^D");
			exit(0);

		case LCL_CUT_INT:
			insertion_point = 0;
			putchar('\n');
			print_error(LINE_TOO_LONG);
			continue;

		default:
			print_error(UNKNOWN_ERROR);
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

			lastch = '\n';
			run(code, len, false);
			if(lastch != '\n') putchar('\n');

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