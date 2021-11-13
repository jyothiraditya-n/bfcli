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
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "clidata.h"
#include "errors.h"
#include "files.h"
#include "interpreter.h"
#include "main.h"
#include "printing.h"
#include "translator.h"

bool BFt_compile;
bool BFt_translate;
bool BFt_use_safe_code;

static size_t label;

static void conv_safe(size_t i, FILE *file);
static void conv_unsafe(size_t i, FILE *file);
static void conv_amd64(size_t i, FILE *file);

static void _plus_amd64(FILE *file, size_t plus);
static void _minus_amd64(FILE *file, size_t minus);
static void _next_amd64(FILE *file, size_t next);
static void _before_amd64(FILE *file, size_t before);

void BFt_convert_file() {
	FILE *file = strlen(BFf_outfile_name)
		? fopen(BFf_outfile_name, "w")
		: stdout;

	if(!file) {
		BFe_file_name = BFf_outfile_name;
		BFe_report_err(BFE_FILE_UNWRITABLE);
		exit(BFE_FILE_UNWRITABLE);
	}

	if(BFt_use_safe_code && !BFt_compile)
		fputs("#include <stddef.h>\n", file);

	if(!BFt_compile) fputs("#include <stdio.h>\n\n", file);

	if(BFc_direct_inp) {
		fputs("#include <termios.h>\n", file);
		fputs("#include <unistd.h>\n\n", file);

		fputs("struct termios cooked, raw;\n", file);
	}

	fprintf(file, "char cells[%zu];\n\n", BFi_mem_size);
	
	if(BFt_compile) fputs("void compile(char *);\n\n", file);
	
	else {
		fputs("void brainfuck();\n\n", file);

		if(BFt_use_safe_code) fputs("size_t ptr = 0;\n", file);
		else fputs("char *ptr = &cells[0];\n\n", file);
	}

	fputs("int main() {\n", file);

	if(BFc_direct_inp) {
		fputs("\ttcgetattr(STDIN_FILENO, &cooked);\n", file);
		fputs("\traw = cooked;\n", file);
		fputs("\traw.c_lflag &= ~ICANON;\n", file);
		fputs("\traw.c_lflag |= ECHO;\n", file);
		fputs("\traw.c_cc[VINTR] = 3;\n", file);
		fputs("\traw.c_lflag |= ISIG;\n", file);
		fputs("\ttcsetattr(STDIN_FILENO, TCSANOW, &raw);\n\n", file);
	}

	size_t len = strlen(BFi_program_str);

	if(BFt_compile) fputs("\tcompile(&cells[0]);\n", file);
	else fputs("\tbrainfuck();\n", file);

	if(BFc_direct_inp)
		fputs("\ttcsetattr(STDIN_FILENO, TCSANOW, &cooked);\n", file);

	fputs("\treturn 0;\n", file);
	fputs("}\n\n", file);

	if(BFt_compile) {
		fputs("asm (\n", file);
		fputs("\"compile:\\n\"\n", file);
		fputs("\"\tmovq $0x0, %rax\\n\"\n", file);
		fputs("\"\tmovb $0x0, %bl\\n\"\n", file);

		for(size_t i = 0; i < len; i++) conv_amd64(i, file);
		fputs("\"\tret\\n\"\n", file);
		fputs(");\n", file);
	}

	else {
		fputs("void brainfuck() {\n", file);

		if(!BFt_use_safe_code) {
			fputc('\t', file);
			for(size_t i = 0; i < len; i++)
				conv_unsafe(i, file);

			fputc('\n', file);
		}

		else for(size_t i = 0; i < len; i++) 
			conv_safe(i, file);

		fputs("}\n", file);
	}

	if(strlen(BFf_outfile_name)) {
		int ret = fclose(file);
		if(ret == EOF) BFe_report_err(BFE_UNKNOWN_ERROR);
	}

	exit(0);
}

