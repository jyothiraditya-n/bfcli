#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//---------------------------------------------------------------------------//

#define CODE_SIZE (8 * 1024 * 1024)
#define MEM_SIZE (8 * 1024 * 1024)

char code[CODE_SIZE];
char mem[MEM_SIZE];
size_t ptr;

//---------------------------------------------------------------------------//

const char *progname;

void print_error(int errnum);

#define BAD_ARGS 1
#define CODE_TOO_LONG 2
#define LINE_TOO_LONG 3

int main(int argc, char **argv);
void print_help();

#define LINE_SIZE 4096
#define LINE_SIZE_PRI "4095"

//---------------------------------------------------------------------------//

void run(char *start, size_t len);

//---------------------------------------------------------------------------//

void print_error(int errnum) {
	switch(errnum) {
	case BAD_ARGS:
		fprintf(stderr, "%s: error: too many args.\n\n", progname);
		print_help();

		exit(errnum);

	case CODE_TOO_LONG:
		fprintf(stderr, "%s: error: code too long.\n", progname);
		break;

	case LINE_TOO_LONG:
		fprintf(stderr, "%s: error: line too long.\n", progname);
		break;

	default:
		fprintf(stderr, "%s: error: unknown erro %d\n",
			progname, errnum);

		exit(errnum);
	}
}

int main(int argc, char **argv) {
	progname = argv[0];
	if(argc > 1) print_error(BAD_ARGS);

	static char line[LINE_SIZE];
	size_t insertion_point = 0;

	while(!feof(stdin)) {
		if(!insertion_point) printf("bf) ");
		else printf("bf contd.) ");

		if(CODE_SIZE - insertion_point < LINE_SIZE) {
			insertion_point = 0;

			print_error(CODE_TOO_LONG);
			continue;
		}

		char endl;

		int ret = scanf("%" LINE_SIZE_PRI "[^\n]%c", line, &endl);

		if(ret != 2) print_error(-1);

		if(endl != '\n' && !feof(stdin)) {
			print_error(LINE_TOO_LONG);
			continue;
		}

		strcpy(&code[insertion_point], line);

		size_t len = strlen(code);
		int brackets_open = 0;

		for(size_t i = 0; i < len; i++) {
			switch(code[i]) {
			case '[':
				brackets_open++;
				break;

			case ']':
				brackets_open--;
				break;
			}
		}

		if(!brackets_open) {
			run(code, len);
			insertion_point = 0;
		}

		else insertion_point += strlen(line);
	}

	exit(0);
}

void print_help() {
	printf("Usage: %s\n", progname);
	printf("(Don't supply any arguments.)\n\n");

	printf("Interactive bf console; exit with ^C.\n\n");

	printf("Line buffer size: %d chars\n", LINE_SIZE);
	printf("Code buffer size: %d chars\n", CODE_SIZE);
	printf("Memory size: %d chars\n", MEM_SIZE);
}

//---------------------------------------------------------------------------//

void run(char *start, size_t len) {
	int brackets_open = 0;
	size_t sub_start = 0;
	int ret = 0;

	for(size_t i = 0; i < len; i++) {
		switch(start[i]) {
		case '[':
			if(!sub_start) sub_start = i + 1;
			brackets_open++;
			continue;

		case ']':
			brackets_open--;

			if(!brackets_open) {
				while(mem[ptr])
					run(&start[sub_start], i - sub_start);

				sub_start = 0;
			}
		
			continue;
		}

		if(sub_start) continue;

		switch(start[i]) {
		case '>':
			if(ptr < MEM_SIZE) ptr++;
			else ptr = 0;

			break;

		case '<':
			if(ptr > 0) ptr--;
			else ptr = MEM_SIZE - 1;

			break;

		case '+':
			mem[ptr]++;
			break;

		case '-':
			mem[ptr]--;
			break;

		case '.':
			printf("%c", mem[ptr]);
			break;

		case ',':
			ret = scanf("%c", &mem[ptr]);
			if(ret != 1) print_error(-1);

			break;
		}
	}
}