/*
 * distmem_gaspi.h
 *
 *  Created on: 25 Nov 2015
 *      Author: nick
 */

#ifndef DISTMEM_GASPI_H_
#define DISTMEM_GASPI_H_

#include "distmem.h"
#include <memkind.h>
#include "GASPI.h"

struct distmem_gaspi_memory_information {
	int procs_distributed_over;
	size_t element_size, total_number_elements, number_local_elements;
	void * local_buffer;
};

extern memkind_t GASPI_CONTIGUOUS_KIND;

void distmem_gaspi_init();
void distmem_gaspi_create_segment_kind(const char *, memkind_t *, gaspi_segment_id_t, gaspi_size_t);
void* distmem_gaspi_malloc(memkind_t, size_t, size_t, gaspi_group_t);
void* distmem_gaspi_arena_malloc(struct distmem*, size_t, size_t, int, ...);
struct distmem_gaspi_memory_information* distmem_gaspi_get_info(memkind_t, void*);

#endif /* DISTMEM_GASPI_H_ */
