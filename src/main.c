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

#include "print.h"
#include "run.h"
#include "size.h"

int main(int argc, char **argv) {
	progname = argv[0];
	if(argc > 1) print_error(BAD_ARGS);

	static char line[LINE_SIZE];
	size_t insertion_point = 0;

	printf("Bfcli: The Interactive Brainfuck Command-Line Interpreter\n");
	printf("Copyright (C) 2021 Jyothiraditya Nellakra\n");

	printf("This program comes with ABSOLUTELY NO WARRANTY.\n");
	printf("This is free software, and you are welcome to redistribute it\n");
	printf("under certain conditions; for details type `?'.\n\n");

	while(!feof(stdin)) {
		if(!insertion_point) printf("[bfcli@%zx] ", ptr);
		else printf("> ");

		if(CODE_SIZE - insertion_point < LINE_SIZE) {
			insertion_point = 0;

			print_error(CODE_TOO_LONG);
			continue;
		}

		char endl;

		int ret = scanf(" %" LINE_SIZE_PRI "[^\n]%c", line, &endl);

		if(ret != 2) print_error(-1);

		if(endl != '\n' && !feof(stdin)) {
			print_error(LINE_TOO_LONG);
			continue;
		}

		strcpy(&code[insertion_point], line);

		size_t len = strlen(code);
		int brackets_open = 0;

		for(size_t i = 0; i < len; i++) {
			switch(code[i]) {
			case '[':
				brackets_open++;
				break;

			case ']':
				brackets_open--;
				break;
			}
		}

		if(!brackets_open) {
			run(code, len);
			insertion_point = 0;
		}

		else insertion_point += strlen(line);
	}

	exit(0);
}