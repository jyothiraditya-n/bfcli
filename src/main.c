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
#include <stdlib.h>
#include <string.h>

#include <signal.h>
#include <termios.h>
#include <unistd.h>

#include <LC_args.h>
#include <LC_vars.h>

#include "print.h"
#include "run.h"
#include "size.h"

#define NXOR(A, B) ((A && B) || (!A && !B))

static int check(char *code, size_t len);

#define CODE_OK 1
#define CODE_INCOMPLETE 2
#define CODE_ERROR 3

static void about();
static void help();

static struct termios cooked, raw;
static void init(int argc, char **argv);

static void about() {
	putchar('\n');
	print_about();
	putchar('\n');
	exit(0);
}

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

static void help() {
	putchar('\n');
	print_help();
	putchar('\n');
	exit(0);
}

static void init(int argc, char **argv) {
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

	int ret = LCa_read(argc, argv);
	if(ret == LCA_BAD_CMD) print_error(BAD_ARGS);
	else if(ret != LCA_OK) print_error(UNKNOWN_ERROR);
	
	tcgetattr(0, &cooked);
	raw = cooked;

	raw.c_lflag &= ~ICANON;
	raw.c_lflag |= ECHO;

	raw.c_cc[VINTR] = 3;
	raw.c_lflag |= ISIG;
}

int main(int argc, char **argv) {
	init(argc, argv);
	print_banner();

	static char line[LINE_SIZE];
	size_t insertion_point = 0;

	while(!feof(stdin)) {
		tcsetattr(STDIN_FILENO, TCSANOW, &cooked);
		print_prompt(insertion_point);

		if(CODE_SIZE - insertion_point < LINE_SIZE) {
			insertion_point = 0;

			print_error(CODE_TOO_LONG);
			continue;
		}

		char endl;

		int ret = scanf(" " LINE_SIZE_SCN "%c", line, &endl);

		if(ret != 2) print_error(UNKNOWN_ERROR);

		if(endl != '\n' && !feof(stdin)) {
			print_error(LINE_TOO_LONG);
			continue;
		}

		strcpy(&code[insertion_point], line);

		size_t len = strlen(code);
		ret = check(code, len);

		switch(ret) {
		case CODE_OK:
			tcsetattr(STDIN_FILENO, TCSANOW, &raw);
			running = true;

			run(code, len);

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