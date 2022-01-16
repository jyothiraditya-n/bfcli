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

#include <LC_editor.h>
#include <LC_lines.h>

#include "clidata.h"
#include "errors.h"
#include "files.h"
#include "interpreter.h"
#include "main.h"
#include "printing.h"

char *BFi_program_str;
BFi_instr_t *BFi_program_code;
size_t BFi_code_size = BF_CODE_SIZE;

static BFi_instr_t *command_code;

bool Bfi_do_recompile = true;
bool BFi_is_running;
char BFi_last_output;

unsigned char *BFi_mem;
size_t BFi_mem_ptr;
size_t BFi_mem_size = BF_MEM_SIZE;

static BFi_instr_t *compile(char *str, bool enable_exts);
static void run(BFi_instr_t *instr);

static void append_simple(BFi_instr_t **current, int opcode);
static void append_cmplx(BFi_instr_t **current, int *opcode, size_t *value,
			 int context);

static char get_input();

void BFi_init() {
	BFi_mem = calloc(BFi_mem_size, sizeof(unsigned char));
	if(!BFi_mem) BFe_report_err(BFE_UNKNOWN_ERROR);
}

void BFi_compile() {
	while(BFi_program_code) {
		BFi_instr_t *instr = BFi_program_code;
		BFi_program_code = instr -> next;
		free(instr);
	}

	BFi_program_code = compile(BFi_program_str, false);
	Bfi_do_recompile = false;
	return;
}

void BFi_main(char *command_str) {
	while(command_code) {
		BFi_instr_t *instr = command_code;
		command_code = instr -> next;
		free(instr);
	}

	command_code = compile(command_str, true);
	run(command_code);
}

void BFi_exec() {
	if(Bfi_do_recompile) BFi_compile();
	run(BFi_program_code);
}

static BFi_instr_t *compile(char *str, bool enable_exts) {
	BFi_instr_t *first = malloc(sizeof(BFi_instr_t));
	if(!first) BFe_report_err(BFE_UNKNOWN_ERROR);

	first -> prev = NULL;
	first -> next = NULL;
	first -> opcode = BFI_INSTR_NOP;
	first -> op.ptr = NULL;
	first -> op.value = 0;

	BFi_instr_t *current = first;
	size_t length = strlen(str);

	int opcode = BFI_INSTR_NOP;
	size_t value = 0;

	for(size_t i = 0; i < length; i++) {
		switch(str[i]) {
		case '+':
			append_cmplx(&current, &opcode, &value, BFI_INSTR_INC);
			continue;

		case '-':
			append_cmplx(&current, &opcode, &value, BFI_INSTR_DEC);
			continue;

		case '>':
			append_cmplx(&current, &opcode, &value, BFI_INSTR_FWD);
			continue;

		case '<':
			append_cmplx(&current, &opcode, &value, BFI_INSTR_BCK);
			continue;
		}

		append_cmplx(&current, &opcode, &value, BFI_INSTR_NOP);

		switch(str[i]) {
		case ',':
			append_simple(&current, BFI_INSTR_INP);
			continue;

		case '.':
			append_simple(&current, BFI_INSTR_OUT);
			continue;

		case '[':
			append_simple(&current, BFI_INSTR_JZ);
			continue;

		case ']':
			append_simple(&current, BFI_INSTR_JMP);
			BFi_instr_t *j = current -> prev -> prev;
			size_t loops = 1;

			while(true) {
				if(j -> opcode == BFI_INSTR_JMP) loops++;
				else if(j -> opcode == BFI_INSTR_JZ) loops--;

				if(loops) j = j -> prev;
				else break;
			}

			j -> op.ptr = current;
			current -> prev -> op.ptr = j;
			continue;
		}

		if(!enable_exts || BFc_minimal_mode) continue;

		switch(str[i]) {
		case '?':
			append_simple(&current, BFI_INSTR_HELP);
			continue;

		case '/':
			append_simple(&current, BFI_INSTR_INIT);
			continue;

		case '*':
			append_simple(&current, BFI_INSTR_MEM_PEEK);
			continue;

		case '&':
			append_simple(&current, BFI_INSTR_MEM_DUMP);
			continue;

		case '@':
			append_simple(&current, BFI_INSTR_EXEC);
			continue;

		case '%':
			append_simple(&current, BFI_INSTR_EDIT);
			continue;

		case '$':
			append_simple(&current, BFI_INSTR_COMP);
			continue;
		}
	}

	if(value) {
		current -> opcode = opcode;
		current -> op.value = value;
	}

	return first;

}

