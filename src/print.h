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

#define UNKNOWN_ERROR 0
#define BAD_ARGS 1
#define CODE_TOO_LONG 2
#define LINE_TOO_LONG 3
#define NESTED_BRACES 4
#define BAD_BRACKETS 5
#define BAD_FILE 6
#define BAD_CODE 7

extern const char *progname;
extern bool colour;

extern void print_about();
extern void print_banner();
extern void print_error(int errnum);
extern void print_help();
extern void print_mem();
extern void print_prompt(size_t insertion_point);