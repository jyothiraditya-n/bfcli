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
#include "main.h"
#include "print.h"
#include "run.h"
#include "size.h"

const char *progname;
bool colour = true;

void print_about() {
	puts("  Bfcli: The Interactive Brainfuck Command-Line Interpreter");
	puts("  Copyright (C) 2021 Jyothiraditya Nellakra");
	printf("  Version %d.%d: %s\n\n", VERSION, SUBVERSION, VERNAME);

	printf("  Line buffer size: %d chars\n", LINE_SIZE);
	printf("  Code buffer size: %d chars\n", CODE_SIZE);
	printf("  Memory size: %d chars\n\n", MEM_SIZE);

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
	puts("Copyright (C) 2021 Jyothiraditya Nellakra");
	printf("Version %d.%d: %s\n\n", VERSION, SUBVERSION, VERNAME);

	puts("  This program comes with ABSOLUTELY NO WARRANTY. This is");
	puts("  free software, and you are welcome to redestribute it");
	puts("  under certain conditions. For details type '?'.\n");

	printf("  Line buffer size: %d chars\n", LINE_SIZE);
	printf("  Code buffer size: %d chars\n", CODE_SIZE);
	printf("  Memory size: %d chars\n\n", MEM_SIZE);

	if(strlen(filename)) printf("  File: %s\n\n", filename);
}

void print_error(int errnum) {
	switch(errnum) {
	case BAD_ARGS:
		putchar('\n');
		print_usage();
		putchar('\n');

		exit(errnum);

	case CODE_TOO_LONG:
		fprintf(stderr, "%s: error: code too long.\n", progname);
		break;

	case LINE_TOO_LONG:
		fprintf(stderr, "%s: error: line too long.\n", progname);
		break;

	case BAD_FILE:
		fprintf(stderr, "%s: error: cannot read file '%s'.\n",
			progname, filename);

		break;

	case BAD_CODE:
		fprintf(stderr, "%s: error: '%s': invalid brainfuck.\n",
			progname, filename);

		break;

	case FILE_TOO_BIG:
		fprintf(stderr, "%s: error: '%s': file too big.\n",
			progname, filename);

		break;

	case BAD_OUTPUT:
		fprintf(stderr, "%s: error: cannot write file '%s'.\n",
			progname, outname);

		exit(errnum);

	default:
		fprintf(stderr, "%s: error: unknown error\n", progname);
		perror("cstdlib");
		
		exit(errnum);
	}
}

void print_help() {
	puts("  Interactive Brainfuck interpreter; exit with ^C.\n");

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

	puts("  Note: Extended Brainfuck commands are disabled when executing file");
	puts("        code, and will simply be ignored. This is done for");
	puts("        compatibility with vanilla Brainfuck programs.\n");

	puts("    ! Indicates to wait for more code before execution.");
	puts("    ; Indicates to stop waiting for more code before execution.\n");

	puts("  Note: The above two commands can be placed anywhere in a line and");
	puts("        and will function correctly, but they may prove most useful");
	puts("        at the ends of lines while typing long sections of code.\n");

	puts("  Note: The interpreter will still wait for more code if the current");
	puts("        code contains unmatched brackets.\n");

	puts("    @ Execute code from the code buffer.");
	puts("    % Edit code in the code buffer.\n");

	puts("  Note: In order to load a file when Bfcli is running, type the file");
	puts("        name at the main prompt. When files are loaded, they are put");
	puts("        into the code buffer.");

	puts("  Note: Buffer Editing functionality is disabled when the use of ANSI");
	puts("        escape sequences is disabled. % will simply print the contents");
	puts("        of the code buffer in this case.");
}

void print_mem() {
	size_t i =  ptr - 64;
	i -= i % 8;

	if(i >= MEM_SIZE) i += MEM_SIZE;

	for(size_t j = 0; j < 128; j += 8) {
		if(i + j >= MEM_SIZE) i = -j;
		
		printf("  " MEM_SIZE_PRI ":", i + j);

		for(size_t k = 0; k < 8; k++) {
			unsigned char byte = mem[i + j + k];

			if(colour) {
				if(byte) printf("\e[93m");
				else printf("\e[33m");
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

void print_usage() {
	printf("  Usage: %s [ARGS] [FILE]\n\n", progname);
	
	puts("  Valid arguments are:");
	puts("    -a, --about         Prints the licence and about dialogue.");
	puts("    -h, --help          Prints the help dialogue.");
	puts("    -v, --version       Prints the program version.\n");

	puts("    -c, --colour        (Default) Enables colour output.");
	puts("    -m, --monochrome    Disables colour output.");
	puts("    -n, --no-ansi       Disables the use of ANSI escape sequences.\n");

	puts("    -f, --file FILE     Loads the file FILE into memory.");
	puts("    -t, --transpile     Transpiles the file to C source code, ouputs");
	puts("                        the result to OUT and exits.\n");
	puts("    -o, --output OUT    Sets the output file for the transpiled C");
	puts("                        code to OUT.\n");

	puts("  Note: If a file is specified without -f, it is run immediately and");
	puts("        the program exits as soon as the execution of the file");
	puts("        terminates. Use -f if you want the interactive prompt.\n");

	puts("  Note: If no output file is specified, the transpiled code is output");
	puts("        to STDOUT.\n");

	puts("  Happy coding! :)");
}