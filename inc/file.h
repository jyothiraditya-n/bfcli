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

#include <termios.h>
#include <unistd.h>

#ifndef FILE_H
#define FILE_H 1

extern const char *imm_fname;
extern char filename[];
extern char outname[];
extern char savename[];

extern const char *code_error;
extern struct termios cooked, raw;

extern int check_file(size_t len);
extern void init_files();
extern int load_file();
extern void print_mem_file();
extern void save_file(char *buffer, size_t size);

#define FILE_OK 0

#endif