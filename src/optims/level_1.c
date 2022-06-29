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

#include <sys/types.h>

#include "level_1.h"

#include "../errors.h"
#include "../interpreter.h"
#include "../optims.h"

static void delete(BFi_instr_t *node, ssize_t offset);
static void insert(BFi_instr_t *node, ssize_t offset);

BFi_instr_t *BFo_optimise_lv1() {
	bool call_delete = false;
	ssize_t offset = 0;

	if(!BFo_advanced_ops) return BFi_code;

	for(BFi_instr_t *instr = BFi_code; instr; instr = instr -> next) {
		if(call_delete) {
			delete(instr -> prev, offset);
			call_delete = false;
		}

		switch(instr -> opcode) {
		case BFI_INSTR_INC:
			instr -> op1 = instr -> op1 % 256;
			instr -> ad1 = offset;
			break;

		case BFI_INSTR_DEC:
			instr -> op1 = instr -> op1 % 256;
			instr -> ad1 = offset;
			break;

		case BFI_INSTR_FWD:
			offset += instr -> op1;
			goto del;

		case BFI_INSTR_BCK:
			offset -= instr -> op1;
		del:    call_delete = true;
			break;

		case BFI_INSTR_INP:
			instr -> ad1 = offset;
			break;

		case BFI_INSTR_OUT:
			instr -> ad1 = offset;
			break;

		case BFI_INSTR_LOOP: case BFI_INSTR_ENDL:
			insert(instr -> prev, offset);
			offset = 0;
		}
	}

	return BFi_code;
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