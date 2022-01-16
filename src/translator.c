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

#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>

#include "arch/amd64.h"
#include "arch/i386.h"

#include "clidata.h"
#include "errors.h"
#include "files.h"
#include "interpreter.h"
#include "main.h"
#include "translator.h"

#define IS_2_POW(X) !(X & (X - 1))

static size_t log_b2(size_t val) {
	size_t count = 0;
	while(val) { val >>= 1; count++; }
	return count - 1;
}

bool BFt_compile;
bool BFt_translate;
bool BFt_standalone;
char BFt_target_arch[BF_FILENAME_SIZE];
int BFt_optim_lvl;

BFt_instr_t *BFt_code;

static BFt_instr_t *optimise_0();
static BFt_instr_t *optimise_1();

static void conv_loop(BFt_instr_t *start, BFt_instr_t *end, bool compl);
static BFt_instr_t *conv_instr(BFt_instr_t *start, BFt_instr_t *end,
			       BFt_instr_t *instr);

static void translate(FILE *file);
static void conv_standalone();

void BFt_convert_file() {
	if(BFt_standalone) conv_standalone();

	BFt_optimise();
	if(!strlen(BFf_outfile_name)) {
		strcpy(BFf_outfile_name, BFf_mainfile_name);
		strncat(BFf_outfile_name, ".c",
			BF_FILENAME_SIZE - 1 - strlen(BFf_outfile_name));
	}

	FILE *file = fopen(BFf_outfile_name, "w");

	if(!file) {
		BFe_file_name = BFf_outfile_name;
		BFe_report_err(BFE_FILE_UNWRITABLE);
		exit(BFE_FILE_UNWRITABLE);
	}

	if(!BFt_compile) fputs("#include <stdio.h>\n\n", file);

	if(BFc_direct_inp) {
		fputs("#include <termios.h>\n", file);
		fputs("#include <unistd.h>\n\n", file);

		fputs("struct termios cooked, raw;\n", file);
	}

	fprintf(file, "unsigned char cells[%zu];\n\n", BFi_mem_size);

	fputs("int main() {\n", file);
	if(BFc_direct_inp) {
		fputs("\ttcgetattr(STDIN_FILENO, &cooked);\n", file);
		fputs("\traw = cooked;\n", file);
		fputs("\traw.c_lflag &= ~ICANON;\n", file);
		fputs("\traw.c_lflag |= ECHO;\n", file);
		fputs("\traw.c_cc[VINTR] = 3;\n", file);
		fputs("\traw.c_lflag |= ISIG;\n", file);
		fputs("\ttcsetattr(STDIN_FILENO, TCSANOW, &raw);\n\n", file);
	}

	if(BFt_compile) {
		if(!strcmp(BFt_target_arch, "amd64")) BFa_amd64_tc(file);
		else if(!strcmp(BFt_target_arch, "i386")) BFa_i386_tc(file);
		else { BFe_report_err(BFE_BAD_ARCH); exit(BFE_BAD_ARCH); }
	}

	else translate(file);
	fputs("}\n", file);

	int ret = fclose(file);
	if(ret == EOF) BFe_report_err(BFE_UNKNOWN_ERROR);
	exit(0);
}

void BFt_optimise() {
	while(BFt_code) {
		BFt_instr_t *instr = BFt_code;
		BFt_code = instr -> next;
		free(instr);
	}

	BFi_compile();
	switch(BFt_optim_lvl) {
		case 0: BFt_code = optimise_0(); break;
		case 1: BFt_code = optimise_1(); break;

		default:
			BFe_report_err(BFE_BAD_OPTIM);
			exit(BFE_BAD_OPTIM);
	}
}

