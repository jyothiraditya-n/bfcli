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

#ifndef BF_OPTIMS_H
#define BF_OPTIMS_H 1

extern char BFo_level;
extern ssize_t BFo_mem_padding;
extern bool BFo_advanced_ops;

extern size_t BFo_sub_count;
extern size_t BFo_max_subs;

extern unsigned char *BFo_precomp_output;
extern size_t BFo_precomp_cells;
extern size_t BFo_precomp_ptr;

extern void BFo_optimise();

#endif