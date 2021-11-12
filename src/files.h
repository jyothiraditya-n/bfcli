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

#ifndef BF_FILES_H
#define BF_FILES_H 1

extern char BFf_mainfile_name[];
extern char BFf_outfile_name[];
extern char BFf_savefile_name[];

extern void BFf_init();
extern int BFf_load_file();
extern void BFf_dump_mem();
extern void BFf_save_file(char *buffer, size_t size);

#define FILE_OK 0

#endif