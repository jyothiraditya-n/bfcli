/* BFCLI: The Interactive Brainfuck Command-Line Interpreter
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

#include "bfir.h"

#include "../files.h"
#include "../interpreter.h"
#include "../optims.h"

void BFa_bfir_t(FILE *file) {
	fprintf(file, "#0:");

	if(BFo_mem_padding) fprintf(file, "\tpad\t%zu\n", BFo_mem_padding);

	if(BFo_precomp_cells) {
		fprintf(file, "\tinit\t");

		size_t cols = 16;
		char buf[16] = {0};

		for(size_t i = 0; i < BFo_precomp_cells; i++) {
			cols += sprintf(buf, "%d, ", BFi_mem[i]);
			if(cols < 80) fprintf(file, "%s", buf);
			else cols = fprintf(file, "\n\t\t%s", buf) + 13; 
		}

		cols += sprintf(buf, "%%%zd", BFo_precomp_ptr);
		if(cols < 80) fprintf(file, "%s\n", buf);
		else cols = fprintf(file, "\n\t\t%s\n", buf) + 13;
	}

	if(BFo_precomp_output) {
		fprintf(file, "\tprint\t");
		BFf_printstr(file, BFo_precomp_output, true);
		fprintf(file, "\n");
	}

	for(BFi_instr_t *instr = BFi_code; instr; instr = instr -> next) {
		size_t op1 = instr -> op1, op2 = instr -> op2;
		ssize_t ad1 = instr -> ad1, ad2 = instr -> ad2;

		switch(instr -> opcode) {
		case BFI_INSTR_INC:
			fprintf(file, "\tinc\t%%%zd, %zu\n", ad1, op1);
			break;

		case BFI_INSTR_DEC:
			fprintf(file, "\tdec\t%%%zd, %zu\n", ad1, op1);
			break;

		case BFI_INSTR_CMPL:
			fprintf(file, "\tcmpl\t%%%zd\n", ad1);
			break;

		case BFI_INSTR_MOV:
			fprintf(file, "\tmov\t%%%zd, %zu\n", ad1, op1);
			break;

		case BFI_INSTR_FWD:
			fprintf(file, "\tfwd\t%zd\n", op1);
			break;

		case BFI_INSTR_BCK:
			fprintf(file, "\tbck\t%zd\n", op1);
			break;

		case BFI_INSTR_INP:
			fprintf(file, "\tinp\t%%%zd\n", ad1);
			break;

		case BFI_INSTR_OUT:
			fprintf(file, "\tout\t%%%zd\n", ad1);
			break;

		case BFI_INSTR_LOOP:
			fprintf(file, "\tloop\t#%zu\n", op1);
			break;

		case BFI_INSTR_ENDL:
			fprintf(file, "\tendl\t#%zu\n", op1);
			break;

		case BFI_INSTR_IFNZ:
			fprintf(file, "\tifnz\t#%zu\n", op1);
			break;

		case BFI_INSTR_ENDIF:
			fprintf(file, "\tendif\t#%zu\n", op1);
			break;

		case BFI_INSTR_MULA:
			fprintf(file, "\tmula\t%%%zd, %%%zd, %zu\n",
				ad1, ad2, op1);
			break;

		case BFI_INSTR_MULS:
			fprintf(file, "\tmuls\t%%%zd, %%%zd, %zu\n",
				ad1, ad2, op1);
			break;

		case BFI_INSTR_MULM:
			fprintf(file, "\tmulm\t%%%zd, %%%zd, %zu\n",
				ad1, ad2, op1);
			break;

		case BFI_INSTR_SHLA:
			fprintf(file, "\tshla\t%%%zd, %%%zd, %zu\n",
				ad1, ad2, op2);
			break;

		case BFI_INSTR_SHLS:
			fprintf(file, "\tshls\t%%%zd, %%%zd, %zu\n",
				ad1, ad2, op2);
			break;

		case BFI_INSTR_SHLM:
			fprintf(file, "\tshlm\t%%%zd, %%%zd, %zu\n",
				ad1, ad2, op2);
			break;

		case BFI_INSTR_CPYA:
			fprintf(file, "\tcpya\t%%%zd, %%%zd\n", ad1, ad2);
			break;

		case BFI_INSTR_CPYS:
			fprintf(file, "\tcpys\t%%%zd, %%%zd\n", ad1, ad2);
			break;

		case BFI_INSTR_CPYM:
			fprintf(file, "\tcpym\t%%%zd, %%%zd\n", ad1, ad2);
			break;


		case BFI_INSTR_SUB:
			fprintf(file, "\n#%zu:", op1);
			break;

		case BFI_INSTR_JSR:
			fprintf(file, "\tjsr\t#%zu\n", op1);
			break;

		case BFI_INSTR_RTS:
			fprintf(file, "\trts\n");
			break;

		case BFI_INSTR_RET:
			fprintf(file, "\tret\n");
			break;
		}
	}
}