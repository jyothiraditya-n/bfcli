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
#include <stdio.h>

#include <termios.h>
#include <unistd.h>

#include "clidata.h"
#include "errors.h"

const char *BFc_cmd_name;
const char *BFc_immediate;

bool BFc_use_colour = false;
size_t BFc_height = 24;
size_t BFc_width = 80;

bool BFc_no_ansi;
bool BFc_direct_inp;
bool BFc_minimal_mode;

struct termios BFc_cooked, BFc_raw;

void BFc_init() {
        int ret = tcgetattr(STDIN_FILENO, &BFc_cooked);
        if(ret == -1) BFe_report_err(BFE_UNKNOWN_ERROR);

        BFc_raw = BFc_cooked;
        BFc_raw.c_lflag &= ~ICANON;
        BFc_raw.c_lflag |= ECHO;
        BFc_raw.c_cc[VINTR] = 3;
        BFc_raw.c_lflag |= ISIG;
}

void BFc_get_dimensions() {
        if(BFc_no_ansi) return;

        BFc_raw.c_lflag &= ~ECHO;
        int ret = tcsetattr(STDIN_FILENO, TCSANOW, &BFc_raw);
        if(ret == -1) BFe_report_err(BFE_UNKNOWN_ERROR);
        
        printf("\e[s\e[999;999H\e[6n\e[u");
        while(getchar() != '\e');

        ret = scanf("[%zu;%zuR", &BFc_height, &BFc_width);
        if(ret != 2) BFe_report_err(BFE_UNKNOWN_ERROR);

        BFc_raw.c_lflag |= ECHO;
        ret = tcsetattr(STDIN_FILENO, TCSANOW, &BFc_cooked);
        if(ret == -1) BFe_report_err(BFE_UNKNOWN_ERROR);

        if(BFc_minimal_mode && BFc_width > 80) BFc_width = 80;
}