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
#include <stdlib.h>
#include <string.h>

#include "clidata.h"
#include "errors.h"
#include "files.h"
#include "interpreter.h"
#include "main.h"
#include "standalone.h"
#include "translator.h"

void BFs_convert_file() {
	if(BFc_direct_inp) {
		BFe_code_error = "--standalone is incompatible with --direct-inp.";
		BFe_report_err(BFE_INCOMPATIBLE_ARGS);
		exit(BFE_INCOMPATIBLE_ARGS);
	}

	BFt_optimise();
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

	fprintf(file, ".bss\n\n");
	fprintf(file, "\t.skip\t%zu\n\n", BFi_mem_size);
	fprintf(file, "cells:\n");
	fprintf(file, "\t.skip\t%zu\n\n", BFi_mem_size);

	fprintf(file, ".global\t_start\n");
	fprintf(file, ".text\n\n");

	fprintf(file, "_start:\n");
	fprintf(file, "\tmov\t$cells, %%rbx\n");

	BFt_instr_t *instr = BFt_code;
	int last_io_instr = BFT_INSTR_NOP;
	bool regs_dirty = true;

	while(instr) {
		size_t op1 = instr -> op1;
		ssize_t ad1 = instr -> ad1;

		switch(instr -> opcode) {
		case BFT_INSTR_MOV:
			fprintf(file, "\tmovb\t$%zu, ", op1);
			if(ad1) fprintf(file, "%zd(%%rbx)\n", ad1);
			else fprintf(file, "(%%rbx)\n");
			break;

		case BFT_INSTR_INC:
			if(op1 == 1) fprintf(file, "\tincb\t");
			else fprintf(file, "\taddb\t$%zu, ", op1);

			if(ad1) fprintf(file, "%zd(%%rbx)\n", ad1);
			else fprintf(file, "(%%rbx)\n");
			break;

		case BFT_INSTR_DEC:
			if(op1 == 1) fprintf(file, "\tdecb\t");
			else fprintf(file, "\tsubb\t$%zu, ", op1);

			if(ad1) fprintf(file, "%zd(%%rbx)\n", ad1);
			else fprintf(file, "(%%rbx)\n");
			break;

		case BFT_INSTR_ADDM:
			fprintf(file, "\tmov\t(%%rbx), %%al\n");
			fprintf(file, "\tmov\t$%zu, %%rcx\n", op1);
			fprintf(file, "\tmul\t%%cl\n");
			fprintf(file, "\taddb\t%%al, %zd(%%rbx)\n", ad1);
			
			regs_dirty = true;
			break;

		case BFT_INSTR_SUBM:
			fprintf(file, "\tmov\t(%%rbx), %%al\n");
			fprintf(file, "\tmov\t$%zu, %%rcx\n", op1);
			fprintf(file, "\tmul\t%%cl\n");
			fprintf(file, "\tsubb\t%%al, %zd(%%rbx)\n", ad1);
			
			regs_dirty = true;
			break;

		case BFT_INSTR_CMPL:
			fprintf(file, "\tnegb\t(%%rbx)\n");
			break;

		case BFT_INSTR_FWD:
			if(op1 == 1) fprintf(file, "\tinc\t");
			else fprintf(file, "\tadd\t$%zu, ", op1);
			fprintf(file, "%%rbx\n");
			break;

		case BFT_INSTR_BCK:
			if(op1 == 1) fprintf(file, "\tdec\t");
			else fprintf(file, "\tsub\t$%zu, ", op1);
			fprintf(file, "%%rbx\n");
			break;

		case BFT_INSTR_INP:
			if(ad1) fprintf(file, "\tlea\t%zd", ad1);
			else fprintf(file, "\tlea\t");
			fprintf(file, "(%%rbx), %%rsi\n");

			if(regs_dirty) fprintf(file, "\tmov\t$1, %%rdx\n");
			else if(last_io_instr == BFT_INSTR_INP) goto in;

			fprintf(file, "\tmov\t$0, %%rax\n");
			fprintf(file, "\tmov\t%%rax, %%rdi\n");
		in:     fprintf(file, "\tsyscall\n");

			last_io_instr = BFT_INSTR_INP;
			regs_dirty = false;
			break;

		case BFT_INSTR_OUT:
			if(ad1) fprintf(file, "\tlea\t%zd", ad1);
			else fprintf(file, "\tlea\t");
			fprintf(file, "(%%rbx), %%rsi\n");

			if(regs_dirty) {
				fprintf(file, "\tmov\t$1, %%rdx\n");
				fprintf(file, "\tmov\t%%rdx, %%rax\n");
			}

			else {
				if(last_io_instr == BFT_INSTR_OUT) goto out;
				fprintf(file, "\tmov\t$1, %%rax\n");
			}

			fprintf(file, "\tmov\t%%rax, %%rdi\n");
		out:    fprintf(file, "\tsyscall\n");

			last_io_instr = BFT_INSTR_OUT;
			regs_dirty = false;
			break;

		case BFT_INSTR_LOOP:
			fprintf(file, "\n.L%zu:\n", op1);
			fprintf(file, "\tcmpb\t$0, (%%rbx)\n");
			fprintf(file, "\tje\t.LE%zu\n", op1);

			if(BFt_optim_lvl >= 1) regs_dirty = true;
			break;

		case BFT_INSTR_ENDL:
			fprintf(file, "\tjmp\t.L%zu\n", op1);
			fprintf(file, "\n.LE%zu:\n", op1);

			if(BFt_optim_lvl >= 1) regs_dirty = true;
			break;
		}

		instr = instr -> next;
	}

	fprintf(file, "\tmov\t$60, %%rax\n");
	fprintf(file, "\tmov\t$0, %%rdi\n");
	fprintf(file, "\tsyscall\n");

	int ret = fclose(file);
	if(ret == EOF) BFe_report_err(BFE_UNKNOWN_ERROR);
	exit(0);
}