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

#include <ctype.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <LC_lines.h>

#include "file.h"
#include "init.h"
#include "main.h"
#include "print.h"
#include "run.h"
#include "size.h"

const char *progname;
bool colour = true;

size_t height = 24;
size_t width = 80;

void print_about() {
	puts("  Bfcli: The Interactive Brainfuck Command-Line Interpreter");
	puts("  Copyright (C) 2021 Jyothiraditya Nellakra");
	printf("  Version %d.%d: %s\n\n", VERSION, SUBVERSION, VERNAME);

	printf("  Line buffer size: %d chars\n", LINE_SIZE);
	printf("  Code buffer size: %zu chars\n", code_size);
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
	printf("  Code buffer size: %zu chars\n", code_size);
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
			progname, filename); break;

	case BAD_CODE:
		fprintf(stderr, "%s: error: '%s': invalid brainfuck.\n",
			progname, filename); break;

	case FILE_TOO_BIG:
		fprintf(stderr, "%s: error: '%s': file too big.\n",
			progname, filename); break;

	case BAD_OUTPUT:
		fprintf(stderr, "%s: error: cannot write file '%s'.\n",
			progname, outname); break;

	case BAD_SAVEFILE:
		fprintf(stderr, "%s: error: cannot write file '%s'.\n",
			progname, savename); break;

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
	puts("    * Prints memory values around the current pointer value.");
	puts("    & Prints all memory values.\n");

	puts("  Note: When ANSI support is enabled, & pauses at the end of the");
	puts("        first screen of text and displays a the prompt ':'. Here,");
	puts("        you can type any key to advance the memory dump by one");
	puts("        line, enter to advance it by one page, or tab to complete");
	puts("        the rest of the dump without pausing again.\n");

	puts("  Note: When an output file is specified with -o, the memory dump");
	puts("        is redirected to that file instead of being displayed on");
	puts("        the console.\n");

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

	puts("    @ Executes code from the code buffer.");
	puts("    % Edits code in the code buffer.\n");

	puts("  Note: In order to load a file when Bfcli is running, type the file");
	puts("        name at the main prompt. When files are loaded, they are put");
	puts("        into the code buffer.\n");

	puts("  Note: Buffer Editing functionality is disabled when the use of ANSI");
	puts("        escape sequences is disabled. % will simply print the contents");
	puts("        of the code buffer in this case.");
}

void print_mem() {
	gethw();
	size_t rows = (height * 2) / 3;
	size_t cols = (width - MEM_SIZE_DIGITS - 8) / 4;

	size_t i = ptr, offset = (rows * cols) / 2;
	for(size_t j = 0; j < offset; j++) if(--i >= MEM_SIZE) i = MEM_SIZE - 1;

	offset = rows * cols;
	for(size_t j = 0; j < offset; j += cols) {
		size_t addr = i + j; if(addr >= MEM_SIZE) addr -= MEM_SIZE;
		printf("  " MEM_SIZE_PRI ":", addr);

		for(size_t k = 0; k < cols; k++) {
			size_t sub_addr = i + j + k;
			if(sub_addr >= MEM_SIZE) sub_addr -= MEM_SIZE;
			unsigned char byte = mem[sub_addr];

			if(colour) {
				if(sub_addr == ptr) printf("\e[97m");
				else if(byte) printf("\e[93m");
				else printf("\e[33m");
			}

			printf(" %02x", byte);
		}

		if(colour) printf("\e[0m |");
		else printf(" |");

		for(size_t k = 0; k < cols; k++) {
			size_t sub_addr = i + j + k;
			if(sub_addr >= MEM_SIZE) sub_addr -= MEM_SIZE;
			unsigned char byte = mem[sub_addr];

			if(isprint(byte)) putchar(byte);
			else putchar('.');
		}

		puts("|");
	}
}

void print_mem_full() {
	bool no_pause = false;
	size_t pages = 1;

	gethw(); size_t cols = (width - MEM_SIZE_DIGITS - 8) / 4;

	for(size_t i = 0; i < MEM_SIZE; i += cols) {
		if(i >= height * cols * pages && !no_ansi && !no_pause) {
			printf(":");

			signed char ret = LCl_readch();
			switch(ret) {
				case LCLCH_ERR:	print_error(UNKNOWN_ERROR);
				case LCLCH_INT:	return;

				case '\t':	no_pause = true; break;
				case '\n':	pages++; break;
				default:  	break;
			}

			printf("\e[%zu;1H  " MEM_SIZE_PRI ":", height - 1, i);
		}

		else printf("  " MEM_SIZE_PRI ":", i);

		for(size_t j = 0; j < cols; j++) {
			if(i + j >= MEM_SIZE) { printf("   "); continue; }
			unsigned char byte = mem[i + j];

			if(colour) {
				if(byte) printf("\e[93m");
				else printf("\e[33m");
			}

			printf(" %02x", byte);
		}

		if(colour) printf("\e[0m |");
		else printf(" |");

		for(size_t j = 0; j < cols; j++) {
			if(i + j >= MEM_SIZE) { printf("."); continue; }
			unsigned char byte = mem[i + j];

			if(isprint(byte)) putchar(byte);
			else putchar('.');
		}

		puts("|");
	}
}

void print_prompt(size_t insertion_point) {
	if(colour) {
		if(!insertion_point)
			printf("\e[93m" "bfcli"
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
	puts("    -a, --about       Prints the licence and about dialogue.");
	puts("    -h, --help        Prints the help dialogue.");
	puts("    -v, --version     Prints the program version.\n");

	puts("    -c, --colour      (Default) Enables colour output.");
	puts("    -m, --monochrome  Disables colour output.");
	puts("    -n, --no-ansi     Disables the use of ANSI escape sequences.\n");

	puts("    -f, --file FILE   Loads the file FILE into memory.");
	puts("    -l, --length LEN  Sets the shell's code buffer length to LEN.\n");

	puts("    -t, --transpile   Transpiles the file to C source code, ouputs");
	puts("                      the result to OUT and exits.\n");

	puts("    -o, --output OUT  Sets the output file for the transpiled C");
	puts("                      code and the memory dump to OUT.\n");
	
	puts("    -s, --safe-code   Generates code that won't segfault if < or");
	puts("                      > are used out-of-bounds. (The pointer wraps");
	puts("                      around.)\n");

	puts("    -x, --assembly    Generates assembly code intermixed with the C");
	puts("                      output. This option affords both high performance");
	puts("                      and fast compile times, however it only works on");
	puts("                      amd64-based computers.\n");

	puts("  Note: If a file is specified without -f, it is run immediately and");
	puts("        the program exits as soon as the execution of the file");
	puts("        terminates. Use -f if you want the interactive prompt.\n");

	puts("  Note: If no output file is specified, the transpiled code is output");
	puts("        to STDOUT. Code generated with -s may be both slower to compile");
	puts("        and execute, so only use it when necessary.\n");

	puts("  Happy coding! :)");
}