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

#define COMMAND_STRING 0
#define PROGRAM_STRING 1
#define PARTIAL_OUTPUT 2

char *BFi_program_str;
BFi_instr_t *BFi_code;
size_t BFi_code_size = BF_CODE_SIZE;

static BFi_instr_t *cmd_code;

bool Bfi_do_recompile = true;
bool BFi_is_running;
char BFi_last_output;

unsigned char *BFi_mem;
size_t BFi_mem_ptr;
size_t BFi_mem_size = BF_MEM_SIZE;

static BFi_instr_t *compile(char *str, int mode);
static void run(BFi_instr_t *instr);

static void append_simple(BFi_instr_t **current, int opcode);
static void append_cmplx(BFi_instr_t **current, int *opcode, size_t *op,
			 int context);

static char get_input();

void BFi_init() {
	BFi_mem = calloc(BFi_mem_size, sizeof(unsigned char));
	if(!BFi_mem) BFe_report_err(BFE_UNKNOWN_ERROR);
}

void BFi_compile(bool translate) {
	while(BFi_code) {
		BFi_instr_t *instr = BFi_code;
		BFi_code = instr -> next;
		free(instr);
	}

	if(!translate) {
		BFi_code = compile(BFi_program_str, PROGRAM_STRING);
		Bfi_do_recompile = false;
	}

	else BFi_code = compile(BFi_program_str, PARTIAL_OUTPUT);
	return;
}

void BFi_main(char *command_str) {
	while(cmd_code) {
		BFi_instr_t *instr = cmd_code;
		cmd_code = instr -> next;
		free(instr);
	}

	cmd_code = compile(command_str, COMMAND_STRING);
	run(cmd_code);
}

void BFi_exec() {
	if(Bfi_do_recompile) BFi_compile(false);
	run(BFi_code);
}

static BFi_instr_t *compile(char *str, int mode) {
	BFi_instr_t *first = malloc(sizeof(BFi_instr_t));
	if(!first) BFe_report_err(BFE_UNKNOWN_ERROR);

	first -> prev = NULL;
	first -> next = NULL;
	first -> ptr = NULL;

	first -> opcode = BFI_INSTR_NOP;
	first -> op1 = 0;
	first -> op2 = 0;
	first -> ad = 0;

	size_t length = strlen(str);
	size_t brackets = 0, nesting = 0;

	for(size_t i = 0; i < length; i++)
		if(str[i] == '[') brackets++;

	BFi_instr_t **stack1 = malloc(sizeof(BFi_instr_t *) * brackets);
	size_t *stack2 = malloc(sizeof(size_t) * brackets);

	if(!(stack1 && stack2)) BFe_report_err(BFE_UNKNOWN_ERROR);

	BFi_instr_t *current = first;
	brackets = 0;

	int opcode = BFI_INSTR_NOP;
	size_t op = 0;

	for(size_t i = 0; i < length; i++) {
		switch(str[i]) {
		case '+':
			append_cmplx(&current, &opcode, &op, BFI_INSTR_INC);
			continue;

		case '-':
			append_cmplx(&current, &opcode, &op, BFI_INSTR_DEC);
			continue;

		case '>':
			append_cmplx(&current, &opcode, &op, BFI_INSTR_FWD);
			continue;

		case '<':
			append_cmplx(&current, &opcode, &op, BFI_INSTR_BCK);
			continue;
		}

		append_cmplx(&current, &opcode, &op, BFI_INSTR_NOP);

		switch(str[i]) {
		case ',':
			append_simple(&current, BFI_INSTR_INP);
			continue;

		case '.':
			append_simple(&current, BFI_INSTR_OUT);
			continue;

		case '[':
			if(mode != PARTIAL_OUTPUT) {
				append_simple(&current, BFI_INSTR_JZ);
				stack1[nesting++] = current -> prev;
				continue;
			}

			append_simple(&current, BFI_INSTR_LOOP);
			current -> prev -> op1 = ++brackets;
			stack2[nesting++] = brackets;
			continue;

		case ']':
			if(mode == PARTIAL_OUTPUT) {
				append_simple(&current, BFI_INSTR_ENDL);
				current -> prev -> op1 = stack2[--nesting];
				continue;
			}

			append_simple(&current, BFI_INSTR_JMP);
			stack1[--nesting] -> ptr = current;
			current -> prev -> ptr = stack1[nesting];
			continue;
		}

		if(mode != COMMAND_STRING || BFc_minimal_mode) continue;

		switch(str[i]) {
		case '?':
			append_simple(&current, BFI_INSTR_HELP);
			break;

		case '/':
			append_simple(&current, BFI_INSTR_INIT);
			break;

		case '*':
			append_simple(&current, BFI_INSTR_MEM_PEEK);
			break;

		case '&':
			append_simple(&current, BFI_INSTR_MEM_DUMP);
			break;

		case '@':
			append_simple(&current, BFI_INSTR_EXEC);
			break;

		case '%':
			append_simple(&current, BFI_INSTR_EDIT);
			break;

		case '$':
			append_simple(&current, BFI_INSTR_COMP);
			break;
		}
	}

	if(op) {
		current -> opcode = opcode;
		current -> op1 = op;
	}

	if(stack1) free(stack1);
	if(stack2) free(stack2);
	return first;

}

