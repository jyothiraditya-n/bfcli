/* Bfcli: The Interactive Brainfuck Command-Line Interpreter
 * Copyright (C) 2021-2022 Jyothiraditya Nellakra
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHout
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
		size_t len = strlen(BFf_outfile_name);

		for(size_t i = len; i <= len; i--)
			if(BFf_outfile_name[i] == '.') BFf_outfile_name[i] = 0;
		
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

	fprintf(file, "unsigned char cells[%zu]",
		BFi_mem_size + BFo_mem_padding);

	if(!BFt_compile) {
		if(!BFo_mem_padding) fprintf(file, ", *p = cells");
		else fprintf(file, ", *p = &cells[%zu]", BFo_mem_padding);

		if(BFo_advanced_ops) fprintf(file, ", a = 0");
	}

	fprintf(file, ";\n\n");

	if(!BFt_compile) {
		fprintf(file, "#define in getchar\n");
		fprintf(file, "#define out putchar\n\n");
	}

	static char line[BF_LINE_SIZE];
	size_t chars = 0;

	for(size_t i = 1; i < BFo_sub_count; i++) {
		chars += sprintf(line, i == 1? "void _%zu()": "_%zu()", i) + 2;

		if(chars > 80) chars = fprintf(file, ",\n%s", line) - 1;
		else fprintf(file, i == 1? "%s" : ", %s", line);
	}

	if(BFo_sub_count > 1) fputs(";\n\n", file);
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
	char mode = 'a';

	fputc('\t', file);

	for(BFi_instr_t *instr = BFi_code; instr; instr = instr -> next) {
		size_t op1 = instr -> op1, op2 = instr -> op2;
		ssize_t ad1 = instr -> ad1, ad2 = instr -> ad2;

		switch(instr -> opcode) {
		case BFI_INSTR_INC:
			chars += ad1
				? sprintf(line, "p[%zd] += %zu; ", ad1, op1)
				: sprintf(line, "*p += %zu; ", op1);
			break;

		case BFI_INSTR_DEC:
			chars += ad1
				? sprintf(line, "p[%zd] -= %zu; ", ad1, op1)
				: sprintf(line, "*p -= %zu; ", op1);
			break;

		case BFI_INSTR_CMPL:
			chars += sprintf(line, "*p = -*p; ");
			break;

		case BFI_INSTR_MOV:
			chars += ad1
				? sprintf(line, "p[%zd] = %zu; ", ad1, op1)
				: sprintf(line, "*p = %zu; ", op1);
			break;

		case BFI_INSTR_FWD:
			chars += sprintf(line, "p += %zu; ", op1);
			break;

		case BFI_INSTR_BCK:
			chars += sprintf(line, "p -= %zu; ", op1);
			break;

		case BFI_INSTR_INP:
			chars += ad1 ? sprintf(line, "p[%zd] = in(); ", ad1)
				: sprintf(line, "*p = in(); ");
			break;

		case BFI_INSTR_OUT:
			chars += ad1 ? sprintf(line, "out(p[%zd]); ", ad1)
				: sprintf(line, "out(*p); ");
			break;

		case BFI_INSTR_LOOP:
			chars += sprintf(line, "while(*p) { ");
			break;

		case BFI_INSTR_ENDL:
			chars += sprintf(line, "} ");
			break;

		case BFI_INSTR_IFNZ:
			chars += sprintf(line, "if(*p) { ");
			break;

		case BFI_INSTR_ENDIF:
			chars += sprintf(line, "} ");
			break;

		case BFI_INSTR_MULA:
			if(instr -> prev -> opcode != BFI_INSTR_MULA
				|| instr -> prev -> op1 != op1)
			{
				if(ad2) chars += sprintf(line,
					"a = p[%zd] * %zu; ", ad2, op1);

				else chars += sprintf(line,
					"a = *p * %zu; ", op1);
			}

			else line[0] = 0;

			mode = 'a';
			goto next;

		case BFI_INSTR_MULS:
			if(instr -> prev -> opcode != BFI_INSTR_MULS
				|| instr -> prev -> op1 != op1)
			{
				if(ad2) chars += sprintf(line,
					"a = p[%zd] * %zu; ", ad2, op1);
					
				else chars += sprintf(line,
					"a = *p * %zu; ", op1);
			}

			else line[0] = 0;

			mode = 's';
			goto next;

		case BFI_INSTR_MULM:
			if(instr -> prev -> opcode != BFI_INSTR_MULM
				|| instr -> prev -> op1 != op1)
			{
				if(ad2) chars += sprintf(line,
					"a = p[%zd] * %zu; ", ad2, op1);
					
				else chars += sprintf(line,
					"a = *p * %zu; ", op1);
			}

			else line[0] = 0;

			mode = 'm';
			goto next;

		case BFI_INSTR_SHLA:
			if(instr -> prev -> opcode != BFI_INSTR_SHLA
				|| instr -> prev -> op2 != op2)
			{
				if(ad2) chars += sprintf(line,
					"a = p[%zd] << %zu; ", ad2, op2);
					
				else chars += sprintf(line,
					"a = *p << %zu; ", op2);
			}

			else line[0] = 0;

			mode = 'a';
			goto next;

		case BFI_INSTR_SHLS:
			if(instr -> prev -> opcode != BFI_INSTR_SHLS
				|| instr -> prev -> op2 != op2)
			{
				if(ad2) chars += sprintf(line,
					"a = p[%zd] << %zu; ", ad2, op2);
					
				else chars += sprintf(line,
					"a = *p << %zu; ", op2);
			}

			else line[0] = 0;

			mode = 's';
			goto next;

		case BFI_INSTR_SHLM:
			if(instr -> prev -> opcode != BFI_INSTR_SHLM
				|| instr -> prev -> op2 != op2)
			{
				if(ad2) chars += sprintf(line,
					"a = p[%zd] << %zu; ", ad2, op2);
					
				else chars += sprintf(line,
					"a = *p << %zu; ", op2);
			}

			else line[0] = 0;

			mode = 'm';
			goto next;

		case BFI_INSTR_CPYA:
			if(instr -> prev -> opcode != BFI_INSTR_CPYA
				|| instr -> prev -> op1 != op1)
			{
				if(ad2) chars += sprintf(line,
					"a = p[%zd]; ", ad2);
					
				else chars += sprintf(line,
					"a = *p; ");
			}

			else line[0] = 0;

			mode = 'a';
			goto next;

		case BFI_INSTR_CPYS:
			if(instr -> prev -> opcode != BFI_INSTR_CPYS
				|| instr -> prev -> op1 != op1)
			{
				if(ad2) chars += sprintf(line,
					"a = p[%zd]; ", ad2);
					
				else chars += sprintf(line,
					"a = *p; ");
			}

			else line[0] = 0;

			mode = 's';
			goto next;

		case BFI_INSTR_CPYM:
			if(instr -> prev -> opcode != BFI_INSTR_CPYM
				|| instr -> prev -> op1 != op1)
			{
				if(ad2) chars += sprintf(line,
					"a = p[%zd]; ", ad2);
					
				else chars += sprintf(line,
					"a = *p; ");
			}

			else line[0] = 0;

			mode = 'm';

		next:	if(chars >= 80) chars = fprintf(file, "\n\t%s", line) + 6;
			else fputs(line, file);

			chars += sprintf(line, mode == 'a'? "p[%zd] += a; " :
				mode == 's'? "p[%zd] -= a; " :
				"p[%zd] = a; ", ad1);
			break;

		case BFI_INSTR_SUB:
			fprintf(file, "\n}\n\nvoid _%zu() {\n\t", op1);
			chars = 8;
			continue;

		case BFI_INSTR_JSR:
			chars += sprintf(line, "_%zu(); ", op1);
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