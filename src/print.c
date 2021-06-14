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

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "file.h"
#include "print.h"
#include "run.h"
#include "size.h"

const char *progname;
bool colour = true;

void print_about() {
	puts("  Bfcli: The Interactive Brainfuck Command-Line Interpreter");
	puts("  Copyright (C) 2021 Jyothiraditya Nellakra\n");

	puts("  This program is free software: you can redistribute it and/or modify");
	puts("  it under the terms of the GNU General Public License as published by");
	puts("  the Free Software Foundation, either version 3 of the License, or");
	puts("  (at your option) any later version.\n");

	puts("  This program is distributed in the hope that it will be useful,");
	puts("  but WITHOUT ANY WARRANTY; without even the implied warranty of");
	puts("  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the");
	puts("  GNU General Public License for more details.\n");

	puts("  You should have received a copy of the GNU General Public License");
	puts("  along with this program. If not, see <https://www.gnu.org/licenses/>.");
}

void print_banner() {
	puts("Bfcli: The Interactive Brainfuck Command-Line Interpreter");
	puts("Copyright (C) 2021 Jyothiraditya Nellakra\n");

	puts("  This program comes with ABSOLUTELY NO WARRANTY; for details type `?'.");
	puts("  This is free software, and you are welcome to redistribute it");
	puts("  under certain conditions. (Hit ^C to exit.)\n");

	printf("  Line buffer size: %d chars\n", LINE_SIZE);
	printf("  Code buffer size: %d chars\n", CODE_SIZE);
	printf("  Memory size: %d chars\n\n", MEM_SIZE);

	if(filecode) printf("  File: %s\n\n", filename);
}

void print_error(int errnum) {
	switch(errnum) {
	case BAD_ARGS:
		fprintf(stderr, "%s: error: incorrect usage.\n\n", progname);
		print_help();
		putchar('\n');

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

	case BAD_FILE:
		fprintf(stderr, "%s: error: cannot read file '%s'.\n",
			progname, filename);

		break;

	case BAD_CODE:
		fprintf(stderr, "%s: error: '%s': invalid brainfuck.\n",
			progname, filename);

		break;

	default:
		perror(progname);
		fprintf(stderr, "%s: error: unknown error %d\n",
			progname, errnum);

		exit(errnum);
	}
}

void print_help() {
	printf("  Usage: %s [ARGS]\n\n", progname);
	
	puts("  Valid arguments are:");
	puts("    -a, --about		prints the licence and about dialogue.");
	puts("    -h, --help		prints the help dialogue.\n");

	puts("    -c, --colour		(default) enables colour output.");
	puts("    -m, --monochrome	disables colour output.\n");

	puts("    -f, --file FILE	loads the file FILE into memory.\n");

	puts("  Interactive Brainfuck interpreter; exit with ^C.\n");

	printf("  Line buffer size: %d chars\n", LINE_SIZE);
	printf("  Code buffer size: %d chars\n", CODE_SIZE);
	printf("  Memory size: %d chars\n\n", MEM_SIZE);

	puts("  Brainfuck commands:");
	puts("    > Increments the data pointer.");
	puts("    < Decrements the data pointer.\n");

	puts("    + Increments the value at the data pointer.");
	puts("    - Decrements the value at the data pointer.\n");

	puts("  Note: Data values are modulo-256 unsigned integers, meaning");
	puts("        0 - 1 = 255, and 255 + 1 = 0.\n");

	puts("    . Outputs the value at the data pointer as an ASCII character.");
	puts("    , Inputs an ASCII character and stores its value at the data");
	puts("      pointer.\n");

	puts("    [ (Open bracket) begins a loop.");
	puts("    ] (Close brace) ends a loop.\n");

	puts("  Note: Loops run while the value at the data pointer is non-zero.\n");

	puts("  Extended Brainfuck commands:");
	puts("    ? Prints the help and copyright disclaimer to the console.");
	puts("    / Clears the memory and moves the pointer to 0.");
	puts("    * Prints memory values around the current pointer value.\n");

	puts("    { (Open brace) begins a block of code.");
	puts("    } (Close brace) ends a block of code.\n");

	puts("  Note: Once a block of code has been started, code will not be");
	puts("        executed until the block has been ended.\n");

	puts("    @ Execute code from the loaded file.");
	puts("    % Print code from the loaded file.\n");

	puts("  Note: The above two commands perform no action if no file is loaded.");
	puts("        In order to load a file when Bfcli is running, type the file");
	puts("        name at the main prompt.\n");

	puts("  Happy coding! :)");
}

void print_mem() {
	intmax_t i = (intmax_t) ptr - 64;
	if(i < 0) i += MEM_SIZE;

	i -= i % 8;

	for(size_t j = 0; j < 128; j += 8) {
		if(i + j >= MEM_SIZE) i = -j;

		printf("  " MEM_SIZE_PRI ":", i + j);

		for(size_t k = 0; k < 8; k++) {
			unsigned char byte = mem[i + j + k];

			if(colour) {
				if(byte) printf("\e[0m");
				else printf("\e[90m");
			}

			printf(" %02x", byte);
		}

		if(colour) printf("\e[0m");
		putchar('\n');
	}
}

void print_prompt(size_t insertion_point) {
	if(colour) {
		if(!insertion_point) printf("\e[93m" "bfcli"
			"\e[0m" "@"
			"\e[33m" "data:%zx"
			"\e[0m" "$ ", ptr);

		else printf("\e[36m" "code:%zx"
			"\e[0m" "$ ", insertion_point);
	}

	else {
		if(!insertion_point) printf("bfcli@data:%zx$ ", ptr);
		else printf("code:%zx$ ", insertion_point);
	}
}