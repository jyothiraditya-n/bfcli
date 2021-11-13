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

#ifndef BF_ERRORS_H
#define BF_ERRORS_H 1

#define BFE_UNKNOWN_ERROR 0
#define BFE_BAD_ARGS 1
#define BFE_CODE_TOO_LONG 2
#define BFE_LINE_TOO_LONG 3
#define BFE_FILE_UNREADABLE 6
#define BFE_FILE_UNWRITABLE 7
#define BFE_BAD_CODE 8

extern char *BFe_code_error;
extern char *BFe_file_name;

extern void BFe_report_err(int errnum);

#endif