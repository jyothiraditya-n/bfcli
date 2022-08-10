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

#include "level_2.h"
#include "level_3.h"

#include "../interpreter.h"
#include "../errors.h"
#include "../optims.h"

static void delete(BFi_instr_t *node, ssize_t offset);
static void insert(BFi_instr_t *node, ssize_t offset);

static bool evaluate(BFi_instr_t *start, ssize_t ad);

BFi_instr_t *BFo_optimise_lv3() {
	BFi_instr_t *start = BFo_optimise_lv2();
	BFi_instr_t *instr = start;

	if(!BFo_advanced_ops) return start;

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
		if(instr -> ad1 < 0 && instr -> ad1 < -BFo_mem_padding)
			BFo_mem_padding = -instr -> ad1;

		goto end;

	default: end:
		instr = instr -> next;
	}

	instr = start;

	bool call_delete = false;
	ssize_t offset = 0;

	for(; instr; instr = instr -> next) {
		if(call_delete) {
			delete(instr -> prev, offset);
			call_delete = false;
		}

		switch(instr -> opcode) {
		case BFI_INSTR_INC: case BFI_INSTR_DEC:
		case BFI_INSTR_INP: case BFI_INSTR_OUT:
			instr -> ad1 += offset;
			break;

		case BFI_INSTR_MOV:
			instr -> ad1 = offset;
			break;

		case BFI_INSTR_MULA: case BFI_INSTR_MULS:
		case BFI_INSTR_SHLA: case BFI_INSTR_SHLS:
		case BFI_INSTR_CPYA: case BFI_INSTR_CPYS:
			instr -> ad1 += offset;
			instr -> ad2 = offset;
			break;

		case BFI_INSTR_FWD:
			offset += instr -> op1;
			goto del;

		case BFI_INSTR_BCK:
			offset -= instr -> op1;
		del:	call_delete = true;
			break;

		case BFI_INSTR_LOOP: case BFI_INSTR_ENDL:
			insert(instr -> prev, offset);
			offset = 0;
		}
	}

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

	return start;
}

BFi_instr_t *BFo_optimise_lv3_2() {
	BFi_instr_t *start = BFo_optimise_lv3();

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

static void delete(BFi_instr_t *node, ssize_t offset) {
	switch(node -> next -> opcode) {
	case BFI_INSTR_LOOP: case BFI_INSTR_ENDL:
		if(offset > 0) {
			node -> opcode = BFI_INSTR_FWD;
			node -> op1 = offset;
			break;
		}

		else if(offset < 0) {
			node -> opcode = BFI_INSTR_BCK;
			node -> op1 = -offset;
			break;
		}

		else goto del;

	default:
	del:    if(node == BFi_code) BFi_code = node -> next;
		if(node -> prev) node -> prev -> next = node -> next;
		node -> next -> prev = node -> prev;
		free(node);
	}
}

static void insert(BFi_instr_t *node, ssize_t offset) {
	if(!node || !offset) return;
	BFi_instr_t *new;

	switch(node -> opcode) {
	case BFI_INSTR_FWD: case BFI_INSTR_BCK:
		break;

	default:
		new = malloc(sizeof(BFi_instr_t));
		if(!new) BFe_report_err(BFE_UNKNOWN_ERROR);

		new -> next = node -> next;
		new -> prev = node;

		node -> next -> prev = new;
		node -> next = new;

		new -> opcode = offset > 0 ? BFI_INSTR_FWD : BFI_INSTR_BCK;
		new -> op1 = offset > 0 ? offset : -offset;

		new -> ptr = NULL;
		new -> op2 = 0;
		new -> ad1 = 0;
		new -> ad2 = 0;
	}
}

static bool evaluate(BFi_instr_t *start, ssize_t ad) {
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
			break;

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