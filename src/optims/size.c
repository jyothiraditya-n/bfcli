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
#include <stdlib.h>

#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#include <sys/types.h>

#include "level_3.h"
#include "precomp.h"
#include "size.h"

#include "../interpreter.h"
#include "../errors.h"
#include "../optims.h"

typedef struct {
	BFi_instr_t **matches;
	size_t length;
	size_t count;
	ssize_t savings;
	ssize_t brackets;

} ret_t;

static size_t total_nodes;
static size_t thread_count;
static sem_t mutex;

static BFi_instr_t *base, **matches;
static size_t length, count;
static ssize_t savings;

static int are_equal(BFi_instr_t *a, BFi_instr_t *b);
static void insert(BFi_instr_t **node);

typedef struct {
	BFi_instr_t *base;
	size_t index;

} data_t;

static void *call_eval(void *data_p);
static void evaluate(BFi_instr_t *our_base, size_t index);
static ret_t rec_eval(BFi_instr_t *our_base, ret_t data);

BFi_instr_t *BFo_optimise_size(bool precomp) {
	BFi_instr_t *start = precomp? BFo_optimise_precomp()
		: BFo_optimise_lv3_2();

	BFi_instr_t *instr;
	for(instr = start; instr; instr = instr -> next) {
		switch(instr -> opcode) {
		case BFI_INSTR_NOP: case BFI_INSTR_CMPL:
			instr -> op1 = instr -> op2 = 0;
			instr -> ad1 = instr -> ad2 = 0;
			break;

		case BFI_INSTR_INC: case BFI_INSTR_DEC:
			instr -> op2 = instr -> ad2 = 0;
			break;

		
		case BFI_INSTR_MULA: case BFI_INSTR_MULS: case BFI_INSTR_MOV:
			instr -> op2 = 0;
			break;

		case BFI_INSTR_SHLA: case BFI_INSTR_SHLS:
			instr -> op1 = 0;
			break;

		case BFI_INSTR_FWD: case BFI_INSTR_BCK:
		case BFI_INSTR_LOOP: case BFI_INSTR_ENDL:
			instr -> op2 = instr -> ad1 = instr -> ad2 = 0;
			break;

		case BFI_INSTR_INP: case BFI_INSTR_OUT:
			instr -> op1 = instr -> op2 = instr -> ad2 = 0;
			break;

		case BFI_INSTR_CPYA: case BFI_INSTR_CPYS:
			instr -> op1 = instr -> op2 = 0;
			break;
		
		}
	}

	thread_count = sysconf(_SC_NPROCESSORS_ONLN);
	pthread_t threads[thread_count];
	data_t thread_data[thread_count];

	int ret = sem_init(&mutex, 0, 1);
	if(ret == -1) BFe_report_err(BFE_UNKNOWN_ERROR);

loop:	total_nodes = 0;
	for(instr = start; instr; instr = instr -> next)
		total_nodes++;

	if(thread_count > total_nodes) thread_count = total_nodes;

	instr = start;
	for(size_t i = 0; i < thread_count; i++) {
		thread_data[i].base = instr;
		thread_data[i].index = i;

		ret = pthread_create(&threads[i], NULL,
			call_eval, (void *) &thread_data[i]);

		if(ret == -1) BFe_report_err(BFE_UNKNOWN_ERROR);
		else instr = instr -> next;
	}

	for(size_t i = 0; i < thread_count; i++)
		pthread_join(threads[i], NULL);

	if(!base || !count || savings <= 0) {
		if(matches) free(matches);
	endl:	sem_destroy(&mutex);
		return start;
	}

	for(instr = start; instr -> next; instr = instr -> next);
	BFi_instr_t *end = instr;

	if(BFo_sub_count == 1) {
		insert(&end);
		end -> opcode = BFI_INSTR_RET;
	}

	insert(&end);
	end -> opcode = BFI_INSTR_SUB;
	end -> op1 = BFo_sub_count;

	instr = base;
	for(size_t i = 1; i <= length; i++) {
		insert(&end);
		end -> opcode = instr -> opcode;
		end -> op1 = instr -> op1;
		end -> op2 = instr -> op2;
		end -> ad1 = instr -> ad1;
		end -> ad2 = instr -> ad2;

		instr = instr -> next;
	}

	insert(&end);
	end -> opcode = BFI_INSTR_RTS;

	for(size_t i = 0; i < count; i++) {
		BFi_instr_t *first = matches[i];
		BFi_instr_t *last = matches[i];

		for(size_t j = 1; j < length; j++)
			last = last -> next;

		BFi_instr_t *new = malloc(sizeof(BFi_instr_t));
		if(!new) BFe_report_err(BFE_UNKNOWN_ERROR);

		new -> prev = first -> prev;
		new -> next = last -> next;

		new -> opcode = BFI_INSTR_JSR;
		new -> op1 = BFo_sub_count;
		new -> op2 = new -> ad1 = new -> ad2 = 0;

		if(first -> prev) first -> prev -> next = new;
		if(last -> next) last -> next -> prev = new;

		if(!first -> prev) start = new;
		last -> next = NULL;

		while(first) {
			BFi_instr_t *rip = first;
			first = first -> next;
			free(rip);
		}
	}

	BFo_sub_count++;
	length = count = savings = 0;

	if(matches) free(matches);
	matches = NULL;
	base = NULL;

	if(BFo_sub_count > BFo_max_subs) goto endl;
	else goto loop;
}

