/*
 * distmem_mpi.c
 *
 *  Created on: 25 Nov 2015
 *      Author: nick
 */

#include "distmem_mpi.h"
#include <memkind.h>
#include <stdio.h>
#include <stdarg.h>

memkind_t MPI_CONTIGUOUS_KIND;

static struct distmem_ops DISTRIBUTED_MEMORY_VTABLE = {
    .dist_malloc=distmem_mpi_arena_malloc,
    .dist_create=distmem_arena_create
};

static void put_info_entry_into_state(struct distmem*, void*, int, size_t, size_t, size_t);

void distmem_mpi_init() {
	init_distmem();
	int err=distmem_create_default(&DISTRIBUTED_MEMORY_VTABLE, "mpi contiguous", &MPI_CONTIGUOUS_KIND);
	if (err) {
		fprintf(stderr, "Error allocating distributed kind\n");
	}
}

void* distmem_mpi_malloc(memkind_t kind, size_t element_size, size_t number_elements, MPI_Comm communicator) {
	struct distmem * dist_kind=distmem_get_distkind_by_name(kind->name);
	return dist_kind->operations->dist_malloc(dist_kind, element_size, number_elements, 1, communicator);
}

void* distmem_mpi_arena_malloc(struct distmem * dist_kind, size_t element_size, size_t number_elements, int nargs, ...) {
	va_list ap;
	va_start(ap, nargs);
	MPI_Comm communicator = va_arg(ap, MPI_Comm);

	size_t number_local_elements, dimension_division, dimension_extra;
	int size, my_rank;
	MPI_Comm_size(communicator, &size);
	MPI_Comm_rank(communicator, &my_rank);
	number_local_elements=number_elements/size;

	dimension_division = number_elements / size;
	dimension_extra = number_elements - (dimension_division * size);

	number_local_elements=dimension_division+(my_rank<dimension_extra?1:0);
	void * buffer = (char*) memkind_malloc(dist_kind->memkind, number_local_elements*element_size);
	put_info_entry_into_state(dist_kind, buffer, size, element_size, number_elements, number_local_elements);
	return buffer;
}

struct distmem_mpi_memory_information * distmem_mpi_get_info(memkind_t kind, void * ptr) {
	struct distmem * dist_kind=distmem_get_distkind_by_name(kind->name);
	return (struct distmem_mpi_memory_information*) distmem_get_specific_entry(dist_kind, ptr);
}

static void put_info_entry_into_state(struct distmem * dist_kind, void * local_buffer, int procs_distributed_over, size_t element_size,
		size_t total_number_elements, size_t number_local_elements) {
	struct distmem_mpi_memory_information* specific_state=(struct distmem_mpi_memory_information*) memkind_malloc(MEMKIND_DEFAULT, sizeof(struct distmem_mpi_memory_information));
	specific_state->local_buffer=local_buffer;
	specific_state->element_size=element_size;
	specific_state->total_number_elements=total_number_elements;
	specific_state->number_local_elements=number_local_elements;
	specific_state->procs_distributed_over=procs_distributed_over;
	distmem_put_specific_entry_into_state(dist_kind, specific_state, local_buffer);
}
