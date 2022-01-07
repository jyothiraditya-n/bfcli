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
#include "../translator.h"

void BFa_i386_tasm(FILE *file) {
	fprintf(file, ".bss\n\n");
	fprintf(file, "cells:\n");
	fprintf(file, "\t.skip\t%zu\n\n", BFi_mem_size);

	fprintf(file, ".global\t_start\n");
	fprintf(file, ".text\n\n");

	fprintf(file, "_start:\n");
	fprintf(file, "\tmov\t$cells, %%esi\n");

	int last_io_instr = BFT_INSTR_NOP;
	bool regs_dirty = true;

	for(BFt_instr_t *instr = BFt_code; instr; instr = instr -> next) {
		size_t op1 = instr -> op1;
		ssize_t ad1 = instr -> ad1;

		switch(instr -> opcode) {
		case BFT_INSTR_INC:
			if(op1 == 1) fprintf(file, "\tincb\t");
			else fprintf(file, "\taddb\t$%zu, ", op1);

			if(ad1) fprintf(file, "%zd(%%esi)\n", ad1);
			else fprintf(file, "(%%esi)\n");
			break;

		case BFT_INSTR_DEC:
			if(op1 == 1) fprintf(file, "\tdecb\t");
			else fprintf(file, "\tsubb\t$%zu, ", op1);

			if(ad1) fprintf(file, "%zd(%%esi)\n", ad1);
			else fprintf(file, "(%%esi)\n");
			break;

		case BFT_INSTR_FWD:
			if(op1 == 1) fprintf(file, "\tinc\t");
			else fprintf(file, "\tadd\t$%zu, ", op1);
			fprintf(file, "%%esi\n");
			break;

		case BFT_INSTR_BCK:
			if(op1 == 1) fprintf(file, "\tdec\t");
			else fprintf(file, "\tsub\t$%zu, ", op1);
			fprintf(file, "%%esi\n");
			break;

		case BFT_INSTR_INP:
			if(ad1) fprintf(file, "\tlea\t%zd", ad1);
			else fprintf(file, "\tlea\t");
			fprintf(file, "(%%esi), %%ecx\n");

			if(last_io_instr == BFT_INSTR_INP)
				if(!regs_dirty) goto in;

			fprintf(file, "\tmov\t$0, %%ebx\n");
			fprintf(file, "\tmov\t$1, %%edx\n");
		in:	fprintf(file, "\tmov\t$3, %%eax\n");
			fprintf(file, "\tint\t$128\n");

			last_io_instr = BFT_INSTR_INP;
			regs_dirty = false;
			break;

		case BFT_INSTR_OUT:
			if(ad1) fprintf(file, "\tlea\t%zd", ad1);
			else fprintf(file, "\tlea\t");
			fprintf(file, "(%%esi), %%ecx\n");

			if(last_io_instr == BFT_INSTR_OUT)
				if(!regs_dirty) goto out;

			fprintf(file, "\tmov\t$1, %%ebx\n");
			fprintf(file, "\tmov\t%%ebx, %%edx\n");
		out:	fprintf(file, "\tmov\t$4, %%eax\n");
			fprintf(file, "\tint\t$128\n");

			last_io_instr = BFT_INSTR_OUT;
			regs_dirty = false;
			break;

		case BFT_INSTR_LOOP:
			fprintf(file, "\n.L%zu:\n", op1);
			fprintf(file, "\tcmpb\t$0, (%%esi)\n");
			fprintf(file, "\tje\t.LE%zu\n", op1);
			regs_dirty = true;
			break;

		case BFT_INSTR_ENDL:
			fprintf(file, "\tjmp\t.L%zu\n", op1);
			fprintf(file, "\n.LE%zu:\n", op1);
			regs_dirty = true;
			break;

		case BFT_INSTR_IFNZ:
			fprintf(file, "\n.L%zu:\n", op1);
			fprintf(file, "\tcmpb\t$0, (%%esi)\n");
			fprintf(file, "\tje\t.LE%zu\n", op1);
			break;

		case BFT_INSTR_ENDIF:
			fprintf(file, "\n.LE%zu:\n", op1);
			regs_dirty = true;
			break;

		case BFT_INSTR_CMPL:
			fprintf(file, "\tnegb\t(%%esi)\n");
			break;

		case BFT_INSTR_MOV:
			fprintf(file, "\tmovb\t$%zu, ", op1);
			if(ad1) fprintf(file, "%zd(%%esi)\n", ad1);
			else fprintf(file, "(%%esi)\n");
			break;

		case BFT_INSTR_MULA:
			fprintf(file, "\tmov\t(%%esi), %%al\n");
			fprintf(file, "\tmov\t$%zu, %%ecx\n", op1);
			fprintf(file, "\tmul\t%%cl\n");
			fprintf(file, "\taddb\t%%al, %zd(%%esi)\n", ad1);
			break;

		case BFT_INSTR_MULS:
			fprintf(file, "\tmov\t(%%esi), %%al\n");
			fprintf(file, "\tmov\t$%zu, %%ecx\n", op1);
			fprintf(file, "\tmul\t%%cl\n");
			fprintf(file, "\tsubb\t%%al, %zd(%%esi)\n", ad1);
			break;

		case BFT_INSTR_SHLA:
			fprintf(file, "\tmov\t(%%esi), %%al\n");
			fprintf(file, "\tshl\t$%zu, %%al\n", op1);
			fprintf(file, "\taddb\t%%al, %zd(%%esi)\n", ad1);
			break;

		case BFT_INSTR_SHLS:
			fprintf(file, "\tmov\t(%%esi), %%al\n");
			fprintf(file, "\tshl\t$%zu, %%al\n", op1);
			fprintf(file, "\tsubb\t%%al, %zd(%%esi)\n", ad1);
			break;

		case BFT_INSTR_CPYA:
			fprintf(file, "\tmov\t(%%esi), %%al\n");
			fprintf(file, "\taddb\t%%al, %zd(%%esi)\n", ad1);
			break;

		case BFT_INSTR_CPYS:
			fprintf(file, "\tmov\t(%%esi), %%al\n");
			fprintf(file, "\tsubb\t%%al, %zd(%%esi)\n", ad1);
			break;
		}
	}

	fprintf(file, "\tmov\t$1, %%eax\n");
	fprintf(file, "\tmov\t$0, %%ebx\n");
	fprintf(file, "\tint\t$128\n");
}