static BFt_instr_t *optimise_0() {
	BFi_instr_t *instr = BFi_program_code;
	BFt_instr_t *trans = malloc(sizeof(BFt_instr_t));
	if(!trans) BFe_report_err(BFE_UNKNOWN_ERROR);

	BFt_instr_t *first = trans;
	trans -> prev = NULL;
	trans -> opcode = BFT_INSTR_NOP;

	size_t brackets = 0;
	ssize_t offset = 0;

	for(BFi_instr_t *i = BFi_program_code; i; i = i -> next)
		if(i -> opcode == BFI_INSTR_JMP) brackets++;

	size_t *stack = malloc(sizeof(size_t) * brackets);
	if(!stack) BFe_report_err(BFE_UNKNOWN_ERROR);

	size_t nesting = 0;
	brackets = 0;

	while(instr) {
		switch(instr -> opcode) {
		case BFI_INSTR_INC:
			trans -> opcode = BFT_INSTR_INC;
			trans -> op1 = instr -> operand.value % 256;
			trans -> ad1 = offset;
			goto next;

		case BFI_INSTR_DEC:
			trans -> opcode = BFT_INSTR_DEC;
			trans -> op1 = instr -> operand.value % 256;
			trans -> ad1 = offset;
			goto next;

		case BFI_INSTR_FWD:
			offset += instr -> operand.value;
			goto end;

		case BFI_INSTR_BCK:
			offset -= instr -> operand.value;
			goto end;

		case BFI_INSTR_INP:
			trans -> opcode = BFT_INSTR_INP;
			trans -> ad1 = offset;
			goto next;

		case BFI_INSTR_OUT:
			trans -> opcode = BFT_INSTR_OUT;
			trans -> ad1 = offset;
		
		next:	trans -> next = malloc(sizeof(BFt_instr_t));
			if(!trans -> next) BFe_report_err(BFE_UNKNOWN_ERROR);

			trans -> next -> prev = trans;
			trans = trans -> next;
			trans -> opcode = BFT_INSTR_NOP;

		end:	instr = instr -> next;
			continue;
		}

		if(offset > 0) {
			trans -> opcode = BFT_INSTR_FWD;
			trans -> op1 = offset;
		}

		else if(offset < 0) {
			trans -> opcode = BFT_INSTR_BCK;
			trans -> op1 = -offset;
		}

		if(offset) {
			trans -> next = malloc(sizeof(BFt_instr_t));
			if(!trans -> next) BFe_report_err(BFE_UNKNOWN_ERROR);

			trans -> next -> prev = trans;
			trans = trans -> next;
			trans -> opcode = BFT_INSTR_NOP;
			offset = 0;
		}

		switch(instr -> opcode) {
		case BFI_INSTR_JZ:
			trans -> opcode = BFT_INSTR_LOOP;
			trans -> op1 = ++brackets;
			stack[nesting++] = brackets;
			break;

		case BFI_INSTR_JMP:
			trans -> opcode = BFT_INSTR_ENDL;
			trans -> op1 = stack[--nesting];
			break;

		default:
			instr = instr -> next;
			continue;
		}

		trans -> next = malloc(sizeof(BFt_instr_t));
		if(!trans -> next) BFe_report_err(BFE_UNKNOWN_ERROR);

		trans -> next -> prev = trans;
		instr = instr -> next;

		trans = trans -> next;
		trans -> opcode = BFT_INSTR_NOP;
	}

	free(stack);

	trans -> next = NULL;
	return first;
}

static BFt_instr_t *optimise_1() {
	BFt_instr_t *start = optimise_0();

	BFt_instr_t *init = NULL;
	size_t per_cycle = 0;

	for(BFt_instr_t *i = start; i; i = i -> next) {
		size_t op1 = i -> op1;
		ssize_t ad1 = i -> ad1;

		switch(i -> opcode) {
		case BFT_INSTR_INC:
			if(!ad1) per_cycle += op1;
			break;

		case BFT_INSTR_DEC:
			if(!ad1) per_cycle -= op1;
			break;

		case BFT_INSTR_FWD:
		case BFT_INSTR_BCK:
		case BFT_INSTR_INP:
		case BFT_INSTR_OUT:
			init = NULL; break;

		case BFT_INSTR_LOOP:
			per_cycle = 0;
			init = i;
			break;

		case BFT_INSTR_ENDL:
			if(init) switch(per_cycle) {
				case 1: conv_loop(init, i, true); break;
				case -1: conv_loop(init, i, false);
			}

			init = NULL;
			break;
		}
	}

	for(BFt_instr_t *i = start; i; i = i -> next) {
		switch(i -> opcode) {
		case BFT_INSTR_MOV:
			if(i -> next -> ad1) continue;

			switch(i -> next -> opcode) {
			case BFT_INSTR_INC:
				i -> op1 = i -> next -> op1;
				break;

			case BFT_INSTR_DEC:
				i -> op1 = 256 - i -> next -> op1;
				break;

			default:
				continue;
			}

			BFt_instr_t *next = i -> next -> next;
			free(i -> next);

			i -> next = next;
			next -> prev = i;
		}
	}

	return start;
}