static int are_equal(BFi_instr_t *a, BFi_instr_t *b) {
	if(!a || !b) return 0;
	if(a -> opcode != b -> opcode) return 0;
	
	switch(a -> opcode) {
	case BFI_INSTR_SUB:
	case BFI_INSTR_RTS:
		return 0;

	case BFI_INSTR_LOOP:
		return -1;

	case BFI_INSTR_ENDL:
		return -3;
	}

	return (a -> op1 == b -> op1) && (a -> ad1 == b -> ad1)
	    && (a -> op2 == b -> op2) && (a -> ad2 == b -> ad2);
}

static void insert(BFi_instr_t **node) {
	(*node) -> next = malloc(sizeof(BFi_instr_t));
	if(!(*node) -> next) BFe_report_err(BFE_UNKNOWN_ERROR);

	(*node) -> next -> prev = (*node);
	(*node) = (*node) -> next;
	(*node) -> next = NULL;

	(*node) -> opcode = BFI_INSTR_NOP;
	(*node) -> op1 = (*node) -> op2 = 0;
	(*node) -> ad1 = (*node) -> ad2 = 0;
}

static void *call_eval(void *data_p) {
	data_t *data = (data_t *) data_p;

	while(data -> base) {
		evaluate(data -> base, data -> index);

		for(size_t i = 0; i < thread_count && data -> base; i++)
			data -> base = data -> base -> next;

		data -> index += thread_count;
	}

	return NULL;
}

static void evaluate(BFi_instr_t *our_base, size_t index) {
	BFi_instr_t **our_matches;
	size_t our_length = 1, our_count = 0;
	ssize_t our_savings = -1;

	ssize_t our_brackets = are_equal(our_base, our_base);
	our_brackets = our_brackets < 0 ? our_brackets + 2 : 0;

	our_matches = malloc(sizeof(BFi_instr_t *) * (total_nodes - index));
	if(!our_matches) BFe_report_err(BFE_UNKNOWN_ERROR);

	for(BFi_instr_t *i = our_base; i; i = i -> next)
		if(are_equal(our_base, i)) our_matches[our_count++] = i;

	ret_t data = { our_matches, our_length + 1, our_count,
		our_savings, our_brackets };

	data = rec_eval(our_base -> next, data);

	if(data.savings > our_savings && !data.brackets && our_brackets >= 0) {
		free(our_matches);

		our_matches = data.matches;
		our_length = data.length;
		our_count = data.count;
		our_savings = data.savings;
		our_brackets = data.brackets;
	}

	else free(data.matches);

	sem_wait(&mutex);
	if(our_savings > savings && !our_brackets) {
		if(matches) free(matches);

		base = our_base;
		matches = our_matches;
		length = our_length;
		count = our_count;
		savings = our_savings;
	}

	else free(our_matches);
	sem_post(&mutex);
}

static ret_t rec_eval(BFi_instr_t *our_base, ret_t data) {
	BFi_instr_t **our_matches;
	size_t our_length = data.length, our_count = 0;
	ssize_t our_savings = 0, our_brackets = data.brackets;

	our_matches = malloc(sizeof(BFi_instr_t) * data.count);
	if(!our_matches) BFe_report_err(BFE_UNKNOWN_ERROR);

	for(size_t i = 0; i < data.count; i++) {
		BFi_instr_t *instr = data.matches[i];
		for(size_t j = 1; j < data.length && instr; j++)
			instr = instr -> next;

		if(i != data.count - 1)
			if(instr == data.matches[i + 1]) continue;

		if(are_equal(our_base, instr))
			our_matches[our_count++] = data.matches[i];
	}

	int equality = are_equal(our_base, our_base);
	our_brackets += equality < 0 ? equality + 2 : 0;
	our_savings = (our_length - 1) * (our_count - 1) - 1;

	data.matches = our_matches;
	data.length = our_length;
	data.count = our_count;
	data.savings = our_savings;
	data.brackets = our_brackets;

	if(our_count <= 1) return data;
	else data.length++;
	
	ret_t ret = rec_eval(our_base -> next, data);

	if(ret.savings > our_savings && !ret.brackets && our_brackets >= 0) {
		free(our_matches);
		return ret;
	}

	free(ret.matches);
	data.length--;
	return data;
}