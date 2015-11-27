/*
 * distmem.h
 *
 *  Created on: 3 Nov 2015
 *      Author: nick
 */

#ifndef DISTMEM_H_
#define DISTMEM_H_

#include <memkind.h>
#include <stddef.h>
#include <stdarg.h>
#include "mpi.h"

static int HASH_SIZE=4993;

struct dist_malloc_specific {
	unsigned short id;
	void * specific_information;
};

struct distmem {
	char * name;
	struct memkind * memkind;
	struct distmem_memory_information_generic ** internal_state;
	struct distmem_ops * operations;
};

struct distmem_ops {
	struct memkind_ops * memkind_operations;
	void *(* dist_malloc)(struct distmem *, size_t, size_t, int, ...);
	int (*dist_create)(struct distmem *dist_kind, struct distmem_ops *ops, const char *name);
};

struct distmem_memory_information_generic {
	void * specific_information, * ptr;
	struct distmem_memory_information_generic * next;
};

typedef struct distmem * distmem_t;

void init_distmem();
int distmem_create(struct distmem_ops*, const char*, memkind_t*);
int distmem_arena_create(struct distmem *, struct distmem_ops *, const char *name);
void* distmem_malloc(memkind_t, size_t, size_t, MPI_Comm);
void distmem_put_specific_entry_into_state(struct distmem*, void*, void*);
void* distmem_get_specific_entry(struct distmem*, void*);
struct distmem * distmem_get_distkind_by_name(char*);

#endif /* DISTMEM_H_ */
