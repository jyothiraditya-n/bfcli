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

#include "8086.h"

#include "../clidata.h"
#include "../errors.h"
#include "../files.h"
#include "../interpreter.h"
#include "../main.h"
#include "../optims.h"
#include "../translator.h"

void BFa_8086_t(FILE *file) {
	fprintf(file, "\tbits\t16\n");
	fprintf(file, "\torg\t100h\n\n");

	if(BFi_program_str[0]) {
		fprintf(file, "init:\n");
		fprintf(file, "\txor\tbx, bx\n");
		fprintf(file, "\tmov\tsi, cells\n\n");

		fprintf(file, ".loop:\n");
		fprintf(file, "\tmov\tbyte [si], 0\n");
		fprintf(file, "\tinc\tsi\n");
		fprintf(file, "\tcmp\tsi, sp\n");
		fprintf(file, "\tje\tmain\n");
		fprintf(file, "\tjmp\t.loop\n\n");
	}

	else fprintf(file, "\tjmp\tmain\n\n");

	for(BFi_instr_t *instr = BFi_code; instr; instr = instr -> next) {
		if(instr -> opcode != BFI_INSTR_INP) continue;

		fprintf(file, "input:\n");
		fprintf(file, "\tmov\tah, 1\n");
		fprintf(file, "\tint\t21h\n");
		fprintf(file, "\tcmp\tal, 13\n");
		fprintf(file, "\tjne\t.end\n");
		fprintf(file, "\tmov\tal, 10\n\n");

		fprintf(file, ".end:\n");
		fprintf(file, "\tret\n\n");
		break;
	}

	if(BFo_precomp_output) goto out;
	for(BFi_instr_t *instr = BFi_code; instr; instr = instr -> next) {
		if(instr -> opcode != BFI_INSTR_OUT) continue;

	out:	fprintf(file, "output:\n");
		fprintf(file, "\tmov\tah, 0eh\n");
		fprintf(file, "\tcmp\tal, 10\n");
		fprintf(file, "\tjne\t.end\n");
		fprintf(file, "\tmov\tal, 13\n");
		fprintf(file, "\tint\t10h\n");
		fprintf(file, "\tmov\tal, 10\n\n");

		fprintf(file, ".end:\n");
		fprintf(file, "\tint\t10h\n");
		fprintf(file, "\tret\n\n");

		break;
	}

	fprintf(file, "main:\n");
	bool done_ret = false;

	if(BFo_precomp_output) {
		fprintf(file, "\tmov\tsi, header\n\n");

		fprintf(file, ".loop:\n");
		fprintf(file, "\tlodsb\n");
		fprintf(file, "\tcmp\tsi, cells\n");
		fprintf(file, "\tje\t.next\n");
		fprintf(file, "\tcall\toutput\n");
		fprintf(file, "\tjmp\t.loop\n\n");

		fprintf(file, ".next:\n");
		if(!BFo_precomp_ptr) goto next;
	}

	if(!BFo_precomp_ptr) fprintf(file, "\tmov\tsi, cells\n");
	else fprintf(file, "\tmov\tsi, cells + %zd\n", BFo_precomp_ptr);

next:	for(BFi_instr_t *instr = BFi_code; instr; instr = instr -> next) {
		size_t op1 = instr -> op1, op2 = instr -> op2;
		ssize_t ad1 = instr -> ad1, ad2 = instr -> ad2;

		switch(instr -> opcode) {
		case BFI_INSTR_INC:
			if(op1 != 1) fprintf(file, "\tadd\t");
			else fprintf(file, "\tinc\t");

			if(ad1 > 0) fprintf(file, "byte [si+%zd]", ad1);
			else if(ad1 < 0) fprintf(file, "byte [si-%zd]", -ad1);
			else fprintf(file, "byte [si]");

			if(op1 != 1) fprintf(file, ", %zu\n", op1);
			else fprintf(file, "\n");
			break;

		case BFI_INSTR_DEC:
			if(op1 != 1) fprintf(file, "\tsub\t");
			else fprintf(file, "\tdec\t");

			if(ad1 > 0) fprintf(file, "byte [si+%zd]", ad1);
			else if(ad1 < 0) fprintf(file, "byte [si-%zd]", -ad1);
			else fprintf(file, "byte [si]");

			if(op1 != 1) fprintf(file, ", %zu\n", op1);
			else fprintf(file, "\n");
			break;

		case BFI_INSTR_FWD:
			if(op1 == 1) fprintf(file, "\tinc\tsi\n");
			else fprintf(file, "\tadd\tsi, %zu\n", op1);
			break;

		case BFI_INSTR_BCK:
			if(op1 == 1) fprintf(file, "\tdec\tsi\n");
			else fprintf(file, "\tsub\tsi, %zu\n", op1);
			break;

		case BFI_INSTR_INP:
			fprintf(file, "\tcall\tinput\n");

			fprintf(file, "\tmov\t");
			if(ad1 > 0) fprintf(file, "[si+%zd], ", ad1);
			else if(ad1 < 0) fprintf(file, "[si-%zd], ", -ad1);
			else fprintf(file, "[si], ");
			fprintf(file, "al\n");
			break;

		case BFI_INSTR_OUT:
			fprintf(file, "\tmov\tal, ");
			if(ad1 > 0) fprintf(file, "[si+%zd]\n", ad1);
			else if(ad1 < 0) fprintf(file, "[si-%zd]\n", -ad1);
			else fprintf(file, "[si]\n");

			fprintf(file, "\tcall\toutput\n");
			break;

		case BFI_INSTR_LOOP:
			fprintf(file, "\n.l%zu:\n", op1);
			fprintf(file, "\tcmp\tbyte [si], 0\n");
			fprintf(file, "\tje\t.e%zu\n", op1);
			break;

		case BFI_INSTR_ENDL:
			fprintf(file, "\tjmp\t.l%zu\n", op1);
			fprintf(file, "\n.e%zu:\n", op1);
			break;

		case BFI_INSTR_IFNZ:
			fprintf(file, "\n.l%zu:\n", op1);
			fprintf(file, "\tcmp\tbyte [si], 0\n");
			fprintf(file, "\tje\t.e%zu\n", op1);
			break;

		case BFI_INSTR_ENDIF:
			fprintf(file, "\n.e%zu:\n", op1);
			break;

		case BFI_INSTR_CMPL:
			fprintf(file, "\tneg\tbyte [si]\n");
			break;

		case BFI_INSTR_MOV:
			fprintf(file, "\tmov\tbyte ");
			if(ad1 > 0) fprintf(file, "[si+%zd], ", ad1);
			else if(ad1 < 0) fprintf(file, "[si-%zd], ", -ad1);
			else fprintf(file, "[si], ");
			fprintf(file, "%zu\n", op1);
			break;

		case BFI_INSTR_MULA:
			if(instr -> prev -> opcode == BFI_INSTR_MULA
				&& instr -> prev -> op1 == op1
				&& instr -> prev -> ad1 == ad1) goto mula;

			fprintf(file, "\tmov\tal, ");
			if(ad2 > 0) fprintf(file, "[si+%zd]\n", ad2);
			else if(ad2 < 0) fprintf(file, "[si-%zd]\n", -ad2);
			else fprintf(file, "[si]\n");
			
			fprintf(file, "\tmov\tcl, %zu\n", op1);
			fprintf(file, "\tmul\tcl\n");

		mula:	fprintf(file, "\tadd\t");
			if(ad1 > 0) fprintf(file, "[si+%zd], al\n", ad1);
			else fprintf(file, "[si-%zd], al\n", -ad1);
			break;

		case BFI_INSTR_MULS:
			if(instr -> prev -> opcode == BFI_INSTR_MULS
				&& instr -> prev -> op1 == op1
				&& instr -> prev -> ad1 == ad1) goto muls;

			fprintf(file, "\tmov\tal, ");
			if(ad2 > 0) fprintf(file, "[si+%zd]\n", ad2);
			else if(ad2 < 0) fprintf(file, "[si-%zd]\n", -ad2);
			else fprintf(file, "[si]\n");
			
			fprintf(file, "\tmov\tcl, %zu\n", op1);
			fprintf(file, "\tmul\tcl\n");

		muls:	fprintf(file, "\tsub\t");
			if(ad1 > 0) fprintf(file, "[si+%zd], al\n", ad1);
			else fprintf(file, "[si-%zd], al\n", -ad1);
			break;

		case BFI_INSTR_MULM:
			if((instr -> prev -> opcode == BFI_INSTR_MULM
				|| instr -> prev -> opcode == BFI_INSTR_MULA)
				&& instr -> prev -> op1 == op1
				&& instr -> prev -> ad1 == ad1) goto mulm;

			fprintf(file, "\tmov\tal, ");
			if(ad2 > 0) fprintf(file, "[si+%zd]\n", ad2);
			else if(ad2 < 0) fprintf(file, "[si-%zd]\n", -ad2);
			else fprintf(file, "[si]\n");
			
			fprintf(file, "\tmov\tcl, %zu\n", op1);
			fprintf(file, "\tmul\tcl\n");

		mulm:	fprintf(file, "\tmov\t");
			if(ad1 > 0) fprintf(file, "[si+%zd], al\n", ad1);
			else fprintf(file, "[si-%zd], al\n", -ad1);
			break;

		case BFI_INSTR_SHLA:
			if(instr -> prev -> opcode == BFI_INSTR_SHLA
				&& instr -> prev -> op2 == op2
				&& instr -> prev -> ad1 == ad1) goto mula;

			fprintf(file, "\tmov\tal, ");
			if(ad2 > 0) fprintf(file, "[si+%zd]\n", ad2);
			else if(ad2 < 0) fprintf(file, "[si-%zd]\n", -ad2);
			else fprintf(file, "[si]\n");
			
			if(op2 == 1) fprintf(file, "\tshl\tal\n");
			else fprintf(file, "\tshl\tal, %zu\n", op2);
			goto mula;

		case BFI_INSTR_SHLS:
			if(instr -> prev -> opcode == BFI_INSTR_SHLS
				&& instr -> prev -> op2 == op2
				&& instr -> prev -> ad1 == ad1) goto muls;

			fprintf(file, "\tmov\tal, ");
			if(ad2 > 0) fprintf(file, "[si+%zd]\n", ad2);
			else if(ad2 < 0) fprintf(file, "[si-%zd]\n", -ad2);
			else fprintf(file, "[si]\n");
			
			if(op2 == 1) fprintf(file, "\tshl\tal\n");
			else fprintf(file, "\tshl\tal, %zu\n", op2);
			goto muls;

		case BFI_INSTR_SHLM:
			if((instr -> prev -> opcode == BFI_INSTR_SHLM
				|| instr -> prev -> opcode == BFI_INSTR_SHLA)
				&& instr -> prev -> op2 == op2
				&& instr -> prev -> ad1 == ad1) goto mulm;

			fprintf(file, "\tmov\tal, ");
			if(ad2 > 0) fprintf(file, "[si+%zd]\n", ad2);
			else if(ad2 < 0) fprintf(file, "[si-%zd]\n", -ad2);
			else fprintf(file, "[si]\n");

			if(op2 == 1) fprintf(file, "\tshl\tal\n");
			else fprintf(file, "\tshl\tal, %zu\n", op2);
			goto mulm;

		case BFI_INSTR_CPYA:
			if(instr -> prev -> opcode == BFI_INSTR_CPYA
				&& instr -> prev -> op1 == op1
				&& instr -> prev -> ad1 == ad1) goto mula;

			fprintf(file, "\tmov\tal, ");
			if(ad2 > 0) fprintf(file, "[si+%zd]\n", ad2);
			else if(ad2 < 0) fprintf(file, "[si-%zd]\n", -ad2);
			else fprintf(file, "[si]\n");
			goto mula;

		case BFI_INSTR_CPYS:
			if(instr -> prev -> opcode == BFI_INSTR_CPYS
				&& instr -> prev -> op1 == op1
				&& instr -> prev -> ad1 == ad1) goto muls;

			fprintf(file, "\tmov\tal, ");
			if(ad2 > 0) fprintf(file, "[si+%zd]\n", ad2);
			else if(ad2 < 0) fprintf(file, "[si-%zd]\n", -ad2);
			else fprintf(file, "[si]\n");
			goto muls;

		case BFI_INSTR_CPYM:
			if((instr -> prev -> opcode == BFI_INSTR_CPYM
				|| instr -> prev -> opcode == BFI_INSTR_CPYA)
				&& instr -> prev -> op1 == op1
				&& instr -> prev -> ad1 == ad1) goto mulm;

			fprintf(file, "\tmov\tal, ");
			if(ad2 > 0) fprintf(file, "[si+%zd]\n", ad2);
			else if(ad2 < 0) fprintf(file, "[si-%zd]\n", -ad2);
			else fprintf(file, "[si]\n");
			goto mulm;

		case BFI_INSTR_SUB:
			fprintf(file, "\n_%zu:\n", op1);
			break;

		case BFI_INSTR_JSR:
			fprintf(file, "\tcall\t_%zu\n", op1);
			break;

		case BFI_INSTR_RTS:
			fprintf(file, "\tret\n");
			break;

		case BFI_INSTR_RET:
			fprintf(file, "\tret\n");
			done_ret = true;
		}
	}

	if(done_ret) goto cells;
	fprintf(file, "\tret\n");

cells:	if(BFo_precomp_output) {
		fprintf(file, "\nheader:\tdb\t");
		BFf_printstr(file, BFo_precomp_output, true);
		fprintf(file, "\n");
	}

	if(BFo_precomp_cells) { fprintf(file, "\ncells:\n"); return; }
	else fprintf(file, "\ncells:\n\tdb\t");

	size_t cols = 16;
	char buf[16] = {0};

	for(size_t i = 0; i < BFo_precomp_cells; i++) {
		cols += sprintf(buf, "%d, ", BFi_mem[i]);

		if(cols < 80) fprintf(file, "%s", buf);
		else {
			fprintf(file, "%d", BFi_mem[i]);
			cols = fprintf(file, "\n\tdb\t") + 13;
		}
	}

	fprintf(file, "0\n");
}