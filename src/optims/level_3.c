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
#include <stdlib.h>

#include "level_2.h"
#include "level_3.h"

#include "../interpreter.h"
#include "../errors.h"
#include "../optims.h"

BFi_instr_t *BFo_optimise_lv3() {
	BFi_instr_t *start = BFo_optimise_lv2();
	BFi_instr_t *instr = start;

	while(instr) switch(instr -> opcode) {
	case BFI_INSTR_NOP: case BFI_INSTR_IFNZ: case BFI_INSTR_ENDIF:
		if(instr -> prev) instr -> prev -> next = instr -> next;
		else start = instr -> next;

		if(instr -> next) instr -> next -> prev = instr -> prev;
		BFi_instr_t *rip = instr;
		instr = instr -> next;
		free(rip);
		break;

	case BFI_INSTR_MULA: case BFI_INSTR_MULS:
	case BFI_INSTR_SHLA: case BFI_INSTR_SHLS:
	case BFI_INSTR_CPYA: case BFI_INSTR_CPYS:
		if(instr -> ad < 0 && instr -> ad < -BFo_mem_padding)
			BFo_mem_padding = -instr -> ad;

		goto end;

	default: end:
		instr = instr -> next;
	}

	return start;
}