static void append_simple(BFi_instr_t **current, int opcode) {
	(*current) -> opcode = opcode;

	(*current) -> next = malloc(sizeof(BFi_instr_t));
	if(!(*current) -> next) BFe_report_err(BFE_UNKNOWN_ERROR);

	(*current) -> next -> prev = *current;
	(*current) -> next -> next = NULL;
	(*current) -> next -> ptr = NULL;

	(*current) -> next -> opcode = BFI_INSTR_NOP;
	(*current) -> next -> op1 = 0;
	(*current) -> next -> op2 = 0;
	(*current) -> next -> ad = 0;

	*current = (*current) -> next;
	return;
}

static void append_cmplx(BFi_instr_t **current, int *opcode, size_t *op,
			 int context)
{
	if(*opcode == BFI_INSTR_NOP) {
		*opcode = context;
		
		if(context == BFI_INSTR_NOP) *op = 0;
		else *op = 1;
	}

	else if(*opcode != context) {
		(*current) -> opcode = *opcode;
		(*current) -> op1 = *op;

		(*current) -> next = malloc(sizeof(BFi_instr_t));
		if(!(*current) -> next) BFe_report_err(BFE_UNKNOWN_ERROR);

		(*current) -> next -> prev = *current;
		(*current) -> next -> next = NULL;
		(*current) -> next -> opcode = BFI_INSTR_NOP;

		(*current) = (*current) -> next;
		*opcode = context;

		if(context == BFI_INSTR_NOP) *op = 0;
		else *op = 1;
	}

	else (*op)++;
	return;
}

static void run(BFi_instr_t *instr) {
	while(instr && BFi_is_running) {
		switch(instr -> opcode) {
		case BFI_INSTR_INC:
			BFi_mem[BFi_mem_ptr] += instr -> op1;
			break;

		case BFI_INSTR_DEC:
			BFi_mem[BFi_mem_ptr] -= instr -> op1;
			break;

		case BFI_INSTR_FWD:
			BFi_mem_ptr += instr -> op1;
			if(BFi_mem_ptr >= BFi_mem_size) {
				BFi_mem_ptr -= instr -> op1;

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
			BFi_mem_ptr -= instr -> op1;
			if(BFi_mem_ptr >= BFi_mem_size) {
				BFi_mem_ptr += instr -> op1;

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
			instr = instr -> ptr;
			continue;

		case BFI_INSTR_JZ:
			if(BFi_mem[BFi_mem_ptr]) break;
			instr = instr -> ptr;
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