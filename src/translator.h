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
#include <stdint.h>

#include <sys/types.h>

#ifndef BF_TRANSLATOR_H
#define BF_TRANSLATOR_H 1

extern bool BFt_compile;
extern bool BFt_translate;
extern bool BFt_standalone;
extern char BFt_target_arch[];
extern int BFt_optim_lvl;

typedef struct BFt_instr_s {
	struct BFt_instr_s *prev, *next;

	int opcode;
	size_t op1, op2;
	ssize_t ad1;

	#define BFT_INSTR_NOP 0

	#define BFT_INSTR_INC 1
	#define BFT_INSTR_DEC 2

	#define BFT_INSTR_FWD 3
	#define BFT_INSTR_BCK 4

	#define BFT_INSTR_INP 5
	#define BFT_INSTR_OUT 6
	#define BFT_INSTR_LOOP 7
	#define BFT_INSTR_ENDL 8

	#define BFT_INSTR_IFNZ 9
	#define BFT_INSTR_ENDIF 10

	#define BFT_INSTR_CMPL 11
	#define BFT_INSTR_MOV 12

	#define BFT_INSTR_MULA 13
	#define BFT_INSTR_MULS 14

	#define BFT_INSTR_SHLA 15
	#define BFT_INSTR_SHLS 16

	#define BFT_INSTR_CPYA 17
	#define BFT_INSTR_CPYS 18

} BFt_instr_t;

extern BFt_instr_t *BFt_code;

extern void BFt_convert_file();
extern void BFt_optimise();

#endif