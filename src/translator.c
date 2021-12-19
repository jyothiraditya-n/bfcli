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
#include <stdlib.h>
#include <string.h>

#include "clidata.h"
#include "errors.h"
#include "files.h"
#include "interpreter.h"
#include "main.h"
#include "printing.h"
#include "translator.h"

bool BFt_compile;
bool BFt_translate;
int BFt_optim_lvl;

BFt_instr_t *BFt_code;

static BFt_instr_t *optimise_0();
static void translate_0(FILE *file);
static void trans_amd64_0(FILE *file);

void BFt_convert_file() {
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

	fprintf(file, "char cells[%zu];\n\n", BFi_mem_size);

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

	BFt_optimise();
	switch(BFt_optim_lvl) {
		case 0:
			if(BFt_compile) trans_amd64_0(file);
			else translate_0(file);
			break;

		default:
			BFe_report_err(BFE_BAD_OPTIM);
			exit(BFE_BAD_OPTIM);
	}

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
	trans -> next = NULL;
	trans -> opcode = BFT_INSTR_NOP;

	size_t brackets = 0;
	while(instr) {
		switch(instr -> opcode) { case BFI_INSTR_JMP: brackets++; }
		instr = instr -> next;
	}

	size_t *stack = malloc(sizeof(size_t) * brackets);
	if(!stack) BFe_report_err(BFE_UNKNOWN_ERROR);

	size_t nesting = 0;
	brackets = 0;
	instr = BFi_program_code;

	while(instr) {
		switch(instr -> opcode) {
		case BFI_INSTR_INC:
			trans -> opcode = BFT_INSTR_INC;
			trans -> op1 = instr -> operand.value;
			break;

		case BFI_INSTR_DEC:
			trans -> opcode = BFT_INSTR_DEC;
			trans -> op1 = instr -> operand.value;
			break;

		case BFI_INSTR_FWD:
			trans -> opcode = BFT_INSTR_FWD;
			trans -> op1 = instr -> operand.value;
			break;

		case BFI_INSTR_BCK:
			trans -> opcode = BFT_INSTR_BCK;
			trans -> op1 = instr -> operand.value;
			break;

		case BFI_INSTR_INP:
			trans -> opcode = BFT_INSTR_INP;
			break;

		case BFI_INSTR_OUT:
			trans -> opcode = BFT_INSTR_OUT;
			break;

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
		trans -> opcode = BFI_INSTR_NOP;
	}

	free(stack);
	return first;
}

static void translate_0(FILE *file) {
	static char line[BF_LINE_SIZE];
	BFt_instr_t *instr = BFt_code;
	size_t chars = 8;

	fprintf(file, "\tchar *ptr = &cells[0];\n\n\t");

	while(instr) {
		switch(instr -> opcode) {
		case BFT_INSTR_INC:
			chars += sprintf(line, "*ptr += %zu; ", instr -> op1);
			break;

		case BFT_INSTR_DEC:
			chars += sprintf(line, "*ptr -= %zu; ", instr -> op1);
			break;

		case BFT_INSTR_FWD:
			chars += sprintf(line, "ptr += %zu; ", instr -> op1);
			break;

		case BFT_INSTR_BCK:
			chars += sprintf(line, "ptr -= %zu; ", instr -> op1);
			break;

		case BFT_INSTR_INP:
			chars += sprintf(line, "*ptr = getchar(); ");
			break;

		case BFT_INSTR_OUT:
			chars += sprintf(line, "putchar(*ptr); ");
			break;

		case BFT_INSTR_LOOP:
			chars += sprintf(line, "while(*ptr) { ");
			break;

		case BFT_INSTR_ENDL:
			chars += sprintf(line, "} ");
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

static void trans_amd64_0(FILE *file) {
	BFt_instr_t *instr = BFt_code;

	fprintf(file, "\tasm volatile ( \"AMD64_ASSEMBLY:\\n\"\n");
	fprintf(file, "\t\"\tmovq\t%%0, %%%%rax\\n\"\n");

	while(instr) {
		switch(instr -> opcode) {
		case BFT_INSTR_INC:
			fprintf(file, "\t\"\taddb\t$0x%zx, (%%%%rax)\\n\"\n",
				instr -> op1);
			break;	

		case BFT_INSTR_DEC:
			fprintf(file, "\t\"\tsubb\t$0x%zx, (%%%%rax)\\n\"\n",
				instr -> op1);
			break;

		case BFT_INSTR_FWD:
			fprintf(file, "\t\"\tadd\t$0x%zx, %%%%rax\\n\"\n",
				instr -> op1);
			break;

		case BFT_INSTR_BCK:
			fprintf(file, "\t\"\tsub\t$0x%zx, %%%%rax\\n\"\n",
				instr -> op1);
			break;

		case BFT_INSTR_INP:
			fprintf(file, "\t\"\tpushq\t%%%%rax\\n\"\n");
			fprintf(file, "\t\"\tmovq\t%%%%rax, %%%%rsi\\n\"\n");
			fprintf(file, "\t\"\tmovq\t$0x0, %%%%rax\\n\"\n");
			fprintf(file, "\t\"\tmovq\t$0x0, %%%%rdi\\n\"\n");
			fprintf(file, "\t\"\tmovq\t$0x1, %%%%rdx\\n\"\n");
			fprintf(file, "\t\"\tsyscall\\n\"\n");
			fprintf(file, "\t\"\tpopq\t%%%%rax\\n\"\n");
			break;

		case BFT_INSTR_OUT:
			fprintf(file, "\t\"\tpushq\t%%%%rax\\n\"\n");
			fprintf(file, "\t\"\tmovq\t%%%%rax, %%%%rsi\\n\"\n");
			fprintf(file, "\t\"\tmovq\t$0x1, %%%%rax\\n\"\n");
			fprintf(file, "\t\"\tmovq\t$0x1, %%%%rdi\\n\"\n");
			fprintf(file, "\t\"\tmovq\t$0x1, %%%%rdx\\n\"\n");
			fprintf(file, "\t\"\tsyscall\\n\"\n");
			fprintf(file, "\t\"\tpopq\t%%%%rax\\n\"\n");
			break;

		case BFT_INSTR_LOOP:
			fprintf(file, "\n\t\".L%zu:\\n\"\n", instr -> op1);
			fprintf(file, "\t\"\tcmpb\t$0x0, (%%%%rax)\\n\"\n");
			fprintf(file, "\t\"\tje\t.E%zu\\n\"\n", instr -> op1);
			break;

		case BFT_INSTR_ENDL:
			fprintf(file, "\t\"\tjmp\t.L%zu\\n\"\n", instr -> op1);
			fprintf(file, "\n\t\".E%zu:\\n\"\n", instr -> op1);
			break;

		default:
			instr = instr -> next;
			continue;
		}

		instr = instr -> next;
	}

	fprintf(file, "\n\t: : \"r\" (&cells[0])\n");
	fprintf(file, "\t: \"rdi\", \"rsi\", \"rax\",\n");
	fprintf(file, "\t\"rbx\", \"rcx\", \"rdx\");\n\n");

	fprintf(file, "\treturn 0;\n");
}