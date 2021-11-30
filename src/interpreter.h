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

#include <stdbool.h>
#include <stddef.h>

#ifndef BF_INTERPRETER_H
#define BF_INTERPRETER_H 1

typedef struct BFi_instr_s {
	struct BFi_instr_s *prev, *next;
	int opcode;

	union {
		struct BFi_instr_s *ptr;
		size_t value;

	} operand;

	#define BFI_INSTR_NOP 0

	#define BFI_INSTR_INC 1
	#define BFI_INSTR_DEC 2
	#define BFI_INSTR_FWD 3
	#define BFI_INSTR_BCK 4
	#define BFI_INSTR_INP 5
	#define BFI_INSTR_OUT 6
	#define BFI_INSTR_JMP 7
	#define BFI_INSTR_JZ 8

	#define BFI_INSTR_HELP 9
	#define BFI_INSTR_INIT 10
	#define BFI_INSTR_MEM_PEEK 11
	#define BFI_INSTR_MEM_DUMP 12
	#define BFI_INSTR_EXEC 13
	#define BFI_INSTR_EDIT 14
	#define BFI_INSTR_COMP 15

} BFi_instr_t;

extern char *BFi_program_str;
extern BFi_instr_t *BFi_program_code;
extern size_t BFi_code_size;

extern bool Bfi_do_recompile;
extern bool BFi_is_running;
extern char BFi_last_output;

extern unsigned char *BFi_mem;
extern size_t BFi_mem_ptr;
extern size_t BFi_mem_size;

extern void BFi_init();
extern void BFi_main(char *command_str);

extern void BFi_compile();
extern void BFi_exec();

#endif