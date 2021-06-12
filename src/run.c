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

#include <stddef.h>
#include <stdio.h>

#include "print.h"
#include "run.h"
#include "size.h"

char code[CODE_SIZE];
unsigned char mem[MEM_SIZE];
size_t ptr;

static void clear_mem();
static void run_sub(char *start, size_t *sub_start, size_t i);

void run(char *start, size_t len) {
	int brackets_open = 0;
	size_t sub_start = 0;

	for(size_t i = 0; i < len; i++) {
		switch(start[i]) {
		case '[':
			if(!sub_start) sub_start = i + 1;
			brackets_open++;
			continue;

		case ']':
			brackets_open--;
			if(!brackets_open) run_sub(start, &sub_start, i);
			continue;
		}

		if(sub_start) continue;

		switch(start[i]) {
		case '>':
			if(ptr < MEM_SIZE - 1) ptr++;
			else ptr = 0;

			break;

		case '<':
			if(ptr > 0) ptr--;
			else ptr = MEM_SIZE - 1;

			break;

		case '+':
			mem[ptr]++;
			break;

		case '-':
			mem[ptr]--;
			break;

		case '.':
			putchar(mem[ptr]);
			break;

		case ',':
			mem[ptr] = getchar();
			break;

		case '?':
			putchar('\n');
			print_about();

			putchar('\n');
			print_help();

			putchar('\n');
			break;

		case '/':
			clear_mem();
			ptr = 0;
			break;

		case '*':
			putchar('\n');
			print_mem();
			putchar('\n');
			break;
		}
	}
}

static void clear_mem() {
	for(size_t j = 0; j < MEM_SIZE; j++) mem[j] = 0;
}

static void run_sub(char *start, size_t *sub_start, size_t i) {
	while(mem[ptr]) run(&start[*sub_start], i - *sub_start);
	*sub_start = 0;
}