static void run(BFi_instr_t *instr) {
	while(instr && BFi_is_running) {
		switch(instr -> opcode) {
		case BFI_INSTR_INC:
			BFi_mem[BFi_mem_ptr] += instr -> op.value;
			break;

		case BFI_INSTR_DEC:
			BFi_mem[BFi_mem_ptr] -= instr -> op.value;
			break;

		case BFI_INSTR_FWD:
			BFi_mem_ptr += instr -> op.value;
			if(BFi_mem_ptr >= BFi_mem_size) {
				BFi_mem_ptr -= instr -> op.value;

				BFe_file_name = BFc_immediate
					? BFf_mainfile_name
					: BFc_cmd_name;

				if(BFi_last_output != '\n') putchar('\n');
				BFe_report_err(BFE_SEGFAULT);
				putchar('\n');

				BFi_last_output = '\n';
				return;
			}

			break;

		case BFI_INSTR_BCK:
			BFi_mem_ptr -= instr -> op.value;
			if(BFi_mem_ptr >= BFi_mem_size) {
				BFi_mem_ptr += instr -> op.value;

				BFe_file_name = BFc_immediate
					? BFf_mainfile_name
					: BFc_cmd_name;

				if(BFi_last_output != '\n') putchar('\n');
				BFe_report_err(BFE_SEGFAULT);
				putchar('\n');

				BFi_last_output = '\n';
				return;
			}

			break;

		case BFI_INSTR_INP:
			BFi_mem[BFi_mem_ptr] = get_input();
			break;

		case BFI_INSTR_OUT:
			BFi_last_output = BFi_mem[BFi_mem_ptr];
			putchar(BFi_mem[BFi_mem_ptr]);
			fflush(stdout);
			break;

		case BFI_INSTR_JMP:
			instr = instr -> op.ptr;
			continue;

		case BFI_INSTR_JZ:
			if(BFi_mem[BFi_mem_ptr]) break;
			instr = instr -> op.ptr;
			continue;

		case BFI_INSTR_HELP:
			if(BFi_last_output != '\n') putchar('\n');

			putchar('\n');
			BFp_print_about();
			putchar('\n');
			BFp_print_help();
			putchar('\n');

			BFi_last_output = '\n';
			break;

		case BFI_INSTR_INIT:
			for(size_t i = 0; i < BFi_mem_size; i++)
				BFi_mem[i] = 0;

			BFi_mem_ptr = 0;
			break;

		case BFI_INSTR_MEM_PEEK:
			if(BFi_last_output != '\n') putchar('\n');

			putchar('\n');
			BFp_peek_at_mem();
			putchar('\n');

			BFi_last_output = '\n';
			break;

		case BFI_INSTR_MEM_DUMP:
			if(strlen(BFf_outfile_name)) {
				BFf_dump_mem();
				break;
			}

			if(BFi_last_output != '\n') putchar('\n');

			putchar('\n');
			BFp_dump_mem();
			putchar('\n');

			BFi_last_output = '\n';
			break;

		case BFI_INSTR_EXEC:
			BFi_exec();
			break;

		case BFI_INSTR_EDIT:
			if(BFc_no_ansi) {
				if(!strlen(BFi_program_str)) break;
				printf("\n%s\n\n", BFi_program_str);
				BFi_last_output = '\n';
				break;
			}

			int ret = tcsetattr(STDIN_FILENO, TCSANOW, &BFc_cooked);
			if(ret == -1) BFe_report_err(BFE_UNKNOWN_ERROR);

			ret = LCe_edit();
			if(ret != LCE_OK) BFe_report_err(BFE_UNKNOWN_ERROR);

			ret = tcsetattr(STDIN_FILENO, TCSANOW, &BFc_raw);
			if(ret == -1) BFe_report_err(BFE_UNKNOWN_ERROR);

			printf("\e[H\e[J");
			Bfi_do_recompile = true;
			BFi_last_output = '\n';
			break;

		case BFI_INSTR_COMP:
			if(BFi_last_output != '\n') putchar('\n');

			putchar('\n');
			BFp_print_bytecode();
			putchar('\n');

			BFi_last_output = '\n';
			break;
		}

		instr = instr -> next;
	}
}

static void append_simple(BFi_instr_t **current, int opcode) {
	(*current) -> opcode = opcode;

	(*current) -> next = malloc(sizeof(BFi_instr_t));
	if(!(*current) -> next) BFe_report_err(BFE_UNKNOWN_ERROR);

	(*current) -> next -> prev = *current;
	(*current) -> next -> next = NULL;
	(*current) -> next -> opcode = BFI_INSTR_NOP;

	*current = (*current) -> next;
	return;
}

static void append_cmplx(BFi_instr_t **current, int *opcode, size_t *value,
			 int context)
{
	if(*opcode == BFI_INSTR_NOP) { *opcode = context; *value = 1; }

	else if(*opcode != context) {
		(*current) -> opcode = *opcode;
		(*current) -> op.value = *value;

		(*current) -> next = malloc(sizeof(BFi_instr_t));
		if(!(*current) -> next) BFe_report_err(BFE_UNKNOWN_ERROR);

		(*current) -> next -> prev = *current;
		(*current) -> next -> next = NULL;
		(*current) -> next -> opcode = BFI_INSTR_NOP;

		(*current) = (*current) -> next;
		*opcode = context;

		if(context == BFI_INSTR_NOP) *value = 0;
		else *value = 1;
	}

	else (*value)++;
	return;
}

static char get_input() {
	static char input[BF_LINE_SIZE];
	static size_t length = 0;

	if(BFc_direct_inp) return getchar();

	while(!strlen(input)) {
		int ret = tcsetattr(STDIN_FILENO, TCSANOW, &BFc_cooked);
		if(ret == -1) BFe_report_err(BFE_UNKNOWN_ERROR);

		LCl_buffer = input;
		LCl_length = BF_LINE_SIZE - 1;

		if(BFc_no_ansi) ret = LCl_bread(input, BF_LINE_SIZE - 1);
		else ret = LCl_read();

		switch(ret) {
		case LCL_OK: 	break;
		case LCL_CUT: 	BFe_report_err(BFE_LINE_TOO_LONG); continue;
		case LCL_INT: 	break;

		case LCL_CUT_INT:
			putchar('\n');
			BFe_report_err(BFE_LINE_TOO_LONG);
			break;

		default:
			BFe_report_err(BFE_UNKNOWN_ERROR);
		}

		ret = tcsetattr(STDIN_FILENO, TCSANOW, &BFc_raw);
		if(ret == -1) BFe_report_err(BFE_UNKNOWN_ERROR);

		length = strlen(input);
		input[length + 1] = 0;
		input[length] = '\n';
		length++;
	}

	char ret = input[0];
	for(size_t i = 1; i <= length; i++)
		input[i - 1] = input[i];

	length--;
	return ret;
}