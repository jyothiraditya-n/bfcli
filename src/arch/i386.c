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

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "i386.h"

#include "../clidata.h"
#include "../errors.h"
#include "../files.h"
#include "../interpreter.h"
#include "../main.h"
#include "../optims.h"
#include "../translator.h"

void BFa_i386_tasm(FILE *file) {
	if(BFo_precomp_output) {
		fprintf(file, "\t.data\n");

		fprintf(file, "header:\t.ascii\t\"");
		BFf_printstr(file, BFo_precomp_output, false);
		fprintf(file, "\"\n");
		fprintf(file, "length\t=\t. - header\n\n");
	}

	if(!BFi_program_str[0]) goto n2;

	if(BFo_precomp_output) {
		if(!BFo_precomp_cells) fprintf(file, "\t.bss\n");
	}

	else {
		if(BFo_precomp_cells) fprintf(file, "\t.data\n");
		else fprintf(file, "\t.bss\n");
	}

	if(BFo_mem_padding) fprintf(file, "\t.skip\t%zu\n", BFo_mem_padding);

	fprintf(file, BFo_precomp_cells? "cells:\n\t.byte\t" : "cells:\n");
	if(!BFo_precomp_cells) goto next;

	size_t cols = 16;
	char buf[16] = {0};

	for(size_t i = 0; i < BFo_precomp_cells; i++) {
		cols += sprintf(buf, "%d, ", BFi_mem[i]);

		if(cols < 80) fprintf(file, "%s", buf);
		else {
			fprintf(file, "%d", BFi_mem[i]);
			cols = fprintf(file, "\n\t.byte\t") + 13;
		}
	}

	fprintf(file, "0\n");
next:	fprintf(file, "\t.skip\t%zu\n\n", BFi_mem_size - BFo_precomp_cells - 1);

n2:	fprintf(file, "\t.text\n");
	fprintf(file, "\t.global\t_start\n");

	fprintf(file, "_start:\n");
	if(BFo_precomp_output) {
		fprintf(file, "\tmov\t$4, %%eax\n");
		fprintf(file, "\tmov\t$1, %%ebx\n");
		fprintf(file, "\tmov\t$header, %%ecx\n");
		fprintf(file, "\tmov\t$length, %%edx\n");
		fprintf(file, "\tint\t$0x80\n");
	}

	if(!BFi_program_str[0]) goto end;
	
	if(!BFo_precomp_ptr) fprintf(file, "\tmov\t$cells, %%esi\n");
	else fprintf(file, "\tmov\t$(cells + %zu), %%esi\n", BFo_precomp_ptr);

	int last_io_instr = BFI_INSTR_NOP;
	bool regs_dirty = true;
	bool done_ret = false;

	for(BFi_instr_t *instr = BFi_code; instr; instr = instr -> next) {
		size_t op1 = instr -> op1, op2 = instr -> op2;
		ssize_t ad1 = instr -> ad1, ad2 = instr -> ad2;

		switch(instr -> opcode) {
		case BFI_INSTR_INC:
			fprintf(file, "\taddb\t$%zu, ", op1);
			if(ad1) fprintf(file, "%zd(%%esi)\n", ad1);
			else fprintf(file, "(%%esi)\n");
			break;

		case BFI_INSTR_DEC:
			fprintf(file, "\tsubb\t$%zu, ", op1);
			if(ad1) fprintf(file, "%zd(%%esi)\n", ad1);
			else fprintf(file, "(%%esi)\n");
			break;

		case BFI_INSTR_FWD:
			fprintf(file, "\tadd\t$%zu, %%esi\n", op1);
			break;

		case BFI_INSTR_BCK:
			fprintf(file, "\tsub\t$%zu, %%esi\n", op1);
			break;

		case BFI_INSTR_INP:
			if(ad1) fprintf(file, "\tlea\t%zd", ad1);
			else fprintf(file, "\tlea\t");
			fprintf(file, "(%%esi), %%ecx\n");

			if(last_io_instr == BFI_INSTR_INP)
				if(!regs_dirty) goto in;

			fprintf(file, "\tmov\t$0, %%ebx\n");
			fprintf(file, "\tmov\t$1, %%edx\n");
		in:	fprintf(file, "\tmov\t$3, %%eax\n");
			fprintf(file, "\tint\t$0x80\n");

			last_io_instr = BFI_INSTR_INP;
			regs_dirty = false;
			break;

		case BFI_INSTR_OUT:
			if(ad1) fprintf(file, "\tlea\t%zd", ad1);
			else fprintf(file, "\tlea\t");
			fprintf(file, "(%%esi), %%ecx\n");

			if(last_io_instr == BFI_INSTR_OUT)
				if(!regs_dirty) goto out;

			fprintf(file, "\tmov\t$1, %%ebx\n");
			fprintf(file, "\tmov\t%%ebx, %%edx\n");
		out:	fprintf(file, "\tmov\t$4, %%eax\n");
			fprintf(file, "\tint\t$0x80\n");

			last_io_instr = BFI_INSTR_OUT;
			regs_dirty = false;
			break;

		case BFI_INSTR_LOOP:
			fprintf(file, "\n.L%zu:\n", op1);
			fprintf(file, "\tcmpb\t$0, (%%esi)\n");
			fprintf(file, "\tje\t.LE%zu\n", op1);
			regs_dirty = true;
			break;

		case BFI_INSTR_ENDL:
			fprintf(file, "\tjmp\t.L%zu\n", op1);
			fprintf(file, "\n.LE%zu:\n", op1);
			regs_dirty = true;
			break;

		case BFI_INSTR_IFNZ:
			fprintf(file, "\n.L%zu:\n", op1);
			fprintf(file, "\tcmpb\t$0, (%%esi)\n");
			fprintf(file, "\tje\t.LE%zu\n", op1);
			break;

		case BFI_INSTR_ENDIF:
			fprintf(file, "\n.LE%zu:\n", op1);
			break;

		case BFI_INSTR_CMPL:
			fprintf(file, "\tnegb\t(%%esi)\n");
			break;

		case BFI_INSTR_MOV:
			fprintf(file, "\tmovb\t$%zu, ", op1);
			if(ad1) fprintf(file, "%zd(%%esi)\n", ad1);
			else fprintf(file, "(%%esi)\n");
			break;

		case BFI_INSTR_MULA:
			if(instr -> prev -> opcode == BFI_INSTR_MULA
				&& instr -> prev -> op1 == op1
				&& instr -> prev -> ad1 == ad1) goto mula;

			if(op1 == 3 || op1 == 5 || op1 == 9) goto lmula;

			if(ad2) fprintf(file, "\tmov\t%zd(%%esi), ", ad2);
			else fprintf(file, "\tmov\t(%%esi), ");
			fprintf(file, "%%al\n");

			fprintf(file, "\tmov\t$%zu, %%cl\n", op1);
			fprintf(file, "\tmul\t%%cl\n");
		mula:	fprintf(file, "\taddb\t%%al, ");
			if(ad1) fprintf(file, "%zd(%%esi)\n", ad1);
			else fprintf(file, "(%%esi)\n");
			regs_dirty = true;
			break;

		lmula:	if(ad2) fprintf(file, "\tmovzb\t%zd(%%esi), ", ad2);
			else fprintf(file, "\tmovzb\t(%%esi), ");
			fprintf(file, "%%eax\n");

			fprintf(file, "\tlea\t(%%eax,%%eax,%zu), %%eax\n",
				op1 - 1);
			goto mula;

		case BFI_INSTR_MULS:
			if(instr -> prev -> opcode == BFI_INSTR_MULS
				&& instr -> prev -> op1 == op1
				&& instr -> prev -> ad1 == ad1) goto muls;

			if(op1 == 3 || op1 == 5 || op1 == 9) goto lmuls;

			if(ad2) fprintf(file, "\tmov\t%zd(%%esi), ", ad2);
			else fprintf(file, "\tmov\t(%%esi), ");
			fprintf(file, "%%al\n");

			fprintf(file, "\tmov\t$%zu, %%cl\n", op1);
			fprintf(file, "\tmul\t%%cl\n");
		muls:	fprintf(file, "\tsubb\t%%al, ");
			if(ad1) fprintf(file, "%zd(%%esi)\n", ad1);
			else fprintf(file, "(%%esi)\n");
			regs_dirty = true;
			break;

		lmuls:	if(ad2) fprintf(file, "\tmovzb\t%zd(%%esi), ", ad2);
			else fprintf(file, "\tmovzb\t(%%esi), ");
			fprintf(file, "%%eax\n");

			fprintf(file, "\tlea\t(%%eax,%%eax,%zu), %%eax\n",
				op1 - 1);
			goto muls;

		case BFI_INSTR_MULM:
			if((instr -> prev -> opcode == BFI_INSTR_MULM
				|| instr -> prev -> opcode == BFI_INSTR_MULA)
				&& instr -> prev -> op1 == op1
				&& instr -> prev -> ad1 == ad1) goto mulm;

			if(op1 == 3 || op1 == 5 || op1 == 9) goto lmulm;

			if(ad2) fprintf(file, "\tmov\t%zd(%%esi), ", ad2);
			else fprintf(file, "\tmov\t(%%esi), ");
			fprintf(file, "%%al\n");

			fprintf(file, "\tmov\t$%zu, %%cl\n", op1);
			fprintf(file, "\tmul\t%%cl\n");
		mulm:	fprintf(file, "\tmovb\t%%al, ");
			if(ad1) fprintf(file, "%zd(%%esi)\n", ad1);
			else fprintf(file, "(%%esi)\n");
			regs_dirty = true;
			break;

		lmulm:	if(ad2) fprintf(file, "\tmovzb\t%zd(%%esi), ", ad2);
			else fprintf(file, "\tmovzb\t(%%esi), ");
			fprintf(file, "%%eax\n");

			fprintf(file, "\tlea\t(%%eax,%%eax,%zu), %%eax\n",
				op1 - 1);
			goto mulm;

		case BFI_INSTR_SHLA:
			if(instr -> prev -> opcode == BFI_INSTR_SHLA
				&& instr -> prev -> op2 == op2
				&& instr -> prev -> ad1 == ad1) goto mula;

			switch(op2) {
				case 1: op1 = 2; goto lshla;
				case 2: op1 = 4; goto lshla;
				case 3: op1 = 8; goto lshla;
			}

			if(ad2) fprintf(file, "\tmov\t%zd(%%esi), ", ad2);
			else fprintf(file, "\tmov\t(%%esi), ");
			fprintf(file, "%%al\n");

			if(op2 == 1) fprintf(file, "\tshl\t%%al\n");
			else fprintf(file, "\tshl\t$%zu, %%al\n", op2);
			goto mula;

		lshla:	if(ad2) fprintf(file, "\tmovzb\t%zd(%%esi), ", ad2);
			else fprintf(file, "\tmovzb\t(%%esi), ");
			fprintf(file, "%%eax\n");

			fprintf(file, "\tlea\t(,%%eax,%zu), %%eax\n", op1);
			goto mula;

		case BFI_INSTR_SHLS:
			if(instr -> prev -> opcode == BFI_INSTR_SHLS
				&& instr -> prev -> op2 == op2
				&& instr -> prev -> ad1 == ad1) goto muls;

			switch(op2) {
				case 1: op1 = 2; goto lshls;
				case 2: op1 = 4; goto lshls;
				case 3: op1 = 8; goto lshls;
			}

			if(ad2) fprintf(file, "\tmov\t%zd(%%esi), ", ad2);
			else fprintf(file, "\tmov\t(%%esi), ");
			fprintf(file, "%%al\n");

			if(op2 == 1) fprintf(file, "\tshl\t%%al\n");
			else fprintf(file, "\tshl\t$%zu, %%al\n", op2);
			goto muls;

		lshls:	if(ad2) fprintf(file, "\tmovzb\t%zd(%%esi), ", ad2);
			else fprintf(file, "\tmovzb\t(%%esi), ");
			fprintf(file, "%%eax\n");

			fprintf(file, "\tlea\t(,%%eax,%zu), %%eax\n", op1);
			goto muls;

		case BFI_INSTR_SHLM:
			if((instr -> prev -> opcode == BFI_INSTR_SHLM
				|| instr -> prev -> opcode == BFI_INSTR_SHLA)
				&& instr -> prev -> op2 == op2
				&& instr -> prev -> ad1 == ad1) goto mulm;

			switch(op2) {
				case 1: op1 = 2; goto lshlm;
				case 2: op1 = 4; goto lshlm;
				case 3: op1 = 8; goto lshlm;
			}

			if(ad2) fprintf(file, "\tmov\t%zd(%%esi), ", ad2);
			else fprintf(file, "\tmov\t(%%esi), ");
			fprintf(file, "%%al\n");

			if(op2 == 1) fprintf(file, "\tshl\t%%al\n");
			else fprintf(file, "\tshl\t$%zu, %%al\n", op2);
			goto mulm;

		lshlm:	if(ad2) fprintf(file, "\tmovzb\t%zd(%%esi), ", ad2);
			else fprintf(file, "\tmovzb\t(%%esi), ");
			fprintf(file, "%%eax\n");

			fprintf(file, "\tlea\t(,%%eax,%zu), %%eax\n", op1);
			goto mulm;

		case BFI_INSTR_CPYA:
			if(instr -> prev -> opcode == BFI_INSTR_CPYA
				&& instr -> prev -> op1 == op1
				&& instr -> prev -> ad1 == ad1) goto mula;

			if(ad2) fprintf(file, "\tmov\t%zd(%%esi), ", ad2);
			else fprintf(file, "\tmov\t(%%esi), ");
			fprintf(file, "%%al\n");
			goto mula;

		case BFI_INSTR_CPYS:
			if(instr -> prev -> opcode == BFI_INSTR_CPYS
				&& instr -> prev -> op1 == op1
				&& instr -> prev -> ad1 == ad1) goto muls;

			if(ad2) fprintf(file, "\tmov\t%zd(%%esi), ", ad2);
			else fprintf(file, "\tmov\t(%%esi), ");
			fprintf(file, "%%al\n");
			goto muls;

		case BFI_INSTR_CPYM:
			if((instr -> prev -> opcode == BFI_INSTR_CPYM
				|| instr -> prev -> opcode == BFI_INSTR_CPYA)
				&& instr -> prev -> op1 == op1
				&& instr -> prev -> ad1 == ad1) goto mulm;

			if(ad2) fprintf(file, "\tmov\t%zd(%%esi), ", ad2);
			else fprintf(file, "\tmov\t(%%esi), ");
			fprintf(file, "%%al\n");
			goto mulm;

		case BFI_INSTR_SUB:
			fprintf(file, "\n_%zu:\n", op1);
			regs_dirty = true;
			break;

		case BFI_INSTR_JSR:
			fprintf(file, "\tcall\t_%zu\n", op1);
			regs_dirty = true;
			break;

		case BFI_INSTR_RTS:
			fprintf(file, "\tret\n");
			regs_dirty = true;
			break;

		case BFI_INSTR_RET:
			fprintf(file, "\tmov\t$1, %%eax\n");
			fprintf(file, "\tmov\t$0, %%ebx\n");
			fprintf(file, "\tint\t$0x80\n");
			done_ret = true;
		}
	}

	if(done_ret) return;
end:	fprintf(file, "\tmov\t$1, %%eax\n");
	fprintf(file, "\tmov\t$0, %%ebx\n");
	fprintf(file, "\tint\t$0x80\n");
}