static void conv_safe(size_t i, FILE *file) {
	switch(BFi_program_str[i]) {
	case '<': fprintf(file, "\tif(ptr) { ptr--; } else { ptr = %zu; }\n",
		  BFi_mem_size - 1); break;

	case '>': fprintf(file, "\tptr++; if(ptr >= %zu) { ptr = 0; }\n",
		  BFi_mem_size); break;

	case '+': fputs("\tcells[ptr]++;\n", file); break;
	case '-': fputs("\tcells[ptr]--;\n", file); break;

	case '[': fputs("\twhile(cells[ptr]) {\n", file); break;
	case ']': fputs("\t}\n", file); break;

	case '.': fputs("\tputchar(cells[ptr]);\n", file); break;
	case ',': fputs("\tcells[ptr] = getchar();\n", file); break;
	}
}

static void conv_unsafe(size_t i, FILE *file) {
	static size_t col = 0;
	char *instr;
	size_t length;

	switch(BFi_program_str[i]) {
	case '<':	instr = "--ptr; "; length = 7; break;
	case '>':	instr = "++ptr; "; length = 7; break;

	case '+':	instr = "++*ptr; "; length = 8; break;
	case '-':	instr = "--*ptr; "; length = 8; break;

	case '[':	instr = "while(*ptr) { "; length = 14; break;
	case ']':	instr = "} "; length = 1; break;

	case '.':	instr = "putchar(*ptr); "; length = 15; break;
	case ',':	instr = "*ptr = getchar(); "; length = 18; break;

	default: 	instr = ""; length = 0;
	}

	if(col + length >= 71) { fputs("\n\t", file); col = 0; }
	fputs(instr, file); col += length;
}

static void conv_amd64(size_t i, FILE *file) {
	static size_t *stack, bracket, indent;
	static size_t plus, minus;
	static size_t next, before;

	if(!stack) {
		stack = malloc(strlen(BFi_program_str) * sizeof(size_t));
		if(!stack) BFe_report_err(BFE_UNKNOWN_ERROR);
	}

	switch(BFi_program_str[i]) {
	case '+':
		if(minus) { _minus_amd64(file, minus); minus = 0; }
		if(next) { _next_amd64(file, next); next = 0; }
		if(before) { _before_amd64(file, before); before = 0; }

		plus++;
		break;

	case '-':
		if(plus) { _plus_amd64(file, plus); plus = 0; }
		if(next) { _next_amd64(file, next); next = 0; }
		if(before) { _before_amd64(file, before); before = 0; }

		minus++;
		break;

	case '>':
		if(plus) { _plus_amd64(file, plus); plus = 0; }
		if(minus) { _minus_amd64(file, minus); minus = 0; }
		if(before) { _before_amd64(file, before); before = 0; }

		next++;
		break;

	case '<':
		if(plus) { _plus_amd64(file, plus); plus = 0; }
		if(minus) { _minus_amd64(file, minus); minus = 0; }
		if(next) { _next_amd64(file, next); next = 0; }

		before++;
		break;

	case '[':
		if(plus) { _plus_amd64(file, plus); plus = 0; }
		if(minus) { _minus_amd64(file, minus); minus = 0; }
		if(next) { _next_amd64(file, next); next = 0; }
		if(before) { _before_amd64(file, before); before = 0; }

		fprintf(file, "\"\\n\"\n\"open_%zu:\\n\"\n", bracket);
		fputs("\"\ttestb %bl, %bl\\n\"\n", file);
		fprintf(file, "\"\tjz close_%zu\\n\"\n", bracket);

		stack[indent++] = bracket++;
		break;

	case ']':
		if(plus) { _plus_amd64(file, plus); plus = 0; }
		if(minus) { _minus_amd64(file, minus); minus = 0; }
		if(next) { _next_amd64(file, next); next = 0; }
		if(before) { _before_amd64(file, before); before = 0; }

		fprintf(file, "\"\tjmp open_%zu\\n\"\n", stack[--indent]);
		fprintf(file, "\"\\n\"\n\"close_%zu:\\n\"\n", stack[indent]);
		break;

	case '.':
		if(plus) { _plus_amd64(file, plus); plus = 0; }
		if(minus) { _minus_amd64(file, minus); minus = 0; }
		if(next) { _next_amd64(file, next); next = 0; }
		if(before) { _before_amd64(file, before); before = 0; }

		fputs("\"\tpushq %rax\\n\"\n", file);
		fputs("\"\tpushq %rbx\\n\"\n", file);
		fputs("\"\tpushq %rdi\\n\"\n", file);

		fputs("\"\tmovb %bl, (%rax, %rdi)\\n\"\n", file);
		fputs("\"\tleaq (%rax, %rdi), %rsi\\n\"\n", file);
		fputs("\"\tmovq $0x1, %rax\\n\"\n", file);
		fputs("\"\tmovq $0x1, %rdi\\n\"\n", file);
		fputs("\"\tmovq $0x1, %rdx\\n\"\n", file);
		fputs("\"\tsyscall\\n\"\n", file);

		fputs("\"\tpopq %rdi\\n\"\n", file);
		fputs("\"\tpopq %rbx\\n\"\n", file);
		fputs("\"\tpopq %rax\\n\"\n", file);
		break;

	case ',':
		if(plus) { _plus_amd64(file, plus); plus = 0; }
		if(minus) { _minus_amd64(file, minus); minus = 0; }
		if(next) { _next_amd64(file, next); next = 0; }
		if(before) { _before_amd64(file, before); before = 0; }

		fputs("\"\tpushq %rax\\n\"\n", file);
		fputs("\"\tpushq %rdi\\n\"\n", file);

		fputs("\"\tleaq (%rax, %rdi), %rsi\\n\"\n", file);
		fputs("\"\tmovq $0x0, %rax\\n\"\n", file);
		fputs("\"\tmovq $0x0, %rdi\\n\"\n", file);
		fputs("\"\tmovq $0x1, %rdx\\n\"\n", file);
		fputs("\"\tsyscall\\n\"\n", file);

		fputs("\"\tpopq %rdi\\n\"\n", file);
		fputs("\"\tpopq %rax\\n\"\n", file);
		fputs("\"\tmovb (%rax, %rdi), %bl\\n\"\n", file);
		break;
	}
}

