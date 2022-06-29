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

	fprintf(file, "init:\n");
	fprintf(file, "\txor\tbx, bx\n");
	fprintf(file, "\tmov\tsi, cells\n\n");

	fprintf(file, ".loop:\n");
	fprintf(file, "\tmov\tbyte [si], 0\n");
	fprintf(file, "\tinc\tsi\n");
	fprintf(file, "\tcmp\tsi, sp\n");
	fprintf(file, "\tje\tmain\n");
	fprintf(file, "\tjmp\t.loop\n\n");

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

	for(BFi_instr_t *instr = BFi_code; instr; instr = instr -> next) {
		if(instr -> opcode != BFI_INSTR_OUT) continue;

		fprintf(file, "output:\n");
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
	fprintf(file, "\tmov\tsi, cells\n");

	bool done_ret = false;

	for(BFi_instr_t *instr = BFi_code; instr; instr = instr -> next) {
		size_t op1 = instr -> op1, op2 = instr -> op2;
		ssize_t ad1 = instr -> ad1, ad2 = instr -> ad2;

		switch(instr -> opcode) {
		case BFI_INSTR_INC:
			if(op1 != 1) fprintf(file, "\tadd\t");
			else fprintf(file, "\tinc\t");

			if(ad1 > 0) fprintf(file, "byte [si + %zd]", ad1);
			else if(ad1 < 0) fprintf(file, "byte [si - %zd]", -ad1);
			else fprintf(file, "byte [si]");

			if(op1 != 1) fprintf(file, ", %zu\n", op1);
			else fprintf(file, "\n");
			break;

		case BFI_INSTR_DEC:
			if(op1 != 1) fprintf(file, "\tsub\t");
			else fprintf(file, "\tdec\t");

			if(ad1 > 0) fprintf(file, "byte [si + %zd]", ad1);
			else if(ad1 < 0) fprintf(file, "byte [si - %zd]", -ad1);
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
			if(ad1 > 0) fprintf(file, "[si + %zd], ", ad1);
			else if(ad1 < 0) fprintf(file, "[si - %zd], ", -ad1);
			else fprintf(file, "[si], ");
			fprintf(file, "al\n");
			break;

		case BFI_INSTR_OUT:
			fprintf(file, "\tmov\tal, ");
			if(ad1 > 0) fprintf(file, "[si + %zd]\n", ad1);
			else if(ad1 < 0) fprintf(file, "[si - %zd]\n", -ad1);
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
			fprintf(file, "\tneg\t[si]\n");
			break;

		case BFI_INSTR_MOV:
			fprintf(file, "\tmov\tbyte ");
			if(ad1 > 0) fprintf(file, "[si + %zd], ", ad1);
			else if(ad1 < 0) fprintf(file, "[si - %zd], ", -ad1);
			else fprintf(file, "[si], ");
			fprintf(file, "%zu\n", op1);
			break;

		case BFI_INSTR_MULA:
			if(instr -> prev -> opcode == BFI_INSTR_MULA
				&& instr -> prev -> op1 == op1
				&& instr -> prev -> ad1 == ad1) goto mula;

			fprintf(file, "\tmov\tal, ");
			if(ad2 > 0) fprintf(file, "[si + %zd]\n", ad2);
			else if(ad2 < 0) fprintf(file, "[si - %zd]\n", -ad2);
			else fprintf(file, "[si]\n");
			
			fprintf(file, "\tmov\tcl, %zu\n", op1);
			fprintf(file, "\tmul\tcl\n");

		mula:	fprintf(file, "\tadd\t");
			if(ad1 > 0) fprintf(file, "[si + %zd], al\n", ad1);
			else fprintf(file, "[si - %zd], al\n", -ad1);
			break;

		case BFI_INSTR_MULS:
			if(instr -> prev -> opcode == BFI_INSTR_MULS
				&& instr -> prev -> op1 == op1
				&& instr -> prev -> ad1 == ad1) goto muls;

			fprintf(file, "\tmov\tal, ");
			if(ad2 > 0) fprintf(file, "[si + %zd]\n", ad2);
			else if(ad2 < 0) fprintf(file, "[si - %zd]\n", -ad2);
			else fprintf(file, "[si]\n");
			
			fprintf(file, "\tmov\tcl, %zu\n", op1);
			fprintf(file, "\tmul\tcl\n");

		muls:	fprintf(file, "\tsub\t");
			if(ad1 > 0) fprintf(file, "[si + %zd], al\n", ad1);
			else fprintf(file, "[si - %zd], al\n", -ad1);
			break;

		case BFI_INSTR_MULM:
			if((instr -> prev -> opcode == BFI_INSTR_MULM
				|| instr -> prev -> opcode == BFI_INSTR_MULA)
				&& instr -> prev -> op1 == op1
				&& instr -> prev -> ad1 == ad1) goto mulm;

			fprintf(file, "\tmov\tal, ");
			if(ad2 > 0) fprintf(file, "[si + %zd]\n", ad2);
			else if(ad2 < 0) fprintf(file, "[si - %zd]\n", -ad2);
			else fprintf(file, "[si]\n");
			
			fprintf(file, "\tmov\tcl, %zu\n", op1);
			fprintf(file, "\tmul\tcl\n");

		mulm:	fprintf(file, "\tmov\t");
			if(ad1 > 0) fprintf(file, "[si + %zd], al\n", ad1);
			else fprintf(file, "[si - %zd], al\n", -ad1);
			break;

		case BFI_INSTR_SHLA:
			if(instr -> prev -> opcode == BFI_INSTR_SHLA
				&& instr -> prev -> op2 == op2
				&& instr -> prev -> ad1 == ad1) goto mula;

			fprintf(file, "\tmov\tal, ");
			if(ad2 > 0) fprintf(file, "[si + %zd]\n", ad2);
			else if(ad2 < 0) fprintf(file, "[si - %zd]\n", -ad2);
			else fprintf(file, "[si]\n");
			
			fprintf(file, "\tshl\tal, %zu\n", op2);
			goto mula;

		case BFI_INSTR_SHLS:
			if(instr -> prev -> opcode == BFI_INSTR_SHLS
				&& instr -> prev -> op2 == op2
				&& instr -> prev -> ad1 == ad1) goto muls;

			fprintf(file, "\tmov\tal, ");
			if(ad2 > 0) fprintf(file, "[si + %zd]\n", ad2);
			else if(ad2 < 0) fprintf(file, "[si - %zd]\n", -ad2);
			else fprintf(file, "[si]\n");
			
			fprintf(file, "\tshl\tal, %zu\n", op2);
			goto muls;

		case BFI_INSTR_SHLM:
			if((instr -> prev -> opcode == BFI_INSTR_SHLM
				|| instr -> prev -> opcode == BFI_INSTR_SHLA)
				&& instr -> prev -> op2 == op2
				&& instr -> prev -> ad1 == ad1) goto mulm;

			fprintf(file, "\tmov\tal, ");
			if(ad2 > 0) fprintf(file, "[si + %zd]\n", ad2);
			else if(ad2 < 0) fprintf(file, "[si - %zd]\n", -ad2);
			else fprintf(file, "[si]\n");
			
			fprintf(file, "\tshl\tal, %zu\n", op2);
			goto mulm;

		case BFI_INSTR_CPYA:
			if(instr -> prev -> opcode == BFI_INSTR_CPYA
				&& instr -> prev -> op1 == op1
				&& instr -> prev -> ad1 == ad1) goto mula;

			fprintf(file, "\tmov\tal, ");
			if(ad2 > 0) fprintf(file, "[si + %zd]\n", ad2);
			else if(ad2 < 0) fprintf(file, "[si - %zd]\n", -ad2);
			else fprintf(file, "[si]\n");
			goto mula;

		case BFI_INSTR_CPYS:
			if(instr -> prev -> opcode == BFI_INSTR_CPYS
				&& instr -> prev -> op1 == op1
				&& instr -> prev -> ad1 == ad1) goto muls;

			fprintf(file, "\tmov\tal, ");
			if(ad2 > 0) fprintf(file, "[si + %zd]\n", ad2);
			else if(ad2 < 0) fprintf(file, "[si - %zd]\n", -ad2);
			else fprintf(file, "[si]\n");
			goto muls;

		case BFI_INSTR_CPYM:
			if((instr -> prev -> opcode == BFI_INSTR_CPYM
				|| instr -> prev -> opcode == BFI_INSTR_CPYA)
				&& instr -> prev -> op1 == op1
				&& instr -> prev -> ad1 == ad1) goto mulm;

			fprintf(file, "\tmov\tal, ");
			if(ad2 > 0) fprintf(file, "[si + %zd]\n", ad2);
			else if(ad2 < 0) fprintf(file, "[si - %zd]\n", -ad2);
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

cells:	
	fprintf(file, "\ncells:\n");
}