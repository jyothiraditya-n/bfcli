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
#include <string.h>

#include "level_3.h"
#include "precomp.h"

#include "../interpreter.h"
#include "../errors.h"
#include "../optims.h"

typedef struct list_s {
	struct list_s *next;
	unsigned char ch;

} list_t;

static list_t output_first;
static size_t output_len = 0;

static int _putchar(int ch);
static void on_fwd(size_t op);
static void on_bck(size_t op);

BFi_instr_t *BFo_optimise_precomp() {
	size_t last = 0, length = strlen(BFi_program_str);
	char *program = BFi_program_str;
	int loops_open = 0;

	for(size_t i = 0; i < length; i++) {
		switch(program[i]) {
			case '[': loops_open++; break;
			case ']': loops_open--; goto next;
			case ',': goto n2;

		default:
		next:	if(!loops_open) last = i;
		}
	}

n2:	if(!last) return BFo_optimise_lv3_2();

	char saved = BFi_program_str[last + 1];
	BFi_program_str[last + 1] = 0;
	BFi_is_running = true;

	if(saved) { BFi_on_fwd = on_fwd; BFi_on_bck = on_bck; }
	BFi_putchar = _putchar;
	BFi_exec();
	
	if(!output_len) goto n3;

	BFo_precomp_output = malloc(sizeof(char) * (output_len + 1));
	if(!BFo_precomp_output) BFe_report_err(BFE_UNKNOWN_ERROR);

	size_t j = 0;
	for(list_t *i = &output_first; i; i = i -> next)
		BFo_precomp_output[j++] = i -> ch;

	BFo_precomp_output[output_len] = 0;

n3:	BFi_program_str[last + 1] = saved;
	BFi_program_str += last + 1;
	BFi_do_recompile = true;

	BFo_precomp_cells++;
	return BFo_optimise_lv3();
}

static int _putchar(int ch) {
	static list_t *last = &output_first;

	last -> ch = ch;
	last -> next = malloc(sizeof(list_t));
	if(!last -> next) BFe_report_err(BFE_UNKNOWN_ERROR);

	last = last -> next;
	last -> next = NULL;
	output_len++;

	return 0;
}

static void on_fwd(size_t op) {
	if(BFi_mem_ptr > BFo_precomp_cells)
		BFo_precomp_cells = BFi_mem_ptr;

	BFo_precomp_ptr += op;
}

static void on_bck(size_t op) {
	BFo_precomp_ptr -= op;
}