static void _plus_amd64(FILE *file, size_t plus) {
	fprintf(file, "\"\taddb $0x%zx, %%bl\\n\"\n", plus);
}

static void _minus_amd64(FILE *file, size_t minus) {
	fprintf(file, "\"\tsubb $0x%zx, %%bl\\n\"\n", minus);
}

static void _next_amd64(FILE *file, size_t next) {
	fputs("\"\tmovb %bl, (%rax, %rdi)\\n\"\n", file);
	fprintf(file, "\"\taddq $0x%zx, %%rax\\n\"\n", next);

	if(BFt_use_safe_code) {
		fprintf(file, "\"\tcmpq $0x%zx, %%rax\\n\"\n", BFi_mem_size);
		fprintf(file, "\"\tjl safe_%zu\\n\"\n", label);
		fprintf(file, "\"\tsubq $0x%zx, %%rax\\n\"\n", BFi_mem_size);
		fprintf(file, "\"\\n\"\n\"safe_%zu:\\n\"\n", label++);
	}

	fputs("\"\tmovb (%rax, %rdi), %bl\\n\"\n", file);
}

static void _before_amd64(FILE *file, size_t before) {
	fputs("\"\tmovb %bl, (%rax, %rdi)\\n\"\n", file);
	fprintf(file, "\"\tsubq $0x%zx, %%rax\\n\"\n", before);

	if(BFt_use_safe_code) {
		fprintf(file, "\"\tcmpq $0x%zx, %%rax\\n\"\n", BFi_mem_size);
		fprintf(file, "\"\tjl safe_%zu\\n\"\n", label);
		fprintf(file, "\"\tsubq $0x%zx, %%rax\\n\"\n", BFi_mem_size);
		fprintf(file, "\"\\n\"\n\"safe_%zu:\\n\"\n", label++);
	}

	fputs("\"\tmovb (%rax, %rdi), %bl\\n\"\n", file);
}
