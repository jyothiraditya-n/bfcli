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

#include <LC_editor.h>
#include <LC_lines.h>

#include "../inc/file.h"
#include "../inc/init.h"
#include "../inc/main.h"
#include "../inc/print.h"
#include "../inc/run.h"
#include "../inc/size.h"

#define NXOR(A, B) ((A && B) || (!A && !B))

size_t insertion_point;
bool no_ansi, minimal;

static int check(char *code, size_t len);
static void handle_int();

#define CODE_OK 1
#define CODE_INCOMPLETE 2
#define CODE_ERROR 3

static int check(char *code, size_t len) {
	int loops_open = 0;
	bool wait = false;

	for(size_t i = 0; i < len; i++)
		switch(code[i])
	{
		case '[': loops_open++; break;
		case ']': loops_open--; break;
		case '!': wait = true; break;
		case ';': wait = false; break;
	}

	if(wait || loops_open) return CODE_INCOMPLETE;
	return CODE_OK;
}

int main(int argc, char **argv) {
	init(argc, argv);

	if(minimal) {
		printf("Bfcli Version %d.%d: %s\n", VERSION, SUBVERSION, VERNAME);
		printf("Memory Size: %d Chars\n\n", MEM_SIZE);
	}

	else print_banner();

	static char line[LINE_SIZE];

	while(true) {
		int ret = tcsetattr(STDIN_FILENO, TCSANOW, &cooked);
		if(ret == -1) print_error(UNKNOWN_ERROR);

		if(minimal) printf("%% "); else print_prompt();
		LCl_buffer = line + insertion_point;
		LCl_length = LINE_SIZE - insertion_point;

		if(no_ansi) ret = LCl_bread(line, LINE_SIZE);
		else ret = LCl_read();

		switch(ret) {
		case LCL_OK: 	break;
		case LCL_CUT: 	print_error(LINE_TOO_LONG); continue;
		case LCL_INT: 	handle_int(); insertion_point = 0; continue;

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

		size_t len = strlen(line);
		ret = check(line, len);

		switch(ret) {
		case CODE_OK:
			ret = tcsetattr(STDIN_FILENO, TCSANOW, &raw);
			if(ret == -1) print_error(UNKNOWN_ERROR);
			
			running = true;

			lastch = '\n';
			if(minimal) run(line, len, true);
			else run(line, len, false);
			if(lastch != '\n') putchar('\n');

			insertion_point = 0;
			running = false;
			break;

		case CODE_INCOMPLETE:
			insertion_point = strlen(line);
			break;

		case CODE_ERROR:
			insertion_point = 0;
			break;
		}
	}

	exit(0);
}

static void handle_int() {
	if(!insertion_point && LCe_dirty && !no_ansi) {
		printf("save unsaved changes? [Y/n]: ");
		char ans = LCl_readch();
		if(ans == LCLCH_ERR) print_error(UNKNOWN_ERROR);
		if(ans == 'Y' || ans == 'y' || ans == '\n')
			save_file(code, strlen(code));

		exit(0);
	}

	else if(!insertion_point && LCe_dirty && no_ansi) {
		puts("you have unsaved changes.");
		save_file(code, strlen(code));
		exit(0);
	}

	else if(!insertion_point) exit(0);
}