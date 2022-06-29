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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>

#include "arch.h"
#include "errors.h"
#include "interpreter.h"
#include "optims.h"
#include "translator.h"

#include "optims/level_1.h"
#include "optims/level_2.h"
#include "optims/level_3.h"
#include "optims/level_4.h"
#include "optims/size.h"

char BFo_level = '0';
ssize_t BFo_mem_padding;
bool BFo_advanced_ops = true;

size_t BFo_sub_count = 1;
size_t BFo_max_subs = SIZE_MAX;

void BFo_optimise() {
	while(BFi_code) {
		BFi_instr_t *instr = BFi_code;
		BFi_code = instr -> next;
		free(instr);
	}

	if(!strcmp(BFa_target_arch, "z80")) BFo_advanced_ops = false;
	BFi_compile(true);

	switch(BFo_level) {
		case '0': break;
		case '1': BFi_code = BFo_optimise_lv1(); break;
		case '2': BFi_code = BFo_optimise_lv2(); break;
		case '3': BFi_code = BFo_optimise_lv3(); break;
		case '4': BFi_code = BFo_optimise_lv4(); break;

		case 'S':
		case 's': BFi_code = BFo_optimise_size(); break;

		default:
			BFe_report_err(BFE_BAD_OPTIM);
			exit(BFE_BAD_OPTIM);
	}
}