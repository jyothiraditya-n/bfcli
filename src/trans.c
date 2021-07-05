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

#include "file.h"
#include "print.h"
#include "run.h"
#include "size.h"
#include "trans.h"

bool assembly;
bool transpile;
bool safe_code;

static void conv_safe(size_t i, FILE *file);
static void conv_unsafe(size_t i, FILE *file);
static void conv_amd64(size_t i, FILE *file);

static void _plus_amd64(FILE *file, size_t plus);
static void _minus_amd64(FILE *file, size_t minus);
static void _next_amd64(FILE *file, size_t next);
static void _before_amd64(FILE *file, size_t before);

void convert_file() {
	FILE *file = strlen(outname) ? fopen(outname, "w") : stdout;
	if(!file) { print_error(BAD_OUTPUT); exit(BAD_OUTPUT); }

	if(safe_code && !assembly)
		fputs("#include <stddef.h>\n", file);

	if(!assembly) fputs("#include <stdio.h>\n", file);
	fputs("#include <termios.h>\n", file);
	fputs("#include <unistd.h>\n", file);

	fprintf(file, "char cells[%d];\n", MEM_SIZE);

	if(!assembly) {
		if(safe_code) fputs("size_t ptr = 0;\n", file);
		else fputs("char *ptr = &cells[0];\n", file);
	}

	fputs("struct termios cooked, raw;\n", file);
	if(assembly) fputs("void assembly(char *);\n", file);

	fputs("int main() {\n", file);
	fputs("tcgetattr(STDIN_FILENO, &cooked);\n", file);
	fputs("raw = cooked;\n", file);
	fputs("raw.c_lflag &= ~ICANON;\n", file);
	fputs("raw.c_lflag |= ECHO;\n", file);
	fputs("raw.c_cc[VINTR] = 3;\n", file);
	fputs("raw.c_lflag |= ISIG;\n", file);
	fputs("tcsetattr(STDIN_FILENO, TCSANOW, &raw);\n", file);

	size_t len = strlen(code);

	if(assembly) fputs("assembly(&cells[0]);\n", file);

	else if(!safe_code) {
		for(size_t i = 0; i < len; i++) conv_unsafe(i, file);
		fputc('\n', file);
	}

	else for(size_t i = 0; i < len; i++) conv_safe(i, file);

	fputs("tcsetattr(STDIN_FILENO, TCSANOW, &cooked);\n", file);
	fputs("return 0;\n", file);
	fputs("}\n", file);

	if(assembly) {
		fputs("asm (\n", file);
		fputs("\"assembly:\\n\"\n", file);
		fputs("\"movq $0x0, %rax\\n\"\n", file);
		fputs("\"movb $0x0, %bl\\n\"\n", file);

		for(size_t i = 0; i < len; i++) conv_amd64(i, file);
		fputs("\"ret\\n\"\n", file);
		fputs(");\n", file);
	}

	if(strlen(outname)) {
		int ret = fclose(file);
		if(ret == EOF) print_error(UNKNOWN_ERROR);
	}

	exit(0);
}

static void conv_safe(size_t i, FILE *file) {
	switch(code[i]) {
	case '<':	fprintf(file, "if(ptr) { ptr--; } else { ptr = %d; }\n",
			MEM_SIZE - 1); break;

	case '>':	fprintf(file, "ptr++; if(ptr >= %d) { ptr = 0; }\n",
			MEM_SIZE); break;

	case '+':	fputs("cells[ptr]++;\n", file); break;
	case '-':	fputs("cells[ptr]--;\n", file); break;

	case '[':	fputs("while(cells[ptr]) {\n", file); break;
	case ']':	fputs("}\n", file); break;

	case '.':	fputs("putchar(cells[ptr]);\n", file); break;
	case ',':	fputs("cells[ptr] = getchar();\n", file); break;
	}
}

static void conv_unsafe(size_t i, FILE *file) {
	static size_t col = 0;
	char *instr;
	size_t length;

	switch(code[i]) {
	case '<':	instr = "ptr--; "; length = 7; break;
	case '>':	instr = "ptr++; "; length = 7; break;

	case '+':	instr = "(*ptr)++; "; length = 10; break;
	case '-':	instr = "(*ptr)--; "; length = 10; break;

	case '[':	instr = "while(*ptr) { "; length = 14; break;
	case ']':	instr = "} "; length = 1; break;

	case '.':	instr = "putchar(*ptr); "; length = 15; break;
	case ',':	instr = "*ptr = getchar(); "; length = 18; break;

	default: 	instr = ""; length = 0;
	}

	if(col + length >= 79) { fputc('\n', file); col = 0; }
	fputs(instr, file); col += length;
}

