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

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include <LC_args.h>
#include <LC_vars.h>

#include "file.h"
#include "init.h"
#include "print.h"
#include "run.h"
#include "size.h"

static void about();
static void help();

static void about() {
	putchar('\n');
	print_about();
	putchar('\n');
	exit(0);
}

static void help() {
	putchar('\n');
	print_help();
	putchar('\n');
	exit(0);
}

void init(int argc, char **argv) {
	progname = argv[0];
	signal(SIGINT, on_interrupt);

	LCa_t *arg = LCa_new();
	if(!arg) print_error(UNKNOWN_ERROR);
	arg -> long_flag = "about";
	arg -> short_flag = 'a';
	arg -> pre = about;

	arg = LCa_new();
	if(!arg) print_error(UNKNOWN_ERROR);
	arg -> long_flag = "help";
	arg -> short_flag = 'h';
	arg -> pre = help;

	LCv_t *var = LCv_new();
	if(!var) print_error(UNKNOWN_ERROR);
	var -> id = "colour";
	var -> data = &colour;

	arg = LCa_new();
	if(!arg) print_error(UNKNOWN_ERROR);
	arg -> long_flag = "colour";
	arg -> short_flag = 'c';
	arg -> var = var;
	arg -> value = true;

	arg = LCa_new();
	if(!arg) print_error(UNKNOWN_ERROR);
	arg -> long_flag = "monochrome";
	arg -> short_flag = 'm';
	arg -> var = var;
	arg -> value = false;

	var = LCv_new();
	if(!var) print_error(UNKNOWN_ERROR);
	var -> id = "filename";
	var -> fmt = FILENAME_SCN;
	var -> data = filename;

	arg = LCa_new();
	if(!arg) print_error(UNKNOWN_ERROR);
	arg -> long_flag = "file";
	arg -> short_flag = 'f';
	arg -> var = var;

	int ret = LCa_read(argc, argv);
	if(ret == LCA_BAD_CMD) print_error(BAD_ARGS);
	else if(ret != LCA_OK) print_error(UNKNOWN_ERROR);

	init_files();
}