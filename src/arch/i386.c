/* Bfcli: The Interactive Brainfuck Command-Line Interpreter
 * Copyright (C) 2021 Jyothiraditya Nellakra
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either veesion 3 of the License, or (at your option)
 * any later veesion.
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
	fprintf(file, "\t.bss\n");
	if(BFo_mem_padding) fprintf(file, "\t.skip\t%zu\n", BFo_mem_padding);
	fprintf(file, "\n");

	fprintf(file, "cells:\n");
	fprintf(file, "\t.skip\t%zu\n\n", BFi_mem_size);

	fprintf(file, "\t.global\t_start\n");
	fprintf(file, "\t.text\n\n");

	fprintf(file, "_start:\n");
	fprintf(file, "\tmov\t$cells, %%esi\n");

	int last_io_instr = BFI_INSTR_NOP;
	bool regs_dirty = true;
	bool done_ret = false;

	for(BFi_instr_t *instr = BFi_code; instr; instr = instr -> next) {
		size_t op1 = instr -> op1;
		size_t op2 = instr -> op2;
		ssize_t ad = instr -> ad;

		switch(instr -> opcode) {
		case BFI_INSTR_INC:
			fprintf(file, "\taddb\t$%zu, ", op1);
			if(ad) fprintf(file, "%zd(%%esi)\n", ad);
			else fprintf(file, "(%%esi)\n");
			break;

		case BFI_INSTR_DEC:
			fprintf(file, "\tsubb\t$%zu, ", op1);
			if(ad) fprintf(file, "%zd(%%esi)\n", ad);
			else fprintf(file, "(%%esi)\n");
			break;

		case BFI_INSTR_FWD:
			fprintf(file, "\tadd\t$%zu, ", op1);
			fprintf(file, "%%esi\n");
			break;

		case BFI_INSTR_BCK:
			fprintf(file, "\tsub\t$%zu, ", op1);
			fprintf(file, "%%esi\n");
			break;

		case BFI_INSTR_INP:
			if(ad) fprintf(file, "\tlea\t%zd", ad);
			else fprintf(file, "\tlea\t");
			fprintf(file, "(%%esi), %%ecx\n");

			if(last_io_instr == BFI_INSTR_INP)
				if(!regs_dirty) goto in;

			fprintf(file, "\tmov\t$0, %%ebx\n");
			fprintf(file, "\tmov\t$1, %%edx\n");
		in:	fprintf(file, "\tmov\t$3, %%eax\n");
			fprintf(file, "\tint\t$128\n");

			last_io_instr = BFI_INSTR_INP;
			regs_dirty = false;
			break;

		case BFI_INSTR_OUT:
			if(ad) fprintf(file, "\tlea\t%zd", ad);
			else fprintf(file, "\tlea\t");
			fprintf(file, "(%%esi), %%ecx\n");

			if(last_io_instr == BFI_INSTR_OUT)
				if(!regs_dirty) goto out;

			fprintf(file, "\tmov\t$1, %%ebx\n");
			fprintf(file, "\tmov\t%%ebx, %%edx\n");
		out:	fprintf(file, "\tmov\t$4, %%eax\n");
			fprintf(file, "\tint\t$128\n");

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
			regs_dirty = true;
			break;

		case BFI_INSTR_CMPL:
			fprintf(file, "\tnegb\t(%%esi)\n");
			break;

		case BFI_INSTR_MOV:
			fprintf(file, "\tmovb\t$%zu, ", op1);
			if(ad) fprintf(file, "%zd(%%esi)\n", ad);
			else fprintf(file, "(%%esi)\n");
			break;

		case BFI_INSTR_MULA:
			if(instr -> prev -> opcode == BFI_INSTR_MULA
				&& instr -> prev -> op1 == op1) goto mula;

			if(op1 == 3 || op1 == 5 || op1 == 9) goto lmula;

			fprintf(file, "\tmov\t(%%esi), %%al\n");
			fprintf(file, "\tmov\t$%zu, %%cl\n", op1);
			fprintf(file, "\tmul\t%%cl\n");
		mula:	fprintf(file, "\taddb\t%%al, %zd(%%esi)\n", ad);
			break;

		lmula:	fprintf(file, "\tmovzb\t(%%esi), %%eax\n");
			fprintf(file, "\tlea\t(%%eax,%%eax,%zu), %%eax\n",
				op1 - 1);
			fprintf(file, "\taddb\t%%al, %zd(%%esi)\n", ad);
			break;

		case BFI_INSTR_MULS:
			if(instr -> prev -> opcode == BFI_INSTR_MULS
				&& instr -> prev -> op1 == op1) goto muls;

			if(op1 == 3 || op1 == 5 || op1 == 9) goto lmuls;

			fprintf(file, "\tmov\t(%%esi), %%al\n");
			fprintf(file, "\tmov\t$%zu, %%cl\n", op1);
			fprintf(file, "\tmul\t%%cl\n");
		muls:	fprintf(file, "\tsubb\t%%al, %zd(%%esi)\n", ad);
			break;

		lmuls:	fprintf(file, "\tmovzb\t(%%esi), %%eax\n");
			fprintf(file, "\tlea\t(%%eax,%%eax,%zu), %%eax\n",
				op1 - 1);
			fprintf(file, "\tsubb\t%%al, %zd(%%esi)\n", ad);
			break;

		case BFI_INSTR_SHLA:
			if(instr -> prev -> opcode == BFI_INSTR_SHLA
				&& instr -> prev -> op2 == op2) goto shla;

			switch(op2) {
				case 1: op1 = 2; goto lshla;
				case 2: op1 = 4; goto lshla;
				case 3: op1 = 8; goto lshla;
			}

			fprintf(file, "\tmov\t(%%esi), %%al\n");
			fprintf(file, "\tshl\t$%zu, %%al\n", op2);
		shla:	fprintf(file, "\taddb\t%%al, %zd(%%esi)\n", ad);
			break;

		lshla:	fprintf(file, "\tmovzb\t(%%esi), %%eax\n");
			fprintf(file, "\tlea\t(,%%eax,%zu), %%eax\n", op1);
			fprintf(file, "\taddb\t%%al, %zd(%%esi)\n", ad);
			break;

		case BFI_INSTR_SHLS:
			if(instr -> prev -> opcode == BFI_INSTR_SHLS
				&& instr -> prev -> op2 == op2) goto shls;

			switch(op2) {
				case 1: op1 = 2; goto lshls;
				case 2: op1 = 4; goto lshls;
				case 3: op1 = 8; goto lshls;
			}

			fprintf(file, "\tmov\t(%%esi), %%al\n");
			fprintf(file, "\tshl\t$%zu, %%al\n", op2);
		shls:	fprintf(file, "\tsubb\t%%al, %zd(%%esi)\n", ad);
			break;

		lshls:	fprintf(file, "\tmovzb\t(%%esi), %%eax\n");
			fprintf(file, "\tlea\t(,%%eax,%zu), %%eax\n", op1);
			fprintf(file, "\tsubb\t%%al, %zd(%%esi)\n", ad);
			break;

		case BFI_INSTR_CPYA:
			if(instr -> prev -> opcode == BFI_INSTR_CPYA
				&& instr -> prev -> op1 == op1) goto cpya;

			fprintf(file, "\tmov\t(%%esi), %%al\n");
		cpya:	fprintf(file, "\taddb\t%%al, %zd(%%esi)\n", ad);
			break;

		case BFI_INSTR_CPYS:
			if(instr -> prev -> opcode == BFI_INSTR_CPYS
				&& instr -> prev -> op1 == op1) goto cpys;

			fprintf(file, "\tmov\t(%%esi), %%al\n");
		cpys:	fprintf(file, "\tsubb\t%%al, %zd(%%esi)\n", ad);
			break;

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
			fprintf(file, "\tint\t$128\n");
			done_ret = true;
		}
	}

	if(done_ret) return;
	fprintf(file, "\tmov\t$1, %%eax\n");
	fprintf(file, "\tmov\t$0, %%ebx\n");
	fprintf(file, "\tint\t$128\n");
}

void BFa_i386_tc(FILE *file) {
	fprintf(file, "\tasm volatile ( \"I386_ASSEMBLY:\\n\"\n");
	fprintf(file, "\t\"\tmov\t%%0, %%%%esi\\n\"\n");

	int last_io_instr = BFI_INSTR_NOP;
	bool regs_dirty = true;

	for(BFi_instr_t *instr = BFi_code; instr; instr = instr -> next) {
		size_t op1 = instr -> op1;
		size_t op2 = instr -> op2;
		ssize_t ad = instr -> ad;

		switch(instr -> opcode) {
		case BFI_INSTR_INC:
			fprintf(file, "\t\"\taddb\t$%zu, ", op1);
			if(ad) fprintf(file, "%zd(%%%%esi)\\n\"\n", ad);
			else fprintf(file, "(%%%%esi)\\n\"\n");
			break;

		case BFI_INSTR_DEC:
			fprintf(file, "\t\"\tsubb\t$%zu, ", op1);
			if(ad) fprintf(file, "%zd(%%%%esi)\\n\"\n", ad);
			else fprintf(file, "(%%%%esi)\\n\"\n");
			break;

		case BFI_INSTR_FWD:
			fprintf(file, "\t\"\tadd\t$%zu, ", op1);
			fprintf(file, "%%%%esi\\n\"\n");
			break;

		case BFI_INSTR_BCK:
			fprintf(file, "\t\"\tsub\t$%zu, ", op1);
			fprintf(file, "%%%%esi\\n\"\n");
			break;

		case BFI_INSTR_INP:
			if(ad) fprintf(file, "\t\"\tlea\t%zd", ad);
			else fprintf(file, "\t\"\tlea\t");
			fprintf(file, "(%%%%esi), %%%%ecx\\n\"\n");

			if(last_io_instr == BFI_INSTR_INP)
				if(!regs_dirty) goto in;

			fprintf(file, "\t\"\tmov\t$0, %%%%ebx\\n\"\n");
			fprintf(file, "\t\"\tmov\t$1, %%%%edx\\n\"\n");
		in:	fprintf(file, "\t\"\tmov\t$3, %%%%eax\\n\"\n");
			fprintf(file, "\t\"\tint\t$128\\n\"\n");

			last_io_instr = BFI_INSTR_INP;
			regs_dirty = false;
			break;

		case BFI_INSTR_OUT:
			if(ad) fprintf(file, "\t\"\tlea\t%zd", ad);
			else fprintf(file, "\t\"\tlea\t");
			fprintf(file, "(%%%%esi), %%%%ecx\\n\"\n");

			if(last_io_instr == BFI_INSTR_OUT)
				if(!regs_dirty) goto out;

			fprintf(file, "\t\"\tmov\t$1, %%%%ebx\\n\"\n");
			fprintf(file, "\t\"\tmov\t%%%%ebx, %%%%edx\\n\"\n");
		out:	fprintf(file, "\t\"\tmov\t$4, %%%%eax\\n\"\n");
			fprintf(file, "\t\"\tint\t$128\\n\"\n");

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
			regs_dirty = true;
			break;

		case BFI_INSTR_CMPL:
			fprintf(file, "\t\"\tnegb\t(%%%%esi)\\n\"\n");
			break;

		case BFI_INSTR_MOV:
			fprintf(file, "\t\"\tmovb\t$%zu, ", op1);
			if(ad) fprintf(file, "%zd(%%%%esi)\\n\"\n", ad);
			else fprintf(file, "(%%%%esi)\\n\"\n");
			break;

		case BFI_INSTR_MULA:
			if(instr -> prev -> opcode == BFI_INSTR_MULA
				&& instr -> prev -> op1 == op1) goto mula;

			if(op1 == 3 || op1 == 5 || op1 == 9) goto lmula;

			fprintf(file, "\t\"\tmov\t(%%%%esi), %%%%al\\n\"\n");
			fprintf(file, "\t\"\tmov\t$%zu, %%%%cl\\n\"\n", op1);
			fprintf(file, "\t\"\tmul\t%%%%cl\\n\"\n");
		mula:	fprintf(file, "\t\"\taddb\t%%%%al, %zd(%%%%esi)\\n\"\n", ad);
			break;

		lmula:	fprintf(file, "\t\"\tmovzb\t(%%%%esi), %%%%eax\\n\"\n");
			fprintf(file, "\t\"\tlea\t(%%%%eax,%%%%eax,%zu),%%%%eax\\n\"\n",
				op1 - 1);
			fprintf(file, "\t\"\taddb\t%%%%al, %zd(%%%%esi)\\n\"\n", ad);
			break;

		case BFI_INSTR_MULS:
			if(instr -> prev -> opcode == BFI_INSTR_MULS
				&& instr -> prev -> op1 == op1) goto muls;

			if(op1 == 3 || op1 == 5 || op1 == 9) goto lmuls;

			fprintf(file, "\t\"\tmov\t(%%%%esi), %%%%al\\n\"\n");
			fprintf(file, "\t\"\tmov\t$%zu, %%%%cl\\n\"\n", op1);
			fprintf(file, "\t\"\tmul\t%%%%cl\\n\"\n");
		muls:	fprintf(file, "\t\"\tsubb\t%%%%al, %zd(%%%%esi)\\n\"\n", ad);
			break;

		lmuls:	fprintf(file, "\t\"\tmovzb\t(%%%%esi), %%%%eax\\n\"\n");
			fprintf(file, "\t\"\tlea\t(%%%%eax,%%%%eax,%zu), %%%%eax\\n\"\n",
				op1 - 1);
			fprintf(file, "\t\"\tsubb\t%%%%al, %zd(%%%%esi)\\n\"\n", ad);
			break;

		case BFI_INSTR_SHLA:
			if(instr -> prev -> opcode == BFI_INSTR_SHLA
				&& instr -> prev -> op2 == op2) goto shla;

			switch(op2) {
				case 1: op1 = 2; goto lshla;
				case 2: op1 = 4; goto lshla;
				case 3: op1 = 8; goto lshla;
			}

			fprintf(file, "\t\"\tmov\t(%%%%esi), %%%%al\\n\"\n");
			fprintf(file, "\t\"\tshl\t$%zu, %%%%al\\n\"\n", op2);
		shla:	fprintf(file, "\t\"\taddb\t%%%%al, %zd(%%%%esi)\\n\"\n", ad);
			break;

		lshla:	fprintf(file, "\t\"\tmovzb\t(%%%%esi), %%%%eax\\n\"\n");
			fprintf(file, "\t\"\tlea\t(,%%%%eax,%zu), %%%%eax\\n\"\n", op1);
			fprintf(file, "\t\"\taddb\t%%%%al, %zd(%%%%esi)\\n\"\n", ad);
			break;

		case BFI_INSTR_SHLS:
			if(instr -> prev -> opcode == BFI_INSTR_SHLS
				&& instr -> prev -> op2 == op2) goto shls;

			switch(op2) {
				case 1: op1 = 2; goto lshls;
				case 2: op1 = 4; goto lshls;
				case 3: op1 = 8; goto lshls;
			}

			fprintf(file, "\t\"\tmov\t(%%%%esi), %%%%al\\n\"\n");
			fprintf(file, "\t\"\tshl\t$%zu, %%%%al\\n\"\n", op2);
		shls:	fprintf(file, "\t\"\tsubb\t%%%%al, %zd(%%%%esi)\\n\"\n", ad);
			break;

		lshls:	fprintf(file, "\t\"\tmovzb\t(%%%%esi), %%%%eax\\n\"\n");
			fprintf(file, "\t\"\tlea\t(,%%%%eax,%zu), %%%%eax\\n\"\n", op1);
			fprintf(file, "\t\"\tsubb\t%%%%al, %zd(%%%%esi)\\n\"\n", ad);
			break;

		case BFI_INSTR_CPYA:
			if(instr -> prev -> opcode == BFI_INSTR_CPYA
				&& instr -> prev -> op1 == op1) goto cpya;

			fprintf(file, "\t\"\tmov\t(%%%%esi), %%%%al\\n\"\n");
		cpya:	fprintf(file, "\t\"\taddb\t%%%%al, %zd(%%%%esi)\\n\"\n", ad);
			break;

		case BFI_INSTR_CPYS:
			if(instr -> prev -> opcode == BFI_INSTR_CPYS
				&& instr -> prev -> op1 == op1) goto cpys;

			fprintf(file, "\t\"\tmov\t(%%%%esi), %%%%al\\n\"\n");
		cpys:	fprintf(file, "\t\"\tsubb\t%%%%al, %zd(%%%%esi)\\n\"\n", ad);
			break;

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
			fprintf(file, "\t\"\tint\t$128\\n\"\n");
		}
	}

	fprintf(file, "\n\t: : \"r\" (&cells[%zu])\n",
		BFi_mem_size + BFo_mem_padding);
	
	fprintf(file, "\t: \"eax\", \"ebx\", \"ecx\", \"edx\", \"esi\");\n\n");

	fprintf(file, "\treturn 0;\n");
}