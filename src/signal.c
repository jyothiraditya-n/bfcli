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

#include <signal.h>
#include <LC_lines.h>

#include "main.h"
#include "run.h"

void on_interrupt(int signum) {
	if(signum != SIGINT) {
		signal(signum, SIG_DFL);
		return;
	}

	if(running) {
		signal(SIGINT, on_interrupt);
		running = false;
		lastch = 0;
	}

	else if(insertion_point) {
		signal(SIGINT, on_interrupt);
		LCl_sigint = true;
	}

	else {
		signal(SIGINT, SIG_DFL);
		raise(SIGINT);
	}
}