void BFa_i386_tc(FILE *file) {
	fprintf(file, "\tasm volatile ( \"I386_ASSEMBLY:\\n\"\n");
	fprintf(file, "\t\"\tmov\t%%0, %%%%esi\\n\"\n");

	int last_io_instr = BFT_INSTR_NOP;
	bool regs_dirty = true;

	for(BFt_instr_t *instr = BFt_code; instr; instr = instr -> next) {
		size_t op1 = instr -> op1;
		ssize_t ad1 = instr -> ad1;

		switch(instr -> opcode) {
		case BFT_INSTR_INC:
			if(op1 == 1) fprintf(file, "\t\"\tincb\t");
			else fprintf(file, "\t\"\taddb\t$%zu, ", op1);

			if(ad1) fprintf(file, "%zd(%%%%esi)\\n\"\n", ad1);
			else fprintf(file, "(%%%%esi)\\n\"\n");
			break;

		case BFT_INSTR_DEC:
			if(op1 == 1) fprintf(file, "\t\"\tdecb\t");
			else fprintf(file, "\t\"\tsubb\t$%zu, ", op1);

			if(ad1) fprintf(file, "%zd(%%%%esi)\\n\"\n", ad1);
			else fprintf(file, "(%%%%esi)\\n\"\n");
			break;

		case BFT_INSTR_FWD:
			if(op1 == 1) fprintf(file, "\t\"\tinc\t");
			else fprintf(file, "\t\"\tadd\t$%zu, ", op1);
			fprintf(file, "%%%%esi\\n\"\n");
			break;

		case BFT_INSTR_BCK:
			if(op1 == 1) fprintf(file, "\t\"\tdec\t");
			else fprintf(file, "\t\"\tsub\t$%zu, ", op1);
			fprintf(file, "%%%%esi\\n\"\n");
			break;

		case BFT_INSTR_INP:
			if(ad1) fprintf(file, "\t\"\tlea\t%zd", ad1);
			else fprintf(file, "\t\"\tlea\t");
			fprintf(file, "(%%%%esi), %%%%ecx\\n\"\n");

			if(last_io_instr == BFT_INSTR_INP)
				if(!regs_dirty) goto in;

			fprintf(file, "\t\"\tmov\t$0, %%%%ebx\\n\"\n");
			fprintf(file, "\t\"\tmov\t$1, %%%%edx\\n\"\n");
		in:	fprintf(file, "\t\"\tmov\t$3, %%%%eax\\n\"\n");
			fprintf(file, "\t\"\tint\t$128\\n\"\n");

			last_io_instr = BFT_INSTR_INP;
			regs_dirty = false;
			break;

		case BFT_INSTR_OUT:
			if(ad1) fprintf(file, "\t\"\tlea\t%zd", ad1);
			else fprintf(file, "\t\"\tlea\t");
			fprintf(file, "(%%%%esi), %%%%ecx\\n\"\n");

			if(last_io_instr == BFT_INSTR_OUT)
				if(!regs_dirty) goto out;

			fprintf(file, "\t\"\tmov\t$1, %%%%ebx\\n\"\n");
			fprintf(file, "\t\"\tmov\t%%%%ebx, %%%%edx\\n\"\n");
		out:	fprintf(file, "\t\"\tmov\t$4, %%%%eax\\n\"\n");
			fprintf(file, "\t\"\tint\t$128\\n\"\n");

			last_io_instr = BFT_INSTR_OUT;
			regs_dirty = false;
			break;

		case BFT_INSTR_LOOP:
			fprintf(file, "\n\t\".L%zu:\\n\"\n", op1);
			fprintf(file, "\t\"\tcmpb\t$0, (%%%%esi)\\n\"\n");
			fprintf(file, "\t\"\tje\t.LE%zu\\n\"\n", op1);
			regs_dirty = true;
			break;

		case BFT_INSTR_ENDL:
			fprintf(file, "\t\"\tjmp\t.L%zu\\n\"\n", op1);
			fprintf(file, "\n\t\".LE%zu:\\n\"\n", op1);
			regs_dirty = true;
			break;

		case BFT_INSTR_IFNZ:
			fprintf(file, "\n\t\".L%zu:\\n\"\n", op1);
			fprintf(file, "\t\"\tcmpb\t$0, (%%%%esi)\\n\"\n");
			fprintf(file, "\t\"\tje\t.LE%zu\\n\"\n", op1);
			break;

		case BFT_INSTR_ENDIF:
			fprintf(file, "\n\t\".LE%zu:\\n\"\n", op1);
			regs_dirty = true;
			break;

		case BFT_INSTR_CMPL:
			fprintf(file, "\t\"\tnegb\t(%%%%esi)\\n\"\n");
			break;

		case BFT_INSTR_MOV:
			fprintf(file, "\t\"\tmovb\t$%zu, ", op1);
			if(ad1) fprintf(file, "%zd(%%%%esi)\\n\"\n", ad1);
			else fprintf(file, "(%%%%esi)\\n\"\n");
			break;

		case BFT_INSTR_MULA:
			fprintf(file, "\t\"\tmov\t(%%%%esi), %%%%al\\n\"\n");
			fprintf(file, "\t\"\tmov\t$%zu, %%%%ecx\\n\"\n", op1);
			fprintf(file, "\t\"\tmul\t%%%%cl\\n\"\n");
			fprintf(file, "\t\"\taddb\t%%%%al, %zd(%%%%esi)\\n\"\n", ad1);
			break;

		case BFT_INSTR_MULS:
			fprintf(file, "\t\"\tmov\t(%%%%esi), %%%%al\\n\"\n");
			fprintf(file, "\t\"\tmov\t$%zu, %%%%ecx\\n\"\n", op1);
			fprintf(file, "\t\"\tmul\t%%%%cl\\n\"\n");
			fprintf(file, "\t\"\tsubb\t%%%%al, %zd(%%%%esi)\\n\"\n", ad1);
			break;

		case BFT_INSTR_SHLA:
			fprintf(file, "\t\"\tmov\t(%%%%esi), %%%%al\\n\"\n");
			fprintf(file, "\t\"\tshl\t$%zu, %%%%al\\n\"\n", op1);
			fprintf(file, "\t\"\taddb\t%%%%al, %zd(%%%%esi)\\n\"\n", ad1);
			break;

		case BFT_INSTR_SHLS:
			fprintf(file, "\t\"\tmov\t(%%%%esi), %%%%al\\n\"\n");
			fprintf(file, "\t\"\tshl\t$%zu, %%%%al\\n\"\n", op1);
			fprintf(file, "\t\"\tsubb\t%%%%al, %zd(%%%%esi)\\n\"\n", ad1);
			break;

		case BFT_INSTR_CPYA:
			fprintf(file, "\t\"\tmov\t(%%%%esi), %%%%al\\n\"\n");
			fprintf(file, "\t\"\taddb\t%%%%al, %zd(%%%%esi)\\n\"\n", ad1);
			break;

		case BFT_INSTR_CPYS:
			fprintf(file, "\t\"\tmov\t(%%%%esi), %%%%al\\n\"\n");
			fprintf(file, "\t\"\tsubb\t%%%%al, %zd(%%%%esi)\\n\"\n", ad1);
			break;
		}
	}

	fprintf(file, "\n\t: : \"r\" (&cells[0])\n");
	fprintf(file, "\t: \"eax\", \"ebx\", \"ecx\", \"edx\", \"esi\");\n\n");

	fprintf(file, "\treturn 0;\n");
}