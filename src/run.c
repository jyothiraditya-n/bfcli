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

#include <LC_editor.h>

#include "file.h"
#include "main.h"
#include "print.h"
#include "run.h"
#include "size.h"

char code[CODE_SIZE];
unsigned char mem[MEM_SIZE];
size_t ptr;

bool running;
char lastch;

static void run_sub(char *start, size_t *sub_start, size_t i, bool isfile);

void run(char *start, size_t len, bool isfile) {
	int brackets_open = 0;
	size_t sub_start = 0;
	size_t file_len;
	int ret;

	for(size_t i = 0; i < len && running; i++) {
		switch(start[i]) {
		case '[':
			if(!sub_start) sub_start = i + 1;
			brackets_open++;
			continue;

		case ']':
			brackets_open--;

			if(!brackets_open)
				run_sub(start, &sub_start, i, isfile);

			continue;
		}

		if(sub_start) continue;

		switch(start[i]) {
		case '>':
			if(ptr < MEM_SIZE - 1) ptr++;
			else ptr = 0;

			continue;

		case '<':
			if(ptr > 0) ptr--;
			else ptr = MEM_SIZE - 1;

			continue;

		case '+':
			mem[ptr]++;
			continue;

		case '-':
			mem[ptr]--;
			continue;

		case '.':
			lastch = mem[ptr];
			putchar(lastch);
			continue;

		case ',':
			mem[ptr] = getchar();
			continue;
		}

		if(isfile) continue;

		switch(start[i]) {
		case '?':
			if(lastch != '\n') putchar('\n');

			putchar('\n');
			print_about();
			putchar('\n');
			print_help();
			putchar('\n');

			lastch = '\n';
			break;

		case '/':
			for(size_t j = 0; j < MEM_SIZE; j++) mem[j] = 0;
			ptr = 0;
			break;

		case '*':
			if(lastch != '\n') putchar('\n');

			putchar('\n');
			print_mem();
			putchar('\n');

			lastch = '\n';
			break;

		case '&':
			if(lastch != '\n') putchar('\n');

			putchar('\n');
			print_mem_full();
			putchar('\n');

			lastch = '\n';
			break;

		case '@':
			file_len = strlen(code);
			ret = check_file(file_len);
			if(ret == FILE_OK) run(code, file_len, true);
			
			else {
				if(lastch != '\n') putchar('\n');
				puts("error: invalid brainfuck.");
				lastch = '\n';
			}

			break;

		case '%':
			if(no_ansi) {
				if(!strlen(code)) break;
				printf("\n%s\n\n", code);
				lastch = '\n';
				break;
			}

			ret = tcsetattr(STDIN_FILENO, TCSANOW, &cooked);
			if(ret == -1) print_error(UNKNOWN_ERROR);

			ret = LCe_edit();
			if(ret != LCE_OK) print_error(UNKNOWN_ERROR);

			ret = tcsetattr(STDIN_FILENO, TCSANOW, &raw);
			if(ret == -1) print_error(UNKNOWN_ERROR);

			printf("\e[H\e[J");
			lastch = '\n';
			break;
		}
	}
}

static void run_sub(char *start, size_t *sub_start, size_t i, bool isfile) {
	while(mem[ptr] && running)
		run(&start[*sub_start], i - *sub_start, isfile);

	*sub_start = 0;
}