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
#include <LC_lines.h>

#include "../inc/file.h"
#include "../inc/main.h"
#include "../inc/print.h"
#include "../inc/run.h"
#include "../inc/size.h"

unsigned char mem[MEM_SIZE];
char *code;
size_t ptr;

bool running;
char lastch;

static void run_sub(char *start, size_t *sub_start, size_t i, bool isfile);
static char get_input();

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
			mem[ptr] = get_input();
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
			if(strlen(outname)) { print_mem_file(); break; }
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
				printf("error: invalid brainfuck: ");
				puts(code_error);
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

static char get_input() {
	static char input[LINE_SIZE];
	static size_t length = 0;

	if(direct_inp) return getchar();

	while(!strlen(input)) {
		int ret = tcsetattr(STDIN_FILENO, TCSANOW, &cooked);
		if(ret == -1) print_error(UNKNOWN_ERROR);

		LCl_buffer = input;
		LCl_length = LINE_SIZE - 1;

		if(no_ansi) ret = LCl_bread(input, LINE_SIZE - 1);
		else ret = LCl_read();

		switch(ret) {
		case LCL_OK: 	break;
		case LCL_CUT: 	print_error(LINE_TOO_LONG); continue;
		case LCL_INT: 	break;

		case LCL_CUT_INT:
			putchar('\n');
			print_error(LINE_TOO_LONG);
			break;

		default:
			print_error(UNKNOWN_ERROR);
		}

		ret = tcsetattr(STDIN_FILENO, TCSANOW, &raw);
		if(ret == -1) print_error(UNKNOWN_ERROR);

		length = strlen(input);
		input[length + 1] = 0;
		input[length] = '\n';
		length++;
	}

	char ret = input[0];
	for(size_t i = 1; i <= length; i++)
		input[i - 1] = input[i];

	length--;
	return ret;
}