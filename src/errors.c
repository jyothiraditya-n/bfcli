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

#include "arch.h"
#include "errors.h"
#include "clidata.h"
#include "optims.h"
#include "translator.h"

const char *BFe_code_error;
const char *BFe_file_name;

void BFe_report_err(int errnum) {
	switch(errnum) {
	case BFE_BAD_OPTIM:
		fprintf(stderr, "%s: error: unknown optimisation band: '%c'\n",
			BFc_cmd_name, BFo_level);
		break;

	case BFE_INCOMPATIBLE_ARGS:
		fprintf(stderr, "%s: error: %s\n", BFc_cmd_name,
			BFe_code_error);
		break;

	case BFE_BAD_ARCH:
		fprintf(stderr, "%s: error: unknown architecture: %s\n",
			BFc_cmd_name, BFa_target_arch);
		break;

	case BFE_CODE_TOO_LONG:
		fprintf(stderr, "%s: error: code too long.\n", BFc_cmd_name);
		break;

	case BFE_LINE_TOO_LONG:
		fprintf(stderr, "%s: error: line too long.\n", BFc_cmd_name);
		break;

	case BFE_NO_FILE:
		fprintf(stderr, "%s: error: no file specified to "
			"translate or compile.\n", BFc_cmd_name);
		break;

	case BFE_FILE_UNREADABLE:
		fprintf(stderr, "%s: error: cannot read file '%s'.\n",
			BFc_cmd_name, BFe_file_name);
		break;

	case BFE_FILE_UNWRITABLE:
		fprintf(stderr, "%s: error: cannot write file '%s'.\n",
			BFc_cmd_name, BFe_file_name);
		break;

	case BFE_BAD_CODE:
		fprintf(stderr, "%s: error: '%s': %s\n", BFc_cmd_name,
			BFe_file_name, BFe_code_error);
		break;

	case BFE_SEGFAULT:
		fprintf(stderr, "%s: error: out of memory.\n", BFe_file_name);
		break;

	default:
		fprintf(stderr, "%s: error: unknown error\n", BFc_cmd_name);
		perror("cstdlib");
		exit(errnum);
	}
}