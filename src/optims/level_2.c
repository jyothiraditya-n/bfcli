/* Bfcli: The Interactive Brainfuck Command-Line Interpreter
 * Copyright (C) 2021 Jyothiraditya Nellakra
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
#include <sys/types.h>

#include "level_1.h"
#include "level_2.h"

#include "../interpreter.h"
#include "../errors.h"

#define IS_2_POW(X) !(X & (X - 1))

static size_t log_b2(size_t val) {
	size_t count = 0;
	while(val) { val >>= 1; count++; }
	return count - 1;
}

static void conv_loop(BFi_instr_t *start, BFi_instr_t *end, bool compl);
static BFi_instr_t *conv_instr(BFi_instr_t *start, BFi_instr_t *end,
			       BFi_instr_t *instr);

BFi_instr_t *BFo_optimise_lv2() {
	BFi_instr_t *start = BFo_optimise_lv1();

	BFi_instr_t *init = NULL;
	size_t per_cycle = 0;

	for(BFi_instr_t *i = start; i; i = i -> next) {
		size_t op1 = i -> op1;
		ssize_t ad = i -> ad;

		switch(i -> opcode) {
		case BFI_INSTR_INC:
			if(!ad) per_cycle += op1;
			break;

		case BFI_INSTR_DEC:
			if(!ad) per_cycle -= op1;
			break;

		case BFI_INSTR_FWD:
		case BFI_INSTR_BCK:
		case BFI_INSTR_INP:
		case BFI_INSTR_OUT:
			init = NULL; break;

		case BFI_INSTR_LOOP:
			per_cycle = 0;
			init = i;
			break;

		case BFI_INSTR_ENDL:
			if(init) switch(per_cycle) {
				case 1: conv_loop(init, i, true); break;
				case -1: conv_loop(init, i, false);
			}

			init = NULL;
			break;
		}
	}

	for(BFi_instr_t *i = start; i; i = i -> next) {
		switch(i -> opcode) {
		case BFI_INSTR_MOV:
			if(i -> next -> ad) continue;

			switch(i -> next -> opcode) {
			case BFI_INSTR_INC:
				i -> op1 = i -> next -> op1;
				break;

			case BFI_INSTR_DEC:
				i -> op1 = 256 - i -> next -> op1;
				break;

			default:
				continue;
			}

			BFi_instr_t *next = i -> next -> next;
			free(i -> next);

			i -> next = next;
			next -> prev = i;
		}
	}

	return start;
}

static void conv_loop(BFi_instr_t *start, BFi_instr_t *end, bool compl) {
	BFi_instr_t *new = malloc(sizeof(BFi_instr_t));
	if(!new) BFe_report_err(BFE_UNKNOWN_ERROR);

	BFi_instr_t *start_new = new;
	new -> op1 = 0; new -> op2 = 0;

	if(compl) new -> opcode = BFI_INSTR_CMPL;
	else new -> opcode = BFI_INSTR_NOP;

	for(BFi_instr_t *i = start -> next; i != end; i = i -> next)
		if(i -> ad) new = conv_instr(start_new, new, i);

	for(BFi_instr_t *i = start -> next; i != end;) {
		BFi_instr_t *instr = i;
		i = instr -> next;
		free(instr);
	}

	if(new -> opcode == BFI_INSTR_NOP || new -> opcode == BFI_INSTR_CMPL) {
		start -> next = end;
		end -> prev = start;

		start -> opcode = BFI_INSTR_NOP;
		end -> opcode = BFI_INSTR_MOV;
		end -> ad = 0; end -> op1 = 0;

		free(new);
		return;
	}

	new -> next = malloc(sizeof(BFi_instr_t));
	if(!new -> next) BFe_report_err(BFE_UNKNOWN_ERROR);

	new -> next -> prev = new;
	new = new -> next;

	start -> opcode = BFI_INSTR_IFNZ;
	new -> opcode = BFI_INSTR_ENDIF;
	new -> op1 = end -> op1;

	end -> opcode = BFI_INSTR_MOV;
	end -> op1 = 0; end -> ad = 0;

	start -> next = start_new;
	end -> prev = new;

	start_new -> prev = start;
	new -> next = end;
}

static BFi_instr_t *conv_instr(BFi_instr_t *start, BFi_instr_t *end,
			       BFi_instr_t *instr)
{
	size_t op = instr -> op1;
	ssize_t ad = instr -> ad;

	int opcode = BFI_INSTR_NOP;
	size_t op1 = 0, op2 = 0;

	BFi_instr_t *i;
	switch(instr -> opcode) {
	case BFI_INSTR_INC:
		if(IS_2_POW(op) && op != 1) {
			for(i = start; i -> op2 != op && i != end;
				i = i -> next);

			opcode = BFI_INSTR_SHLA;
			op2 = log_b2(op); op1 = 0;
			break;
		}

		for(i = start; i -> op1 != op && i != end; i = i -> next);

		opcode = op == 1 ? BFI_INSTR_CPYA : BFI_INSTR_MULA;
		op1 = op; op2 = 0;
		break;

	case BFI_INSTR_DEC:
		if(IS_2_POW(op) && op != 1) {
			for(i = start; i -> op2 != op && i != end;
				i = i -> next);

			opcode = BFI_INSTR_SHLS;
			op2 = log_b2(op); op1 = 0;
			break;
		}

		for(i = start; i -> op1 != op && i != end; i = i -> next);

		opcode = op == 1 ? BFI_INSTR_CPYS : BFI_INSTR_MULS;
		op1 = op; op2 = 0;
		break;

	default:
		return end;
	}

	BFi_instr_t *new = malloc(sizeof(BFi_instr_t));
	if(!new) BFe_report_err(BFE_UNKNOWN_ERROR);

	new -> prev = i; new -> next = i -> next;
	new -> opcode = opcode; new -> ad = ad;
	new -> op1 = op1; new -> op2 = op2; 

	if(i -> next) i -> next -> prev = new;
	i -> next = new;

	if(i == end) return new;
	else return end;
}