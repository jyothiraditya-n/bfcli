/* Bfcli: The Interactive Brainfuck Command-Line Interpreter
 * Copyright (C) 2021-2022 Jyothiraditya Nellakra
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

static int hex_digits(size_t n);

void BFp_print_about() {
	puts("  Bfcli: The Interactive Brainfuck Command-Line Interpreter");
	puts("  Copyright (C) 2021-2022 Jyothiraditya Nellakra");
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
	puts("Copyright (C) 2021-2022 Jyothiraditya Nellakra");
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

	puts("  Note: Data values are modulo-256 unsigned integers, meaning 0 - 1 = 255, and");
	puts("        255 + 1 = 0.\n");

	puts("    . Outputs the val at the data pointer as an ASCII character.");
	puts("    , Inputs an ASCII character and stores its val at the data pointer\n");

	puts("    [ (Open bracket) begins a loop.");
	puts("    ] (Close brace) ends a loop.\n");

	puts("  Note: Loops run while the val at the data pointer is non-zero.\n");

	puts("  Extended Brainfuck commands:");
	puts("    ? Prints the help and copyright disclaimer to the console.");
	puts("    / Clears the memory and moves the pointer to 0.");
	puts("    * Prints memory values around the current pointer val.");
	puts("    & Prints all memory values.\n");

	puts("  Note: When ANSI support is enabled, & pauses at the end of the first screen");
	puts("        of text and displays a the prompt ':'. Here, you can type any key to");
	puts("        advance the memory dump by one line, enter to advance it by one page,");
	puts("        or tab to complete the rest of the dump without pausing again.\n");

	puts("  Note: When an output file is specified with -o, the memory dump is redirected");
	puts("        to that file instead of being displayed on the console.\n");

	puts("  Note: Extended Brainfuck commands are disabled when executing file code, and");
	puts("        will simply be ignored. This is done for compatibility with vanilla");
	puts("        Brainfuck programs.\n");

	puts("    ! Indicates to wait for more code before execution.");
	puts("    ; Indicates to stop waiting for more code before execution.\n");

	puts("  Note: The above two commands can be placed anywhere in a line and and will");
	puts("        function correctly, but they may prove most useful at the ends of lines");
	puts("        while typing long sections of code.\n");

	puts("  Note: The interpreter will still wait for more code if the current code");
	puts("        contains unmatched brackets.\n");

	puts("    @ Executes code from the code buffer.");
	puts("    % Edits code in the code buffer.");
	puts("    $ Disassemblys the object code generated for the code buffer.\n");

	puts("  Note: In order to load a file when Bfcli is running, type the file name at");
	puts("        the main prompt. When files are loaded, they are put into the code");
	puts("        buffer.\n");

	puts("  Note: Buffer Editing functionality is disabled when the use of ANSI escape");
	puts("        sequences is disabled. % will simply print the contents of the code");
	puts("        buffer in this case.");
}

void BFp_print_minihelp() {
	printf("\n Usage: %s [ARGS] [FILE]\n\n", BFc_cmd_name);
	
	puts("  Valid arguments are:\n");

	puts("    -a, --about      | -h, --help       | -v, --version    | -c, --colour");
	puts("    -m, --minimal    | -n, --no-ansi    | -f, --file FILE  |\n");

	puts("    -d, --direct-inp | -l, --length LEN | -r, --ram SIZE   | -t, --translate");
	puts("    -x, --compile    | -s, --standalone |\n");

	puts("    -o, --output OUT | -A, --arch ARCH  | -O, --optim BAND | -M, --max-subs N\n");

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

	puts("  Note: Colour support and use of ANSI escape sequences is enabled by default.\n");

	puts("  Note: When in Minimal Mode, it's you and the original Brainfuck language, and");
	puts("        that's it. All of the extensions of interactive mode are disabled.\n");

	puts("    -f, --file FILE   Loads the file FILE into memory.");
	puts("    -l, --length LEN  Sets the shell's code buffer length to LEN.");
	puts("    -r, --ram SIZE    Sets the shell's total memory size to SIZE.\n");

	puts("  Note: If a file is specified without -f, it is run immediately and the");
	puts("        program exits as soon as the execution of the file terminates. Use -f");
	puts("        if you want the interactive prompt.\n");

	puts("  Note: If a file is specified with -f, the code buffer's length is set to LEN");
	puts("        plus the file's length.\n");

	puts("    -t, --translate   Translates the file to C source code and exits.\n");
	
	puts("    -x, --compile     Generates assembly code intermixed with the C output.");
	puts("                      (Implies -t)\n");

	puts("    -s, --standalone  Generates a standalone .s assembly file. (Implies -x,");
	puts("                      Incompatible with -d)\n");

	puts("    -d, --direct-inp  Disables input buffering. Characters are sent to");
	puts("                      Brainfuck code without waiting for a newline.\n");

	puts("    -o, --output OUT  Sets the output file for the translated C code and the");
	puts("                      memory dump to OUT.\n");

	puts("    -A, --arch ARCH   Sets the assembly architecture to ARCH. Valid values are");
	puts("                      amd64, i386, 8086, z80 (this feature is in beta) and bfir");
	puts("                      (Intermediate compiler code representation).\n");

	puts("    -O, --optim BAND  Sets the optimisation band to BAND. Valid values are 0,");
	puts("                      1, 2, 3, P/p, S/s and A/a.\n");

	puts("    -M, --max-subs N  Sets the maximum number of subroutines and relocations");
	puts("                      used by `-OS` to N. (N = 0 disables the limit.)\n");

	puts("  Note: If no output file is specified, a filename is chosen automatically.\n");

	puts("  Note: The following is the list of optimisations enabled by the --optim flags:");
	puts("     0: No optimisations enabled.");
	puts("     1: Merges most FWD and BCK operations into indexed INCs and DECs.");
	puts("     2: Detects and converts loops into multiply-and-add operations.");
	puts("     3: Optimises addition to zeroed-out cells away to a simple copy.");
	puts("   P/p: Precomputes final values as far as possible.");
	puts("   S/s: Moves repeated code to dedicated subroutines to save space.");
	puts("   A/a: Applies both precomputation and subroutine detection.\n");

	puts("  Happy coding! :)");
}

void BFp_print_bytecode() {
	bool no_pause = false;
	size_t pages = 1;

	if(BFi_do_recompile) BFi_compile(false);
	BFc_get_dimensions();

	BFi_instr_t *instr = BFi_code;
	BFi_instr_t *first = instr;
	size_t total = 0;

	while(first) { first = first -> next; total++; }
	int width = hex_digits(total);

	size_t column = 0, rows = 0;
	BFc_width -= width + 7;
	first = instr;

	for(size_t i = 0; BFi_is_running && instr; i++) {
		if(rows >= BFc_height * pages && !BFc_no_ansi && !no_pause) {
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

		size_t val = instr -> op1;
		switch(instr -> opcode) {
		case BFI_INSTR_NOP:
			if(!column) column += printf("  %0*zx: nop %*s",
				width, i, width, "");
			else column += printf(" | nop %*s", width, "");
			break;
		
		case BFI_INSTR_INP:
			if(!column) column += printf("  %0*zx: inp %*s",
				width, i, width, "");
			else column += printf(" | inp %*s", width, "");
			break;
		
		case BFI_INSTR_OUT:
			if(!column) column += printf("  %0*zx: out %*s",
				width, i, width, "");
			else column += printf(" | out %*s", width, "");
			break;
		
		case BFI_INSTR_INC:
			if(!column) column += printf("  %0*zx: inc %*zx",
				width, i, width, val);
			else column += printf(" | inc %*zx", width, val);
			break;
		
		case BFI_INSTR_DEC:
			if(!column) column += printf("  %0*zx: dec %*zx",
				width, i, width, val);
			else column += printf(" | dec %*zx", width, val);
			break;
		
		case BFI_INSTR_FWD:
			if(!column) column += printf("  %0*zx: fwd %*zx",
				width, i, width, val);
			else column += printf(" | fwd %*zx", width, val);
			break;
		
		case BFI_INSTR_BCK:
			if(!column) column += printf("  %0*zx: bck %*zx",
				width, i, width, val);
			else column += printf(" | bck %*zx", width, val);
			break;
		}
		
		size_t index = i;

		switch(instr -> opcode) {
		case BFI_INSTR_JMP:
			for(BFi_instr_t *j = instr; j; j = j -> prev) {
				if(j == instr -> ptr) break;
				else index--; 
			}

			if(!column) column += printf("  %0*zx: jmp %*zx",
				width, i, width, index);
			else column += printf(" | jmp %*zx", width, index);
			break;
		
		case BFI_INSTR_JZ:
			for(BFi_instr_t *j = instr; j; j = j -> next) {
				if(j == instr -> ptr) break;
				else index++; 
			}

			if(!column) column += printf("  %0*zx: jz  %*zx",
				width, i, width, index);
			else column += printf(" | jz  %*zx", width, index);
			break;
		}

		if(column > BFc_width) {
			putchar('\n');
			column = 0;
			rows++;
		}

		instr = instr -> next;
	}

	if(column) putchar('\n');
}

void BFp_peek_at_mem() {
	BFc_get_dimensions();
	int width = hex_digits(BFi_mem_size);

	size_t rows = (BFc_height * 2) / 3, cols = (BFc_width - width - 8) / 4;
	while(rows * cols > BFi_mem_size) rows--;

	size_t i = BFi_mem_ptr, offset = (rows * cols) / 2;

	for(size_t j = 0; j < offset; j++)
		if(--i >= BFi_mem_size) i = BFi_mem_size - 1;

	offset = rows * cols;
	for(size_t j = 0; j < offset; j += cols) {
		size_t addr = i + j;

		if(addr >= BFi_mem_size) addr -= BFi_mem_size;
		printf("  %0*zx:", width, addr);

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
	int width = hex_digits(BFi_mem_size);

	BFc_get_dimensions();
	size_t cols = (BFc_width - width - 8) / 4;

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

			printf("\e[%zu;1H  %0*zx:", BFc_height - 1, width, i);
		}

		else printf("  %0*zx:", width, i);

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

static int hex_digits(size_t n) {
	int ret = 0;
	while(n) { n /= 16; ret++; }
	return ret;
}