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

#include "print.h"
#include "run.h"
#include "size.h"

const char *progname;

void print_about() {
	printf("  Bfcli: The Interactive Brainfuck Command-Line Interpreter\n");
	printf("  Copyright (C) 2021 Jyothiraditya Nellakra\n\n");

	printf("  This program is free software: you can redistribute it and/or modify\n");
	printf("  it under the terms of the GNU General Public License as published by\n");
	printf("  the Free Software Foundation, either version 3 of the License, or\n");
	printf("  (at your option) any later version.\n\n");

	printf("  This program is distributed in the hope that it will be useful,\n");
	printf("  but WITHOUT ANY WARRANTY; without even the implied warranty of\n");
	printf("  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the\n");
	printf("  GNU General Public License for more details.\n\n");

	printf("  You should have received a copy of the GNU General Public License\n");
	printf("  along with this program. If not, see <https://www.gnu.org/licenses/>.\n");
}

void print_error(int errnum) {
	switch(errnum) {
	case BAD_ARGS:
		fprintf(stderr, "%s: error: too many args.\n\n", progname);
		print_help();

		exit(errnum);

	case CODE_TOO_LONG:
		fprintf(stderr, "%s: error: code too long.\n", progname);
		break;

	case LINE_TOO_LONG:
		fprintf(stderr, "%s: error: line too long.\n", progname);
		break;

	case NESTED_BRACES:
		fprintf(stderr, "%s: error: nested braces.\n", progname);
		break;

	case BAD_BRACKETS:
		fprintf(stderr, "%s: error: unpaired brackets.\n", progname);
		break;

	default:
		fprintf(stderr, "%s: error: unknown error %d\n",
			progname, errnum);

		exit(errnum);
	}
}

void print_help() {
	printf("  Usage: %s\n", progname);
	printf("  (Don't supply any arguments.)\n\n");

	printf("  Interactive Brainfuck interpreter; exit with ^C.\n\n");

	printf("  Line buffer size: %d chars\n", LINE_SIZE);
	printf("  Code buffer size: %d chars\n", CODE_SIZE);
	printf("  Memory size: %d chars\n\n", MEM_SIZE);

	printf("  Extended Brainfuck commands:\n");
	printf("    ?: Prints the help and copyright disclaimer to the console.\n");
	printf("    /: Clears the memory and moves the pointer to 0.\n");
	printf("    *: Prints memory values around the current pointer value.\n\n");

	printf("    {: Begins a block of code.\n");
	printf("    }: Ends a block of code.\n\n");

	printf("  Note, once a block of code has been started, code will not be "
		"executed until the block has been ended.\n");
}

void print_mem() {
	size_t i = ptr;

	for(int j = 0; j < 64; j++) {
		if(i) i--;
		else i = MEM_SIZE - 1;
	}

	i -= i % 8;

	for(size_t j = 0; j < 128; j += 8) {
		if(i + j >= MEM_SIZE) i = -j;

		printf("  %05zx:", i + j);

		for(size_t k = 0; k < 8; k++) {
			printf(" %02x", mem[i + j + k]);
		}

		printf("\n");
	}
}