static void conv_loop(BFt_instr_t *start, BFt_instr_t *end, bool compl) {
	BFt_instr_t *new = malloc(sizeof(BFt_instr_t));
	if(!new) BFe_report_err(BFE_UNKNOWN_ERROR);

	BFt_instr_t *start_new = new;
	new -> op1 = 0; new -> op2 = 0;

	if(compl) new -> opcode = BFT_INSTR_CMPL;
	else new -> opcode = BFT_INSTR_NOP;

	for(BFt_instr_t *i = start -> next; i != end; i = i -> next)
		if(i -> ad1) new = conv_instr(start_new, new, i);

	for(BFt_instr_t *i = start -> next; i != end;) {
		BFt_instr_t *instr = i;
		i = instr -> next;
		free(instr);
	}

	if(new -> opcode == BFT_INSTR_NOP || new -> opcode == BFT_INSTR_CMPL) {
		start -> next = end;
		end -> prev = start;

		start -> opcode = BFT_INSTR_NOP;
		end -> opcode = BFT_INSTR_MOV;
		end -> ad1 = 0; end -> op1 = 0;

		free(new);
		return;
	}

	new -> next = malloc(sizeof(BFt_instr_t));
	if(!new -> next) BFe_report_err(BFE_UNKNOWN_ERROR);

	new -> next -> prev = new;
	new = new -> next;

	start -> opcode = BFT_INSTR_IFNZ;
	new -> opcode = BFT_INSTR_ENDIF;
	new -> op1 = end -> op1;

	end -> opcode = BFT_INSTR_MOV;
	end -> op1 = 0; end -> ad1 = 0;

	start -> next = start_new;
	end -> prev = new;

	start_new -> prev = start;
	new -> next = end;
}

static BFt_instr_t *conv_instr(BFt_instr_t *start, BFt_instr_t *end,
			       BFt_instr_t *instr)
{
	size_t op = instr -> op1;
	ssize_t ad = instr -> ad1;

	int opcode = BFT_INSTR_NOP;
	size_t op1 = 0, op2 = 0;

	BFt_instr_t *i;
	switch(instr -> opcode) {
	case BFT_INSTR_INC:
		if(IS_2_POW(op) && op != 1) {
			for(i = start; i -> op2 != op && i != end;
				i = i -> next);

			opcode = BFT_INSTR_SHLA;
			op2 = log_b2(op); op1 = 0;
			break;
		}

		for(i = start; i -> op1 != op && i != end; i = i -> next);

		opcode = op == 1 ? BFT_INSTR_CPYA : BFT_INSTR_MULA;
		op1 = op; op2 = 0;
		break;

	case BFT_INSTR_DEC:
		if(IS_2_POW(op) && op != 1) {
			for(i = start; i -> op2 != op && i != end;
				i = i -> next);

			opcode = BFT_INSTR_SHLS;
			op2 = log_b2(op); op1 = 0;
			break;
		}

		for(i = start; i -> op1 != op && i != end; i = i -> next);

		opcode = op == 1 ? BFT_INSTR_CPYS : BFT_INSTR_MULS;
		op1 = op; op2 = 0;
		break;

	default:
		return end;
	}

	BFt_instr_t *new = malloc(sizeof(BFt_instr_t));
	if(!new) BFe_report_err(BFE_UNKNOWN_ERROR);

	new -> prev = i; new -> next = i -> next;
	new -> opcode = opcode; new -> ad1 = ad;
	new -> op1 = op1; new -> op2 = op2; 

	if(i -> next) i -> next -> prev = new;
	i -> next = new;

	if(i == end) return new;
	else return end;
}

