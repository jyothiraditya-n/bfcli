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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arch.h"
#include "clidata.h"
#include "files.h"
#include "errors.h"
#include "main.h"
#include "optims.h"
#include "translator.h"

#include "arch/amd64.h"
#include "arch/i386.h"
#include "arch/8086.h"
#include "arch/z80.h"
#include "arch/bfir.h"

char BFa_target_arch[BF_FILENAME_SIZE];

extern void BFa_translate() {
	if(BFc_direct_inp) {
		BFe_code_error = "--standalone is incompatible with --direct-inp.";
		BFe_report_err(BFE_INCOMPATIBLE_ARGS);
		exit(BFE_INCOMPATIBLE_ARGS);
	}

	if(!strlen(BFf_outfile_name)) {
		const char *extension = ".s";
		if(!strcmp(BFa_target_arch, "8086")) extension = ".asm";
		else if(!strcmp(BFa_target_arch, "z80")) extension = ".asm";
		else if(!strcmp(BFa_target_arch, "bfir")) extension = ".bfir";

		strcpy(BFf_outfile_name, BFf_mainfile_name);
		size_t len = strlen(BFf_outfile_name);

		for(size_t i = len; i <= len; i--)
			if(BFf_outfile_name[i] == '.') BFf_outfile_name[i] = 0;

		strncat(BFf_outfile_name, extension,
			BF_FILENAME_SIZE - 1 - strlen(BFf_outfile_name));
	}

	FILE *file = fopen(BFf_outfile_name, "w");

	if(!file) {
		BFe_file_name = BFf_outfile_name;
		BFe_report_err(BFE_FILE_UNWRITABLE);
		exit(BFE_FILE_UNWRITABLE);
	}

	BFo_optimise();

	if(!strcmp(BFa_target_arch, "amd64")) BFa_amd64_tasm(file);
	else if(!strcmp(BFa_target_arch, "i386")) BFa_i386_tasm(file);
	else if(!strcmp(BFa_target_arch, "8086")) BFa_8086_t(file);
	else if(!strcmp(BFa_target_arch, "z80")) BFa_z80_t(file);
	else if(!strcmp(BFa_target_arch, "bfir")) BFa_bfir_t(file);

	else { BFe_report_err(BFE_BAD_ARCH); exit(BFE_BAD_ARCH); }

	int ret = fclose(file);
	if(ret == EOF) BFe_report_err(BFE_UNKNOWN_ERROR);
	exit(0);
}

extern void BFa_translate_c(FILE *file) {
	if(!strcmp(BFa_target_arch, "amd64")) BFa_amd64_tc(file);
	else if(!strcmp(BFa_target_arch, "i386")) BFa_i386_tc(file);
	else { BFe_report_err(BFE_BAD_ARCH); exit(BFE_BAD_ARCH); }
}