void BFa_i386_tc(FILE *file) {
	fprintf(file, "\tasm volatile (\n");
	fprintf(file, "\t\"\tmov\t%%0, %%%%esi\\n\"\n");

	int last_io_instr = BFI_INSTR_NOP;
	bool regs_dirty = true;

	for(BFi_instr_t *instr = BFi_code; instr; instr = instr -> next) {
		size_t op1 = instr -> op1, op2 = instr -> op2;
		ssize_t ad1 = instr -> ad1, ad2 = instr -> ad2;

		switch(instr -> opcode) {
		case BFI_INSTR_INC:
			fprintf(file, "\t\"\taddb\t$%zu, ", op1);
			if(ad1) fprintf(file, "%zd(%%%%esi)\\n\"\n", ad1);
			else fprintf(file, "(%%%%esi)\\n\"\n");
			break;

		case BFI_INSTR_DEC:
			fprintf(file, "\t\"\tsubb\t$%zu, ", op1);
			if(ad1) fprintf(file, "%zd(%%%%esi)\\n\"\n", ad1);
			else fprintf(file, "(%%%%esi)\\n\"\n");
			break;

		case BFI_INSTR_FWD:
			fprintf(file, "\t\"\tadd\t$%zu, %%%%esi\\n\"\n", op1);
			break;

		case BFI_INSTR_BCK:
			fprintf(file, "\t\"\tsub\t$%zu, %%%%esi\\n\"\n", op1);
			break;

		case BFI_INSTR_INP:
			if(ad1) fprintf(file, "\t\"\tlea\t%zd", ad1);
			else fprintf(file, "\t\"\tlea\t");
			fprintf(file, "(%%%%esi), %%%%ecx\\n\"\n");

			if(last_io_instr == BFI_INSTR_INP)
				if(!regs_dirty) goto in;

			fprintf(file, "\t\"\tmov\t$0, %%%%ebx\\n\"\n");
			fprintf(file, "\t\"\tmov\t$1, %%%%edx\\n\"\n");
		in:	fprintf(file, "\t\"\tmov\t$3, %%%%eax\\n\"\n");
			fprintf(file, "\t\"\tint\t$0x80\\n\"\n");

			last_io_instr = BFI_INSTR_INP;
			regs_dirty = false;
			break;

		case BFI_INSTR_OUT:
			if(ad1) fprintf(file, "\t\"\tlea\t%zd", ad1);
			else fprintf(file, "\t\"\tlea\t");
			fprintf(file, "(%%%%esi), %%%%ecx\\n\"\n");

			if(last_io_instr == BFI_INSTR_OUT)
				if(!regs_dirty) goto out;

			fprintf(file, "\t\"\tmov\t$1, %%%%ebx\\n\"\n");
			fprintf(file, "\t\"\tmov\t%%%%ebx, %%%%edx\\n\"\n");
		out:	fprintf(file, "\t\"\tmov\t$4, %%%%eax\\n\"\n");
			fprintf(file, "\t\"\tint\t$0x80\\n\"\n");

			last_io_instr = BFI_INSTR_OUT;
			regs_dirty = false;
			break;

		case BFI_INSTR_LOOP:
			fprintf(file, "\n\t\".L%zu:\\n\"\n", op1);
			fprintf(file, "\t\"\tcmpb\t$0, (%%%%esi)\\n\"\n");
			fprintf(file, "\t\"\tje\t.LE%zu\\n\"\n", op1);
			regs_dirty = true;
			break;

		case BFI_INSTR_ENDL:
			fprintf(file, "\t\"\tjmp\t.L%zu\\n\"\n", op1);
			fprintf(file, "\n\t\".LE%zu:\\n\"\n", op1);
			regs_dirty = true;
			break;

		case BFI_INSTR_IFNZ:
			fprintf(file, "\n\t\".L%zu:\\n\"\n", op1);
			fprintf(file, "\t\"\tcmpb\t$0, (%%%%esi)\\n\"\n");
			fprintf(file, "\t\"\tje\t.LE%zu\\n\"\n", op1);
			break;

		case BFI_INSTR_ENDIF:
			fprintf(file, "\n\t\".LE%zu:\\n\"\n", op1);
			break;

		case BFI_INSTR_CMPL:
			fprintf(file, "\t\"\tnegb\t(%%%%esi)\\n\"\n");
			break;

		case BFI_INSTR_MOV:
			fprintf(file, "\t\"\tmovb\t$%zu, ", op1);
			if(ad1) fprintf(file, "%zd(%%%%esi)\\n\"\n", ad1);
			else fprintf(file, "(%%%%esi)\\n\"\n");
			break;

		case BFI_INSTR_MULA:
			if(instr -> prev -> opcode == BFI_INSTR_MULA
				&& instr -> prev -> op1 == op1
				&& instr -> prev -> ad1 == ad1) goto mula;

			if(op1 == 3 || op1 == 5 || op1 == 9) goto lmula;

			if(ad2) fprintf(file, "\t\"\tmov\t%zd(%%%%esi), ", ad2);
			else fprintf(file, "\t\"\tmov\t(%%%%esi)");
			fprintf(file, "%%%%al\\n\"\n");

			fprintf(file, "\t\"\tmov\t$%zu, %%%%cl\\n\"\n", op1);
			fprintf(file, "\t\"\tmul\t%%%%cl\\n\"\n");
		mula:	fprintf(file, "\t\"\taddb\t%%%%al, ");
			if(ad1) fprintf(file, "%zd(%%%%esi)\\n\"\n", ad1);
			else fprintf(file, "(%%%%esi)\\n\"\n");
			regs_dirty = true;
			break;

		lmula:	if(ad2) fprintf(file, "\t\"\tmovzb\t%zd(%%%%esi), ", ad2);
			else fprintf(file, "\t\"\tmovzb\t(%%%%esi)");
			fprintf(file, "%%%%eax\\n\"\n");

			fprintf(file, "\t\"\tlea\t(%%%%eax,%%%%eax,%zu),%%%%eax\\n\"\n",
				op1 - 1);
			goto mula;

		case BFI_INSTR_MULS:
			if(instr -> prev -> opcode == BFI_INSTR_MULS
				&& instr -> prev -> op1 == op1
				&& instr -> prev -> ad1 == ad1) goto muls;

			if(op1 == 3 || op1 == 5 || op1 == 9) goto lmuls;

			if(ad2) fprintf(file, "\t\"\tmov\t%zd(%%%%esi), ", ad2);
			else fprintf(file, "\t\"\tmov\t(%%%%esi)");
			fprintf(file, "%%%%al\\n\"\n");

			fprintf(file, "\t\"\tmov\t$%zu, %%%%cl\\n\"\n", op1);
			fprintf(file, "\t\"\tmul\t%%%%cl\\n\"\n");
		muls:	fprintf(file, "\t\"\tsubb\t%%%%al, ");
			if(ad1) fprintf(file, "%zd(%%%%esi)\\n\"\n", ad1);
			else fprintf(file, "(%%%%esi)\\n\"\n");
			regs_dirty = true;
			break;

		lmuls:	if(ad2) fprintf(file, "\t\"\tmovzb\t%zd(%%%%esi), ", ad2);
			else fprintf(file, "\t\"\tmovzb\t(%%%%esi)");
			fprintf(file, "%%%%eax\\n\"\n");

			fprintf(file, "\t\"\tlea\t(%%%%eax,%%%%eax,%zu), %%%%eax\\n\"\n",
				op1 - 1);
			goto muls;

		case BFI_INSTR_MULM:
			if((instr -> prev -> opcode == BFI_INSTR_MULM
				|| instr -> prev -> opcode == BFI_INSTR_MULA)
				&& instr -> prev -> op1 == op1
				&& instr -> prev -> ad1 == ad1) goto mulm;

			if(op1 == 3 || op1 == 5 || op1 == 9) goto lmulm;

			if(ad2) fprintf(file, "\t\"\tmov\t%zd(%%%%esi), ", ad2);
			else fprintf(file, "\t\"\tmov\t(%%%%esi)");
			fprintf(file, "%%%%al\\n\"\n");

			fprintf(file, "\t\"\tmov\t$%zu, %%%%cl\\n\"\n", op1);
			fprintf(file, "\t\"\tmul\t%%%%cl\\n\"\n");
		mulm:	fprintf(file, "\t\"\tmovb\t%%%%al, ");
			if(ad1) fprintf(file, "%zd(%%%%esi)\\n\"\n", ad1);
			else fprintf(file, "(%%%%esi)\\n\"\n");
			regs_dirty = true;
			break;

		lmulm:	if(ad2) fprintf(file, "\t\"\tmovzb\t%zd(%%%%esi), ", ad2);
			else fprintf(file, "\t\"\tmovzb\t(%%%%esi)");
			fprintf(file, "%%%%eax\\n\"\n");

			fprintf(file, "\t\"\tlea\t(%%%%eax,%%%%eax,%zu),%%%%eax\\n\"\n",
				op1 - 1);
			goto mulm;

		case BFI_INSTR_SHLA:
			if(instr -> prev -> opcode == BFI_INSTR_SHLA
				&& instr -> prev -> op2 == op2
				&& instr -> prev -> ad1 == ad1) goto mula;

			switch(op2) {
				case 1: op1 = 2; goto lshla;
				case 2: op1 = 4; goto lshla;
				case 3: op1 = 8; goto lshla;
			}

			if(ad2) fprintf(file, "\t\"\tmov\t%zd(%%%%esi), ", ad2);
			else fprintf(file, "\t\"\tmov\t(%%%%esi)");
			fprintf(file, "%%%%al\\n\"\n");

			if(op2 == 1) fprintf(file, "\t\"\tshl\t%%%%al\\n\"\n");
			else fprintf(file, "\t\"\tshl\t$%zu, %%%%al\\n\"\n", op2);
			goto mula;

		lshla:	if(ad2) fprintf(file, "\t\"\tmovzb\t%zd(%%%%esi), ", ad2);
			else fprintf(file, "\t\"\tmovzb\t(%%%%esi)");
			fprintf(file, "%%%%eax\\n\"\n");

			fprintf(file, "\t\"\tlea\t(,%%%%eax,%zu), %%%%eax\\n\"\n", op1);
			goto mula;

		case BFI_INSTR_SHLS:
			if(instr -> prev -> opcode == BFI_INSTR_SHLS
				&& instr -> prev -> op2 == op2
				&& instr -> prev -> ad1 == ad1) goto muls;

			switch(op2) {
				case 1: op1 = 2; goto lshls;
				case 2: op1 = 4; goto lshls;
				case 3: op1 = 8; goto lshls;
			}

			if(ad2) fprintf(file, "\t\"\tmov\t%zd(%%%%esi), ", ad2);
			else fprintf(file, "\t\"\tmov\t(%%%%esi)");
			fprintf(file, "%%%%al\\n\"\n");

			if(op2 == 1) fprintf(file, "\t\"\tshl\t%%%%al\\n\"\n");
			else fprintf(file, "\t\"\tshl\t$%zu, %%%%al\\n\"\n", op2);
			goto muls;

		lshls:	if(ad2) fprintf(file, "\t\"\tmovzb\t%zd(%%%%esi), ", ad2);
			else fprintf(file, "\t\"\tmovzb\t(%%%%esi)");
			fprintf(file, "%%%%eax\\n\"\n");

			fprintf(file, "\t\"\tlea\t(,%%%%eax,%zu), %%%%eax\\n\"\n", op1);
			goto muls;

		case BFI_INSTR_SHLM:
			if((instr -> prev -> opcode == BFI_INSTR_SHLM
				|| instr -> prev -> opcode == BFI_INSTR_SHLA)
				&& instr -> prev -> op2 == op2
				&& instr -> prev -> ad1 == ad1) goto mulm;

			switch(op2) {
				case 1: op1 = 2; goto lshlm;
				case 2: op1 = 4; goto lshlm;
				case 3: op1 = 8; goto lshlm;
			}

			if(ad2) fprintf(file, "\t\"\tmov\t%zd(%%%%esi), ", ad2);
			else fprintf(file, "\t\"\tmov\t(%%%%esi)");
			fprintf(file, "%%%%al\\n\"\n");

			if(op2 == 1) fprintf(file, "\t\"\tshl\t%%%%al\\n\"\n");
			else fprintf(file, "\t\"\tshl\t$%zu, %%%%al\\n\"\n", op2);
			goto mulm;

		lshlm:	if(ad2) fprintf(file, "\t\"\tmovzb\t%zd(%%%%esi), ", ad2);
			else fprintf(file, "\t\"\tmovzb\t(%%%%esi)");
			fprintf(file, "%%%%eax\\n\"\n");

			fprintf(file, "\t\"\tlea\t(,%%%%eax,%zu), %%%%eax\\n\"\n", op1);
			goto mulm;

		case BFI_INSTR_CPYA:
			if(instr -> prev -> opcode == BFI_INSTR_CPYA
				&& instr -> prev -> op1 == op1
				&& instr -> prev -> ad1 == ad1) goto mula;

			if(ad2) fprintf(file, "\t\"\tmov\t%zd(%%%%esi), ", ad2);
			else fprintf(file, "\t\"\tmov\t(%%%%esi)");
			fprintf(file, "%%%%al\\n\"\n");
			goto mula;

		case BFI_INSTR_CPYS:
			if(instr -> prev -> opcode == BFI_INSTR_CPYS
				&& instr -> prev -> op1 == op1
				&& instr -> prev -> ad1 == ad1) goto muls;

			if(ad2) fprintf(file, "\t\"\tmov\t%zd(%%%%esi), ", ad2);
			else fprintf(file, "\t\"\tmov\t(%%%%esi)");
			fprintf(file, "%%%%al\\n\"\n");
			goto muls;

		case BFI_INSTR_CPYM:
			if((instr -> prev -> opcode == BFI_INSTR_CPYM
				|| instr -> prev -> opcode == BFI_INSTR_CPYA)
				&& instr -> prev -> op1 == op1
				&& instr -> prev -> ad1 == ad1) goto mulm;

			if(ad2) fprintf(file, "\t\"\tmov\t%zd(%%%%esi), ", ad2);
			else fprintf(file, "\t\"\tmov\t(%%%%esi)");
			fprintf(file, "%%%%al\\n\"\n");
			goto mulm;

		case BFI_INSTR_SUB:
			fprintf(file, "\n\t\"_%zu:\\n\"\n", op1);
			regs_dirty = true;
			break;

		case BFI_INSTR_JSR:
			fprintf(file, "\t\"\tcall\t_%zu\\n\"\n", op1);
			regs_dirty = true;
			break;

		case BFI_INSTR_RTS:
			fprintf(file, "\t\"\tret\\n\"\n");
			regs_dirty = true;
			break;

		case BFI_INSTR_RET:
			fprintf(file, "\t\"\tmov\t$1, %%%%eax\\n\"\n");
			fprintf(file, "\t\"\tmov\t$0, %%%%ebx\\n\"\n");
			fprintf(file, "\t\"\tint\t$0x80\\n\"\n");
		}
	}

	fprintf(file, "\n\t: :\t\"r\" (&cells[%zu])\n",
		BFo_mem_padding + BFo_precomp_ptr);
	
	fprintf(file, "\t:\t\"eax\", \"ebx\", \"ecx\", \"edx\", \"esi\"\n\t);\n\n");

	fprintf(file, "\treturn 0;\n");
}