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

#include "z80.h"

#include "../clidata.h"
#include "../errors.h"
#include "../files.h"
#include "../interpreter.h"
#include "../main.h"
#include "../optims.h"
#include "../translator.h"

void BFa_z80_t(FILE *file) {
	fprintf(file, "\torg\t8000h\n");
	fprintf(file, "\tdb\t\"AB\"\n");
	fprintf(file, "\tdw\tinit, 0, 0, 0, 0, 0, 0\n\n");

	fprintf(file, "init:\n");
	/*fprintf(file, "\tld\thl, 0f672h\n");
	fprintf(file, "\tld\tde, 0fc48h\n\n");

	fprintf(file, ".loop:\n");
	fprintf(file, "\tcall\t0020h\n");
	fprintf(file, "\tjr\tz, main\n");
	fprintf(file, "\tld\t(hl), 0\n");
	fprintf(file, "\tinc\thl\n");
	fprintf(file, "\tjr\t.loop\n\n");

	fprintf(file, "main:\n");*/
	fprintf(file, "\tld\thl, 0f672h\n");

	for(BFi_instr_t *instr = BFi_code; instr; instr = instr -> next) {
		size_t op1 = instr -> op1;

		switch(instr -> opcode) {
		case BFI_INSTR_INC:
			if(op1 != 1) {
				fprintf(file, "\tinc\t(hl)\n");
				break;
			}

			fprintf(file, "\tld\ta, (hl)\n");
			fprintf(file, "\tadd\ta, %zu\n", op1);
			fprintf(file, "\tld\t(hl), a\n");
			break;

		case BFI_INSTR_DEC:
			if(op1 != 1) {
				fprintf(file, "\tdec\t(hl)\n");
				break;
			}

			fprintf(file, "\tld\ta, (hl)\n");
			fprintf(file, "\tsub\ta, %zu\n", op1);
			fprintf(file, "\tld\t(hl), a\n");
			break;

		case BFI_INSTR_FWD:
			if(op1 == 1) {
				fprintf(file, "\tinc\thl\n");
				break;
			}

			fprintf(file, "\tld\tde, %zu\n", op1);
			fprintf(file, "\tadd\thl, de\n");
			break;

		case BFI_INSTR_BCK:
			if(op1 == 1) {
				fprintf(file, "\tdec\thl\n");
				break;
			}

			fprintf(file, "\tld\tde, %zu\n", 65536 - op1);
			fprintf(file, "\tadd\thl, de\n");
			break;

		case BFI_INSTR_INP:
			fprintf(file, "\tcall\t009fh\n");
			fprintf(file, "\tld\t(hl), a\n");
			break;

		case BFI_INSTR_OUT:
			fprintf(file, "\tld\ta, (hl)\n");
			fprintf(file, "\tcall\t00a2h\n");
			break;

		case BFI_INSTR_LOOP:
			fprintf(file, "\n.l%zu:\n", op1);
			fprintf(file, "\tld\ta, (hl)\n");
			fprintf(file, "\tand\ta\n");
			fprintf(file, "\tjp\tz, .e%zu\n", op1);
			break;

		case BFI_INSTR_ENDL:
			fprintf(file, "\tjp\t.l%zu\n", op1);
			fprintf(file, "\n.e%zu:\n", op1);
			break;

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
			fprintf(file, "\tjp\tend\n");
		}
	}

	fprintf(file, "\nend:\n");
	fprintf(file, "\tjr\tend\n\n");

	fprintf(file, "\nbuf:\n");
	fprintf(file, "\tds\t8000h + 4000h - buf, 255\n");
}