static void translate(FILE *file) {
	static char line[BF_LINE_SIZE];
	BFt_instr_t *instr = BFt_code;
	size_t chars = 8;


	fprintf(file, "\t#define VAR unsigned char\n");
	fprintf(file, "\t#define INP getchar\n");
	fprintf(file, "\t#define OUT putchar\n\n");

	fprintf(file, "\tVAR *ptr = &cells[0];\n\n\t");

	while(instr) {
		size_t op1 = instr -> op1;
		size_t op2 = instr -> op2;
		ssize_t ad1 = instr -> ad1;
		size_t offset = 0;

		switch(instr -> opcode) {
		case BFT_INSTR_INC:
			chars += ad1
				? sprintf(line, "ptr[%zd] += %zu; ", ad1, op1)
				: sprintf(line, "*ptr += %zu; ", op1);
			break;

		case BFT_INSTR_DEC:
			chars += ad1
				? sprintf(line, "ptr[%zd] -= %zu; ", ad1, op1)
				: sprintf(line, "*ptr -= %zu; ", op1);
			break;

		case BFT_INSTR_CMPL:
			chars += sprintf(line, "*ptr = -*ptr; ");
			break;

		case BFT_INSTR_MOV:
			chars += ad1
				? sprintf(line, "ptr[%zd] = %zu; ", ad1, op1)
				: sprintf(line, "*ptr = %zu; ", op1);
			break;

		case BFT_INSTR_FWD:
			chars += sprintf(line, "ptr += %zu; ", op1);
			break;

		case BFT_INSTR_BCK:
			chars += sprintf(line, "ptr -= %zu; ", op1);
			break;

		case BFT_INSTR_INP:
			chars += ad1 ? sprintf(line, "ptr[%zd] = INP(); ", ad1)
				: sprintf(line, "*ptr = INP(); ");
			break;

		case BFT_INSTR_OUT:
			chars += ad1 ? sprintf(line, "OUT(ptr[%zd]); ", ad1)
				: sprintf(line, "OUT(*ptr); ");
			break;

		case BFT_INSTR_LOOP:
			chars += sprintf(line, "while(*ptr) { ");
			break;

		case BFT_INSTR_ENDL:
			chars += sprintf(line, "} ");
			break;

		case BFT_INSTR_IFNZ:
			chars += sprintf(line, "if(*ptr) { VAR ");
			break;

		case BFT_INSTR_ENDIF:
			chars += sprintf(line, "} ");
			break;

		case BFT_INSTR_MULA:
			if(instr -> prev -> opcode != BFT_INSTR_MULA
				|| instr -> prev -> op1 != op1)
			{
				chars += offset = sprintf(line,
					"acc = *ptr * %zu; ", op1);
			}

			goto add;

		case BFT_INSTR_MULS:
			if(instr -> prev -> opcode != BFT_INSTR_MULS
				|| instr -> prev -> op1 != op1)
			{
				chars += offset = sprintf(line,
					"acc = *ptr * %zu; ", op1);
			}

			goto sub;

		case BFT_INSTR_SHLA:
			if(instr -> prev -> opcode != BFT_INSTR_SHLA
				|| instr -> prev -> op2 != op2)
			{
				chars += offset = sprintf(line,
					"acc = *ptr << %zu; ", op2);
			}

			goto add;

		case BFT_INSTR_SHLS:
			if(instr -> prev -> opcode != BFT_INSTR_SHLS
				|| instr -> prev -> op2 != op2)
			{
				chars += offset = sprintf(line,
					"acc = *ptr << %zu; ", op2);
			}

			goto sub;

		case BFT_INSTR_CPYA:
			if(instr -> prev -> opcode != BFT_INSTR_CPYA
				|| instr -> prev -> op1 != op1)
			{
				chars += offset = sprintf(line,
					"acc = *ptr; ");
			}

			goto add;

		case BFT_INSTR_CPYS:
			if(instr -> prev -> opcode != BFT_INSTR_CPYS
				|| instr -> prev -> op1 != op1)
			{
				chars += offset = sprintf(line,
					"acc = *ptr; ");
			}

			goto sub;

		add:	chars += sprintf(line + offset, "ptr[%zd] += acc; ", ad1);
			break;

		sub:	chars += sprintf(line + offset, "ptr[%zd] -= acc; ", ad1);
			break;

		default:
			instr = instr -> next;
			continue;
		}

		if(chars >= 80) chars = fprintf(file, "\n\t%s", line) + 6;
		else fputs(line, file);
		instr = instr -> next;
	}

	fputc('\n', file);
}

static void conv_standalone() {
	if(BFc_direct_inp) {
		BFe_code_error = "--standalone is incompatible with --direct-inp.";
		BFe_report_err(BFE_INCOMPATIBLE_ARGS);
		exit(BFE_INCOMPATIBLE_ARGS);
	}

	if(!strlen(BFf_outfile_name)) {
		strcpy(BFf_outfile_name, BFf_mainfile_name);
		strncat(BFf_outfile_name, ".s",
			BF_FILENAME_SIZE - 1 - strlen(BFf_outfile_name));
	}

	FILE *file = fopen(BFf_outfile_name, "w");

	if(!file) {
		BFe_file_name = BFf_outfile_name;
		BFe_report_err(BFE_FILE_UNWRITABLE);
		exit(BFE_FILE_UNWRITABLE);
	}

	BFt_optimise();
	if(!strcmp(BFt_target_arch, "amd64")) BFa_amd64_tasm(file);
	else if(!strcmp(BFt_target_arch, "i386")) BFa_i386_tasm(file);
	else { BFe_report_err(BFE_BAD_ARCH); exit(BFE_BAD_ARCH); }

	int ret = fclose(file);
	if(ret == EOF) BFe_report_err(BFE_UNKNOWN_ERROR);
	exit(0);
}