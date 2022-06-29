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

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include "level_3.h"
#include "level_4.h"

#include "../interpreter.h"
#include "../errors.h"
#include "../optims.h"

bool evaluate(BFi_instr_t *start, ssize_t ad);

BFi_instr_t *BFo_optimise_lv4() {
	BFi_instr_t *start = BFo_optimise_lv3();
	if(!BFo_advanced_ops) return start;

	for(BFi_instr_t *instr = start; instr; instr = instr -> next) {
	loop:	if(instr -> opcode == BFI_INSTR_MOV && !instr -> op1)
			if(evaluate(instr, instr -> ad1))
		{
			if(start == instr) start = start -> next;

			if(instr -> prev) instr -> prev -> next = instr -> next;
			if(instr -> next) instr -> next -> prev = instr -> prev;

			BFi_instr_t *rip = instr;
			instr = instr -> next;
			free(rip);

			if(instr) goto loop;
			else break;
		}
	}

	for(size_t i = 0; i < BFi_mem_size; i++) {
		ssize_t ad = i;

		for(BFi_instr_t *instr = start; instr; instr = instr -> next) {
		l3:	switch(instr -> opcode) {
			case BFI_INSTR_INC:
				if(instr -> ad1 != ad) break;
				instr -> opcode = BFI_INSTR_MOV;
				goto l2;

			case BFI_INSTR_DEC:
				if(instr -> ad1 != ad) break;

				instr -> opcode = BFI_INSTR_MOV;
				instr -> op1 = 256 - instr -> op1;
				goto l2;

			case BFI_INSTR_CMPL:
				if(instr -> ad1 != ad) break;
				
				if(instr -> prev)
					instr -> prev -> next = instr -> next;

				if(instr -> next)
					instr -> next -> prev = instr -> prev;

				BFi_instr_t *rip = instr;
				instr = instr -> next;
				free(rip);

				if(instr) goto l3;
				else break;

			case BFI_INSTR_MOV:
				if(instr -> ad1 == ad) goto l2;
				else break;

			case BFI_INSTR_FWD:
				ad += instr -> op1;
				break;

			case BFI_INSTR_BCK:
				ad -= instr -> op1;
				break;

			case BFI_INSTR_INP:
				if(instr -> ad1 == ad) goto l2;
				else break;

			case BFI_INSTR_LOOP: case BFI_INSTR_ENDL:
				goto l2;

			default:
				if(instr -> ad2 == ad) {
					if(instr -> prev)
						instr -> prev -> next
						= instr -> next;

					if(instr -> next)
						instr -> next -> prev
						= instr -> prev;

					BFi_instr_t *rip = instr;
					instr = instr -> next;
					free(rip);

					if(instr) goto l3;
					else break;
				}

				if(instr -> ad1 != ad) break;

				switch(instr -> opcode) {
				case BFI_INSTR_MULA:
					instr -> opcode = BFI_INSTR_MULM;
					break;

				case BFI_INSTR_SHLA:
					instr -> opcode = BFI_INSTR_SHLM;
					break;

				case BFI_INSTR_CPYA:
					instr -> opcode = BFI_INSTR_CPYM;
				}

				goto l2;
			}

		}

	l2: continue;
	}

	return start;
}

bool evaluate(BFi_instr_t *start, ssize_t ad) {
	bool ret = true;

	for(BFi_instr_t *instr = start -> next; instr; instr = instr -> next) {
	loop:	switch(instr -> opcode) {
		case BFI_INSTR_INC:
			if(instr -> ad1 != ad) break;

			instr -> opcode = BFI_INSTR_MOV;
			return ret;

		case BFI_INSTR_DEC:
			if(instr -> ad1 != ad) break;

			instr -> opcode = BFI_INSTR_MOV;
			instr -> op1 = 256 - instr -> op1;
			return ret;

		case BFI_INSTR_CMPL:
			if(instr -> ad1 != ad) break;
			
			if(instr -> prev) instr -> prev -> next = instr -> next;
			if(instr -> next) instr -> next -> prev = instr -> prev;
			BFi_instr_t *rip = instr;
			instr = instr -> next;
			free(rip);

			if(instr) goto loop;
			else break;

		case BFI_INSTR_OUT:
			if(instr -> ad1 == ad) ret = false;
			else break;

		case BFI_INSTR_MOV:
			if(instr -> ad1 == ad) return ret;
			else break;

		case BFI_INSTR_FWD:
			ad += instr -> op1;
			break;

		case BFI_INSTR_BCK:
			ad -= instr -> op1;
			break;

		case BFI_INSTR_INP:
			if(instr -> ad1 == ad) return ret;
			else break;

		case BFI_INSTR_LOOP: case BFI_INSTR_ENDL:
			return false;

		default:
			if(instr -> ad2 == ad) {
				if(instr -> prev)
					instr -> prev -> next = instr -> next;

				if(instr -> next)
					instr -> next -> prev = instr -> prev;

				BFi_instr_t *rip = instr;
				instr = instr -> next;
				free(rip);

				if(instr) goto loop;
				else break;
			}

			if(instr -> ad1 != ad) break;

			switch(instr -> opcode) {
			case BFI_INSTR_MULA:
				instr -> opcode = BFI_INSTR_MULM;
				break;

			case BFI_INSTR_SHLA:
				instr -> opcode = BFI_INSTR_SHLM;
				break;

			case BFI_INSTR_CPYA:
				instr -> opcode = BFI_INSTR_CPYM;
			}

			return ret;
		}
	}

	return ret;
}