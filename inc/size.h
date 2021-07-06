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

#ifndef SIZE_H
#define SIZE_H 1

#define LINE_SIZE 4096
#define DEF_CODE_SIZE 65536

#define MEM_SIZE 32768
#define MEM_SIZE_DIGITS 4
#define MEM_SIZE_PRI "%04zx"

#define FILENAME_SIZE 4096
#define FILENAME_SCN "%4095c"

extern size_t code_size;

#endif