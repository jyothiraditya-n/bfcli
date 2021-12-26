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

#include "clidata.h"
#include "errors.h"
#include "files.h"
#include "interpreter.h"
#include "main.h"
#include "standalone.h"
#include "translator.h"

bool BFt_compile;
bool BFt_translate;
bool BFt_standalone;
int BFt_optim_lvl;

BFt_instr_t *BFt_code;

static BFt_instr_t *optimise_0();
static BFt_instr_t *optimise_1();

static void _optimise_1(BFt_instr_t *start, BFt_instr_t *end, bool compl);

static void translate(FILE *file);
static void trans_amd64(FILE *file);

void BFt_convert_file() {
	if(BFt_standalone) BFs_convert_file();

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

	if(BFt_compile) trans_amd64(file);
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
	trans -> next = NULL;
	trans -> opcode = BFT_INSTR_NOP;

	size_t brackets = 0;
	ssize_t offset = 0;

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
			trans -> ad1 = offset;
			goto next;

		case BFI_INSTR_DEC:
			trans -> opcode = BFT_INSTR_DEC;
			trans -> op1 = instr -> operand.value;
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

		if(offset != 0) {
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
				case 1: _optimise_1(init, i, true); break;
				case -1: _optimise_1(init, i, false);
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
				i -> op1 = -(i -> next -> op1);
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

static void _optimise_1(BFt_instr_t *start, BFt_instr_t *end, bool compl) {
	BFt_instr_t *new = malloc(sizeof(BFt_instr_t));
	if(!new) BFe_report_err(BFE_UNKNOWN_ERROR);

	BFt_instr_t *start_new = new;

	if(compl) start -> opcode = BFT_INSTR_CMPL;
	else start -> opcode = BFT_INSTR_NOP;

	for(BFt_instr_t *i = start -> next; i != end; i = i -> next) {
		size_t op1 = i -> op1;
		ssize_t ad1 = i -> ad1;

		if(!ad1) continue;

		switch(i -> opcode) {
		case BFT_INSTR_INC:
			new -> opcode = BFT_INSTR_ADDM;
			new -> op1 = op1; new -> ad1 = ad1;
			break;

		case BFT_INSTR_DEC:
			new -> opcode = BFT_INSTR_SUBM;
			new -> op1 = op1; new -> ad1 = ad1;
			break;

		default:
			continue;
		}

		new -> next = malloc(sizeof(BFt_instr_t));
		if(!new -> next) BFe_report_err(BFE_UNKNOWN_ERROR);

		new -> next -> prev = new;
		new = new -> next;
	}

	new -> opcode = BFT_INSTR_NOP;

	end -> opcode = BFT_INSTR_MOV;
	end -> op1 = 0; end -> ad1 = 0;

	for(BFt_instr_t *i = start -> next; i != end;) {
		BFt_instr_t *instr = i;
		i = instr -> next;
		free(instr);
	}

	start -> next = start_new;
	end -> prev = new;

	start_new -> prev = start;
	new -> next = end;
}

static void translate(FILE *file) {
	static char line[BF_LINE_SIZE];
	BFt_instr_t *instr = BFt_code;
	size_t chars = 8;

	fprintf(file, "\tchar *ptr = &cells[0];\n");
	fprintf(file, "\tint (*inp)() = getchar;\n");
	fprintf(file, "\tint (*out)(int) = putchar;\n\n\t");

	while(instr) {
		size_t op1 = instr -> op1;
		ssize_t ad1 = instr -> ad1;

		switch(instr -> opcode) {
		case BFT_INSTR_MOV:
			chars += ad1
				? sprintf(line, "ptr[%zd] = %zu; ", ad1, op1)
				: sprintf(line, "*ptr = %zu; ", op1);
			break;

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

		case BFT_INSTR_ADDM:
			chars += sprintf(line, "ptr[%zd] += *ptr * %zu; ",
				ad1, op1);
			break;

		case BFT_INSTR_SUBM:
			chars += sprintf(line, "ptr[%zd] -= *ptr * %zu; ",
				ad1, op1);
			break;

		case BFT_INSTR_CMPL:
			chars += sprintf(line, "*ptr = -*ptr; ");
			break;

		case BFT_INSTR_FWD:
			chars += sprintf(line, "ptr += %zu; ", op1);
			break;

		case BFT_INSTR_BCK:
			chars += sprintf(line, "ptr -= %zu; ", op1);
			break;

		case BFT_INSTR_INP:
			chars += ad1 ? sprintf(line, "ptr[%zd] = inp(); ", ad1)
				: sprintf(line, "*ptr = inp(); ");
			break;

		case BFT_INSTR_OUT:
			chars += ad1 ? sprintf(line, "out(ptr[%zd]); ", ad1)
				: sprintf(line, "out(*ptr); ");
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

static void trans_amd64(FILE *file) {
	BFt_instr_t *instr = BFt_code;
	int last_io_instr = BFT_INSTR_NOP;
	bool regs_dirty = true;

	fprintf(file, "\tasm volatile ( \"AMD64_ASSEMBLY:\\n\"\n");
	fprintf(file, "\t\"\tmov\t%%0, %%%%rbx\\n\"\n");

	while(instr) {
		size_t op1 = instr -> op1;
		ssize_t ad1 = instr -> ad1;

		switch(instr -> opcode) {
		case BFT_INSTR_MOV:
			fprintf(file, "\t\"\tmovb\t$%zu, ", op1);
			if(ad1) fprintf(file, "%zd(%%%%rbx)\\n\"\n", ad1);
			else fprintf(file, "(%%%%rbx)\\n\"\n");
			break;

		case BFT_INSTR_INC:
			if(op1 == 1) fprintf(file, "\t\"\tincb\t");
			else fprintf(file, "\t\"\taddb\t$%zu, ", op1);

			if(ad1) fprintf(file, "%zd(%%%%rbx)\\n\"\n", ad1);
			else fprintf(file, "(%%%%rbx)\\n\"\n");
			break;

		case BFT_INSTR_DEC:
			if(op1 == 1) fprintf(file, "\t\"\tdecb\t");
			else fprintf(file, "\t\"\tsubb\t$%zu, ", op1);

			if(ad1) fprintf(file, "%zd(%%%%rbx)\\n\"\n", ad1);
			else fprintf(file, "(%%%%rbx)\\n\"\n");
			break;

		case BFT_INSTR_ADDM:
			fprintf(file, "\t\"\tmov\t(%%%%rbx), %%%%al\\n\"\n");
			fprintf(file, "\t\"\tmov\t$%zu, %%%%rcx\\n\"\n", op1);
			fprintf(file, "\t\"\tmul\t%%%%cl\\n\"\n");
			fprintf(file, "\t\"\taddb\t%%%%al, %zd(%%%%rbx)\\n\"\n", ad1);
			
			regs_dirty = true;
			break;

		case BFT_INSTR_SUBM:
			fprintf(file, "\t\"\tmov\t(%%%%rbx), %%%%al\\n\"\n");
			fprintf(file, "\t\"\tmov\t$%zu, %%%%rcx\\n\"\n", op1);
			fprintf(file, "\t\"\tmul\t%%%%cl\\n\"\n");
			fprintf(file, "\t\"\tsubb\t%%%%al, %zd(%%%%rbx)\\n\"\n", ad1);
			
			regs_dirty = true;
			break;

		case BFT_INSTR_CMPL:
			fprintf(file, "\t\"\tnegb\t(%%%%rbx)\\n\"\n");
			break;

		case BFT_INSTR_FWD:
			if(op1 == 1) fprintf(file, "\t\"\tinc\t");
			else fprintf(file, "\t\"\tadd\t$%zu, ", op1);
			fprintf(file, "%%%%rbx\\n\"\n");
			break;

		case BFT_INSTR_BCK:
			if(op1 == 1) fprintf(file, "\t\"\tdec\t");
			else fprintf(file, "\t\"\tsub\t$%zu, ", op1);
			fprintf(file, "%%%%rbx\\n\"\n");
			break;

		case BFT_INSTR_INP:
			if(ad1) fprintf(file, "\t\"\tlea\t%zd", ad1);
			else fprintf(file, "\t\"\tlea\t");
			fprintf(file, "(%%%%rbx), %%%%rsi\\n\"\n");

			if(regs_dirty) fprintf(file, "\t\"\tmov\t$1, %%%%rdx\\n\"\n");
			else if(last_io_instr == BFT_INSTR_INP) goto in;

			fprintf(file, "\t\"\tmov\t$0, %%%%rax\\n\"\n");
			fprintf(file, "\t\"\tmov\t%%%%rax, %%%%rdi\\n\"\n");
		in:	fprintf(file, "\t\"\tsyscall\\n\"\n");

			last_io_instr = BFT_INSTR_INP;
			regs_dirty = false;
			break;

		case BFT_INSTR_OUT:
			if(ad1) fprintf(file, "\t\"\tlea\t%zd", ad1);
			else fprintf(file, "\t\"\tlea\t");
			fprintf(file, "(%%%%rbx), %%%%rsi\\n\"\n");

			if(regs_dirty) {
				fprintf(file, "\t\"\tmov\t$1, %%%%rdx\\n\"\n");
				fprintf(file, "\t\"\tmov\t%%%%rdx, %%%%rax\\n\"\n");
			}

			else {
				if(last_io_instr == BFT_INSTR_OUT) goto out;
				fprintf(file, "\t\"\tmov\t$1, %%%%rax\\n\"\n");
			}

			fprintf(file, "\t\"\tmov\t%%%%rax, %%%%rdi\\n\"\n");
		out:	fprintf(file, "\t\"\tsyscall\\n\"\n");

			last_io_instr = BFT_INSTR_OUT;
			regs_dirty = false;
			break;

		case BFT_INSTR_LOOP:
			fprintf(file, "\n\t\".L%zu:\\n\"\n", op1);
			fprintf(file, "\t\"\tcmpb\t$0, (%%%%rbx)\\n\"\n");
			fprintf(file, "\t\"\tje\t.LE%zu\\n\"\n", op1);

			if(BFt_optim_lvl >= 1) regs_dirty = true;
			break;

		case BFT_INSTR_ENDL:
			fprintf(file, "\t\"\tjmp\t.L%zu\\n\"\n", op1);
			fprintf(file, "\n\t\".LE%zu:\\n\"\n", op1);

			if(BFt_optim_lvl >= 1) regs_dirty = true;
			break;
		}

		instr = instr -> next;
	}

	fprintf(file, "\n\t: : \"r\" (&cells[0])\n");
	fprintf(file, "\t: \"rax\", \"rbx\", \"rcx\", \"rdx\", \"rdi\",\n"
		"\t\"rsi\", \"r8\", \"r9\", \"r10\", \"r11\");\n\n");

	fprintf(file, "\treturn 0;\n");
}