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

#include "clidata.h"
#include "errors.h"
#include "files.h"
#include "interpreter.h"
#include "main.h"
#include "printing.h"

void BFp_print_about() {
	puts("  Bfcli: The Interactive Brainfuck Command-Line Interpreter");
	puts("  Copyright (C) 2021 Jyothiraditya Nellakra");
	printf("  Version %d.%d: %s\n\n", BF_VERSION, BF_SUBVERSION, BF_VERNAME);

	printf("  Line buffer size: %d chars\n", BF_LINE_SIZE);
	printf("  Code buffer size: %zu chars\n", BFi_code_size);
	printf("  Memory size: %zu chars\n\n", BFi_mem_size);

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

void BFp_print_banner() {
	puts("Bfcli: The Interactive Brainfuck Command-Line Interpreter");
	puts("Copyright (C) 2021 Jyothiraditya Nellakra");
	printf("Version %d.%d: %s\n\n", BF_VERSION, BF_SUBVERSION, BF_VERNAME);

	puts("  This program comes with ABSOLUTELY NO WARRANTY. This is");
	puts("  free software, and you are welcome to redestribute it");
	puts("  under certain conditions. For details type '?'.\n");

	printf("  Line buffer size: %d chars\n", BF_LINE_SIZE);
	printf("  Code buffer size: %zu chars\n", BFi_code_size);
	printf("  Memory size: %zu chars\n\n", BFi_mem_size);

	if(strlen(BFf_mainfile_name))
		printf("  File: %s\n\n", BFf_mainfile_name);
}

void BFp_print_help() {
	puts("  Interactive Brainfuck interpreter; exit with ^C.\n");

	puts("  Brainfuck commands:");
	puts("    > Increments the data pointer.");
	puts("    < Decrements the data pointer.\n");

	puts("    + Increments the val at the data pointer.");
	puts("    - Decrements the val at the data pointer.\n");

	puts("  Note: Data values are modulo-256 unsigned integers, meaning");
	puts("        0 - 1 = 255, and 255 + 1 = 0.\n");

	puts("    . Outputs the val at the data pointer as an ASCII character.");
	puts("    , Inputs an ASCII character and stores its val at the data");
	puts("      pointer.\n");

	puts("    [ (Open bracket) begins a loop.");
	puts("    ] (Close brace) ends a loop.\n");

	puts("  Note: Loops run while the val at the data pointer is non-zero.\n");

	puts("  Extended Brainfuck commands:");
	puts("    ? Prints the help and copyright disclaimer to the console.");
	puts("    / Clears the memory and moves the pointer to 0.");
	puts("    * Prints memory values around the current pointer val.");
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
	puts("    % Edits code in the code buffer.");
	puts("    $ Disassembles the object code generated for the code buffer.\n");

	puts("  Note: In order to load a file when Bfcli is running, type the file");
	puts("        name at the main prompt. When files are loaded, they are put");
	puts("        into the code buffer.\n");

	puts("  Note: Buffer Editing functionality is disabled when the use of ANSI");
	puts("        escape sequences is disabled. % will simply print the contents");
	puts("        of the code buffer in this case.");
}

void BFp_print_minihelp() {
	printf("\n Usage: %s [ARGS] [FILE]\n\n", BFc_cmd_name);
	
	puts("  Valid arguments are:\n");

	puts("    -a, --about      | -h, --help       | -v, --version");
	puts("    -c, --colour     | -m, --minimal    | -n, --no-ansi");
	puts("    -d, --direct-inp |\n");

	puts("    -f, --file FILE  | -l, --length LEN | -r, --ram SIZE");
	puts("    -x, --assemble   | -s, --safe-code  | -t, --transpile");
	puts("    -o, --output OUT |\n");

	puts("  Happy coding! :)\n");
	exit(BFE_BAD_ARGS);
}

void BFp_print_prompt() {
	if(BFm_insertion_point) printf("> ");
	else if(!BFc_use_colour) printf("bfcli:%zx%% ", BFi_mem_ptr);
	else printf("\e[93mbfcli\e[0m:\e[33m%zx\e[0m%% ", BFi_mem_ptr);
}

void BFp_print_usage() {
	printf("  Usage: %s [ARGS] [FILE]\n\n", BFc_cmd_name);
	
	puts("  Valid arguments are:");
	puts("    -a, --about       Prints the licence and about dialogue.");
	puts("    -h, --help        Prints the help dialogue.");
	puts("    -v, --version     Prints the program version.\n");

	puts("    -c, --colour      Enables colour output.");
	puts("    -n, --no-ansi     Disables the use of ANSI escape sequences.");
	puts("    -m, --minimal     Disables Brainfuck extensions.\n");

	puts("  Note: Colour support and use of ANSI escape sequences is enabled");
	puts("        by default.\n");

	puts("  Note: When in Minimal Mode, it's you and the original Brainfuck");
	puts("        language, and that's it. All of the extensions of interactive");
	puts("        mode are disabled.\n");

	puts("    -f, --file FILE   Loads the file FILE into memory.");
	puts("    -l, --length LEN  Sets the shell's code buffer length to LEN.\n");

	puts("  Note: If a file is specified without -f, it is run immediately and");
	puts("        the program exits as soon as the execution of the file");
	puts("        terminates. Use -f if you want the interactive prompt.\n");

	puts("  Note: If a file is specified with -f, the code buffer's length is");
	puts("        set to LEN plus the file's length.\n");

	puts("    -o, --output OUT  Sets the output file for the transpiled C");
	puts("                      code and the memory dump to OUT.\n");

	puts("    -r, --ram SIZE   Sets the total memory size for the compiled");
	puts("                      program to SIZE.\n");

	puts("    -s, --safe-code   Generates code that won't segfault if < or");
	puts("                      > are used out-of-bounds. (The pointer wraps");
	puts("                      around.)\n");

	puts("    -t, --transpile   Transpiles the file to C source code, ouputs");
	puts("                      the result to OUT and exits.\n");
	
	puts("    -x, --assemble    Generates assembly code intermixed with the C");
	puts("                      output. This option affords both high performance");
	puts("                      and fast compile times, however it only works on");
	puts("                      amd64-based computers.\n");

	puts("    -d, --direct-inp  Don't buffer the standard input. Send characters");
	puts("                      to Brainfuck code without waiting for a newline.\n");

	puts("  Note: If no output file is specified, the transpiled code is output");
	puts("        to STDOUT. Code generated with -s may be both slower to compile");
	puts("        and execute, so only use it when necessary.\n");

	puts("  Happy coding! :)");
}

void BFp_print_bytecode() {
	bool no_pause = false;
	size_t pages = 1;

	BFc_get_dimensions();
	BFi_compile();

	BFi_instr_t *instr = BFi_program_code;
	BFi_instr_t *first = instr;

	for(size_t i = 0; BFi_is_running && instr; i++) {
		if(i >= BFc_height * pages && !BFc_no_ansi && !no_pause) {
			printf(":");

			signed char ret = LCl_readch();
			switch(ret) {
				case LCLCH_ERR:	BFe_report_err(BFE_UNKNOWN_ERROR);
				case LCLCH_INT:	return;

				case '\t':	no_pause = true; break;
				case '\n':	pages++; break;
				default:  	break;
			}

			printf("\e[%zu;1H", BFc_height - 1);
		}

		size_t val = instr -> operand.value;
		switch(instr -> opcode) {
		case BFI_INSTR_NOP:
			printf("  %zx  nop\n", i);
			break;
		
		case BFI_INSTR_INP:
			printf("  %zx  inp\e[23G| , |\n", i);
			break;
		
		case BFI_INSTR_OUT:
			printf("  %zx  out\e[23G| . |\n", i);
			break;
		
		case BFI_INSTR_INC:
			printf("  %zx  inc %zx\e[23G| + | %zu\n", i, val, val);
			break;
		
		case BFI_INSTR_DEC:
			printf("  %zx  dec %zx\e[23G| - | %zu\n", i, val, val);
			break;
		
		case BFI_INSTR_FWD:
			printf("  %zx  fwd %zx\e[23G| > | %zu\n", i, val, val);
			break;
		
		case BFI_INSTR_BCK:
			printf("  %zx  bck %zx\e[23G| < | %zu\n", i, val, val);
			break;
		}
		
		size_t count = 0;
		for(BFi_instr_t *j = first; j; j = j -> next) {
			if(j == instr -> operand.ptr) break;
			else count++; 
		}

		switch(instr -> opcode) {
		case BFI_INSTR_JMP:
			printf("  %zx  jmp %zx\e[23G| ] |\n", i, count);
			break;
		
		case BFI_INSTR_JZ:
			printf("  %zx  jz  %zx\e[23G| [ |\n", i, count);
			break;
		}

		instr = instr -> next;
	}
}

void BFp_peek_at_mem() {
	BFc_get_dimensions();
	size_t rows = (BFc_height * 2) / 3;
	size_t cols = (BFc_width - BF_MEM_SIZE_DIGITS - 8) / 4;

	size_t i = BFi_mem_ptr, offset = (rows * cols) / 2;

	for(size_t j = 0; j < offset; j++)
		if(--i >= BFi_mem_size) i = BFi_mem_size - 1;

	offset = rows * cols;
	for(size_t j = 0; j < offset; j += cols) {
		size_t addr = i + j;

		if(addr >= BFi_mem_size) addr -= BFi_mem_size;
		printf("  " BF_MEM_SIZE_PRI ":", addr);

		for(size_t k = 0; k < cols; k++) {
			size_t sub_addr = i + j + k;
			if(sub_addr >= BFi_mem_size) sub_addr -= BFi_mem_size;
			unsigned char byte = BFi_mem[sub_addr];

			if(BFc_use_colour) {
				if(sub_addr == BFi_mem_ptr) printf("\e[97m");
				else if(byte) printf("\e[93m");
				else printf("\e[33m");
			}

			printf(" %02x", byte);
		}

		if(BFc_use_colour) printf("\e[0m |");
		else printf(" |");

		for(size_t k = 0; k < cols; k++) {
			size_t sub_addr = i + j + k;
			if(sub_addr >= BFi_mem_size) sub_addr -= BFi_mem_size;
			unsigned char byte = BFi_mem[sub_addr];

			if(isprint(byte)) putchar(byte);
			else putchar('.');
		}

		puts("|");
	}
}

void BFp_dump_mem() {
	bool no_pause = false;
	size_t pages = 1;

	BFc_get_dimensions();
	size_t cols = (BFc_width - BF_MEM_SIZE_DIGITS - 8) / 4;

	for(size_t i = 0; BFi_is_running && i < BFi_mem_size; i += cols) {
		if(i >= BFc_height * cols * pages && !BFc_no_ansi && !no_pause) {
			printf(":");

			signed char ret = LCl_readch();
			switch(ret) {
				case LCLCH_ERR:	BFe_report_err(BFE_UNKNOWN_ERROR);
				case LCLCH_INT:	return;

				case '\t':	no_pause = true; break;
				case '\n':	pages++; break;
				default:  	break;
			}

			printf("\e[%zu;1H  " BF_MEM_SIZE_PRI ":", BFc_height - 1, i);
		}

		else printf("  " BF_MEM_SIZE_PRI ":", i);

		for(size_t j = 0; j < cols; j++) {
			if(i + j >= BFi_mem_size) { printf("   "); continue; }
			unsigned char byte = BFi_mem[i + j];

			if(BFc_use_colour) {
				if(byte) printf("\e[93m");
				else printf("\e[33m");
			}

			printf(" %02x", byte);
		}

		if(BFc_use_colour) printf("\e[0m |");
		else printf(" |");

		for(size_t j = 0; j < cols; j++) {
			if(i + j >= BFi_mem_size) { printf("."); continue; }
			unsigned char byte = BFi_mem[i + j];

			if(isprint(byte)) putchar(byte);
			else putchar('.');
		}

		puts("|");
	}
}