static void conv_amd64(size_t i, FILE *file) {
	static size_t *stack, bracket, indent;
	static size_t plus, minus;
	static size_t next, before;

	if(!stack) {
		stack = malloc(strlen(code) * sizeof(size_t));
		if(!stack) print_error(UNKNOWN_ERROR);
	}

	switch(code[i]) {
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

		fprintf(file, "\"open_%zu:\\n\"\n", bracket);
		fputs("\"testb %bl, %bl\\n\"\n", file);
		fprintf(file, "\"jz close_%zu\\n\"\n", bracket);

		stack[indent++] = bracket++;
		break;

	case ']':
		if(plus) { _plus_amd64(file, plus); plus = 0; }
		if(minus) { _minus_amd64(file, minus); minus = 0; }
		if(next) { _next_amd64(file, next); next = 0; }
		if(before) { _before_amd64(file, before); before = 0; }

		fprintf(file, "\"jmp open_%zu\\n\"\n", stack[--indent]);
		fprintf(file, "\"close_%zu:\\n\"\n", stack[indent]);
		break;

	case '.':
		if(plus) { _plus_amd64(file, plus); plus = 0; }
		if(minus) { _minus_amd64(file, minus); minus = 0; }
		if(next) { _next_amd64(file, next); next = 0; }
		if(before) { _before_amd64(file, before); before = 0; }

		fputs("\"pushq %rax\\n\"\n", file);
		fputs("\"pushq %rbx\\n\"\n", file);
		fputs("\"pushq %rdi\\n\"\n", file);

		fputs("\"movb %bl, (%rax, %rdi)\\n\"\n", file);
		fputs("\"leaq (%rax, %rdi), %rsi\\n\"\n", file);
		fputs("\"movq $0x1, %rax\\n\"\n", file);
		fputs("\"movq $0x1, %rdi\\n\"\n", file);
		fputs("\"movq $0x1, %rdx\\n\"\n", file);
		fputs("\"syscall\\n\"\n", file);

		fputs("\"popq %rdi\\n\"\n", file);
		fputs("\"popq %rbx\\n\"\n", file);
		fputs("\"popq %rax\\n\"\n", file);
		break;

	case ',':
		if(plus) { _plus_amd64(file, plus); plus = 0; }
		if(minus) { _minus_amd64(file, minus); minus = 0; }
		if(next) { _next_amd64(file, next); next = 0; }
		if(before) { _before_amd64(file, before); before = 0; }

		fputs("\"pushq %rax\\n\"\n", file);
		fputs("\"pushq %rdi\\n\"\n", file);

		fputs("\"leaq (%rax, %rdi), %rsi\\n\"\n", file);
		fputs("\"movq $0x0, %rax\\n\"\n", file);
		fputs("\"movq $0x0, %rdi\\n\"\n", file);
		fputs("\"movq $0x1, %rdx\\n\"\n", file);
		fputs("\"syscall\\n\"\n", file);

		fputs("\"popq %rdi\\n\"\n", file);
		fputs("\"popq %rax\\n\"\n", file);
		fputs("\"movb (%rax, %rdi), %bl\\n\"\n", file);
		break;
	}
}

static void _plus_amd64(FILE *file, size_t plus) {
	fprintf(file, "\"addb $0x%zx, %%bl\\n\"\n", plus);
}

static void _minus_amd64(FILE *file, size_t minus) {
	fprintf(file, "\"subb $0x%zx, %%bl\\n\"\n", minus);
}

static void _next_amd64(FILE *file, size_t next) {
	static size_t label;

	fputs("\"movb %bl, (%rax, %rdi)\\n\"\n", file);
	fprintf(file, "\"addq $0x%zx, %%rax\\n\"\n", next);

	if(safe_code) {
		fprintf(file, "\"cmpq $0x%x, %%rax\\n\"\n", MEM_SIZE);
		fprintf(file, "\"jl safe_next_%zu\\n\"\n", label);
		fprintf(file, "\"subq $0x%x, %%rax\\n\"\n", MEM_SIZE);
		fprintf(file, "\"safe_next_%zu:\\n\"\n", label++);
	}

	fputs("\"movb (%rax, %rdi), %bl\\n\"\n", file);
}

static void _before_amd64(FILE *file, size_t before) {
	static size_t label;

	fputs("\"movb %bl, (%rax, %rdi)\\n\"\n", file);
	fprintf(file, "\"subq $0x%zx, %%rax\\n\"\n", before);

	if(safe_code) {
		fprintf(file, "\"cmpq $0x%x, %%rax\\n\"\n", MEM_SIZE);
		fprintf(file, "\"jl safe_before_%zu\\n\"\n", label);
		fprintf(file, "\"subq $0x%x, %%rax\\n\"\n", MEM_SIZE);
		fprintf(file, "\"safe_before_%zu:\\n\"\n", label++);
	}

	fputs("\"movb (%rax, %rdi), %bl\\n\"\n", file);
}
