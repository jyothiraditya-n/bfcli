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

#ifndef BF_MAIN_H
#define BF_MAIN_H 1

#define BF_VERSION 8
#define BF_SUBVERSION 2
#define BF_VERNAME "The Optimisation of Nonsense"

#define BF_LINE_SIZE 4096
#define BF_CODE_SIZE 65536
#define BF_MEM_SIZE 32768

#define BF_MEM_SIZE_DIGITS 8
#define BF_MEM_SIZE_PRI "%08zx"

#define BF_FILENAME_SIZE 4096
#define BF_FILENAME_SCN "%4095c"

extern size_t BFm_insertion_point;

#endif