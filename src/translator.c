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

#include "arch.h"
#include "clidata.h"
#include "errors.h"
#include "files.h"
#include "interpreter.h"
#include "main.h"
#include "optims.h"
#include "translator.h"

bool BFt_compile;
bool BFt_translate;
bool BFt_standalone;

BFi_instr_t *BFi_code;

static void translate(FILE *file);

void BFt_translate_c() {
	if(BFt_standalone) BFa_translate();

	BFo_optimise();
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

	fprintf(file, "unsigned char cells[%zu];\n\n",
			BFi_mem_size + BFo_mem_padding);

	if(!BFt_compile) {
		fprintf(file, "#define INP getchar\n");
		fprintf(file, "#define OUT putchar\n\n");

		fprintf(file, "unsigned char *ptr = &cells[%zu];\n",
			BFo_mem_padding);

		fprintf(file, "unsigned char acc = 0;\n\n");
	}

	static char line[BF_LINE_SIZE];
	size_t chars = 0;

	for(size_t i = 1; i < BFo_sub_count; i++) {
		chars += sprintf(line, "void sub_%zu(); ", i);
		if(chars >= 80) chars = fprintf(file, "\n%s", line) - 1;
		else fputs(line, file);
	}

	if(BFo_sub_count > 1) fputs("\n\n", file);
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

	if(BFt_compile) BFa_translate_c(file);
	else translate(file);
	fputs("}\n", file);

	int ret = fclose(file);
	if(ret == EOF) BFe_report_err(BFE_UNKNOWN_ERROR);
	exit(0);
}

static void translate(FILE *file) {
	static char line[BF_LINE_SIZE];
	size_t chars = 8;
	fputc('\t', file);

	for(BFi_instr_t *instr = BFi_code; instr; instr = instr -> next) {
		size_t op1 = instr -> op1;
		size_t op2 = instr -> op2;
		ssize_t ad = instr -> ad;
		size_t offset = 0;

		switch(instr -> opcode) {
		case BFI_INSTR_INC:
			chars += ad
				? sprintf(line, "ptr[%zd] += %zu; ", ad, op1)
				: sprintf(line, "*ptr += %zu; ", op1);
			break;

		case BFI_INSTR_DEC:
			chars += ad
				? sprintf(line, "ptr[%zd] -= %zu; ", ad, op1)
				: sprintf(line, "*ptr -= %zu; ", op1);
			break;

		case BFI_INSTR_CMPL:
			chars += sprintf(line, "*ptr = -*ptr; ");
			break;

		case BFI_INSTR_MOV:
			chars += ad
				? sprintf(line, "ptr[%zd] = %zu; ", ad, op1)
				: sprintf(line, "*ptr = %zu; ", op1);
			break;

		case BFI_INSTR_FWD:
			chars += sprintf(line, "ptr += %zu; ", op1);
			break;

		case BFI_INSTR_BCK:
			chars += sprintf(line, "ptr -= %zu; ", op1);
			break;

		case BFI_INSTR_INP:
			chars += ad ? sprintf(line, "ptr[%zd] = INP(); ", ad)
				: sprintf(line, "*ptr = INP(); ");
			break;

		case BFI_INSTR_OUT:
			chars += ad ? sprintf(line, "OUT(ptr[%zd]); ", ad)
				: sprintf(line, "OUT(*ptr); ");
			break;

		case BFI_INSTR_LOOP:
			chars += sprintf(line, "while(*ptr) { ");
			break;

		case BFI_INSTR_ENDL:
			chars += sprintf(line, "} ");
			break;

		case BFI_INSTR_IFNZ:
			chars += sprintf(line, "if(*ptr) { ");
			break;

		case BFI_INSTR_ENDIF:
			chars += sprintf(line, "} ");
			break;

		case BFI_INSTR_MULA:
			if(instr -> prev -> opcode != BFI_INSTR_MULA
				|| instr -> prev -> op1 != op1)
			{
				chars += offset = sprintf(line,
					"acc = *ptr * %zu; ", op1);
			}

			goto add;

		case BFI_INSTR_MULS:
			if(instr -> prev -> opcode != BFI_INSTR_MULS
				|| instr -> prev -> op1 != op1)
			{
				chars += offset = sprintf(line,
					"acc = *ptr * %zu; ", op1);
			}

			goto sub;

		case BFI_INSTR_SHLA:
			if(instr -> prev -> opcode != BFI_INSTR_SHLA
				|| instr -> prev -> op2 != op2)
			{
				chars += offset = sprintf(line,
					"acc = *ptr << %zu; ", op2);
			}

			goto add;

		case BFI_INSTR_SHLS:
			if(instr -> prev -> opcode != BFI_INSTR_SHLS
				|| instr -> prev -> op2 != op2)
			{
				chars += offset = sprintf(line,
					"acc = *ptr << %zu; ", op2);
			}

			goto sub;

		case BFI_INSTR_CPYA:
			if(instr -> prev -> opcode != BFI_INSTR_CPYA
				|| instr -> prev -> op1 != op1)
			{
				chars += offset = sprintf(line,
					"acc = *ptr; ");
			}

			goto add;

		case BFI_INSTR_CPYS:
			if(instr -> prev -> opcode != BFI_INSTR_CPYS
				|| instr -> prev -> op1 != op1)
			{
				chars += offset = sprintf(line,
					"acc = *ptr; ");
			}

			goto sub;

		add:	chars += sprintf(line + offset, "ptr[%zd] += acc; ", ad);
			break;

		sub:	chars += sprintf(line + offset, "ptr[%zd] -= acc; ", ad);
			break;

		case BFI_INSTR_SUB:
			fprintf(file, "\n}\n\nvoid sub_%zu() {\n\t", op1);
			chars = 8;
			continue;

		case BFI_INSTR_JSR:
			chars += sprintf(line, "sub_%zu(); ", op1);
			break;

		case BFI_INSTR_RTS:
		case BFI_INSTR_RET:
			line[0] = 0;
			break;

		default:
			continue;
		}

		if(chars >= 80) chars = fprintf(file, "\n\t%s", line) + 6;
		else fputs(line, file);
	}

	fputc('\n', file);
}