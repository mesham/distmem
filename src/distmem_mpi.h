/*
 * distmem_mpi.h
 *
 *  Created on: 25 Nov 2015
 *      Author: nick
 */

#ifndef DISTMEM_MPI_H_
#define DISTMEM_MPI_H_

#include "distmem.h"
#include <memkind.h>
#include <stdarg.h>
#include <mpi.h>

struct distmem_mpi_memory_information {
  int procs_distributed_over;
  size_t element_size, total_number_elements, number_local_elements, *elements_per_process;
  void* local_buffer;
  MPI_Comm communicator;
};

extern memkind_t MPI_CONTIGUOUS_KIND;

void distmem_mpi_init();
void* distmem_mpi_malloc(memkind_t, size_t, size_t, MPI_Comm);
struct distmem_block* distmem_mpi_contiguous_distributer(int*, struct distmem*, size_t, size_t, int, ...);
struct distmem_mpi_memory_information* distmem_mpi_get_info(memkind_t, void*);

#endif /* DISTMEM_MPI_H_ */
