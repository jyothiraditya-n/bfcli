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

#ifndef BF_PRINTING_H
#define BF_PRINTING_H 1

extern void BFp_print_about();
extern void BFp_print_banner();
extern void BFp_print_help();
extern void BFp_print_minihelp();
extern void BFp_print_prompt();
extern void BFp_print_usage();

extern void BFp_print_bytecode();
extern void BFp_peek_at_mem();
extern void BFp_dump_mem();

#endif