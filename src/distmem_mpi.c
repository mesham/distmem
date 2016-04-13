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

static void* distmem_mpi_arena_malloc(struct distmem*, size_t, size_t, struct distmem_block*, int, int, ...);
static void put_info_entry_into_state(struct distmem*, void*, int, size_t, size_t, size_t, size_t*, MPI_Comm);
static void deallocate_specific_information(void*);

void distmem_mpi_init() {
  init_distmem();
  struct distmem_ops DISTRIBUTED_MEMORY_VTABLE = {.dist_malloc = distmem_mpi_arena_malloc,
                                                  .dist_create = distmem_arena_create,
                                                  .dist_determine_distribution = mpi_contiguous_distribution};
  int err = distmem_create_default(&DISTRIBUTED_MEMORY_VTABLE, "mpicontiguous", &MPI_CONTIGUOUS_KIND);
  if (err) {
    fprintf(stderr, "Error allocating distributed kind\n");
  }
}

void* distmem_mpi_malloc(memkind_t kind, size_t element_size, size_t number_elements, MPI_Comm communicator) {
  struct distmem* dist_kind = distmem_get_distkind_by_name(kind->name);
  int num_blocks;
  struct distmem_block* allocation_blocks =
      dist_kind->operations->dist_determine_distribution(&num_blocks, dist_kind, element_size, number_elements, 1, communicator);
  void* memoryPointer =
      dist_kind->operations->dist_malloc(dist_kind, element_size, number_elements, allocation_blocks, num_blocks, 1, communicator);
  memkind_free(MEMKIND_DEFAULT, allocation_blocks);
  return memoryPointer;
}

static void* distmem_mpi_arena_malloc(struct distmem* dist_kind, size_t element_size, size_t number_elements,
                                      struct distmem_block* allocation_blocks, int number_blocks, int nargs, ...) {
  va_list ap;
  va_start(ap, nargs);
  int i, my_rank, size, num_blocks;
  MPI_Comm communicator = va_arg(ap, MPI_Comm);
  MPI_Comm_rank(communicator, &my_rank);
  MPI_Comm_size(communicator, &size);

  size_t* elements_per_proc = (size_t*)memkind_malloc(MEMKIND_DEFAULT, sizeof(size_t) * size);
  for (i = 0; i < size; i++) elements_per_proc[i] = 0;
  for (i = 0; i < num_blocks; i++) {
    elements_per_proc[allocation_blocks[i].process] += (allocation_blocks[i].endElement - allocation_blocks[i].startElement) + 1;
  }
  void* buffer = (char*)memkind_malloc(dist_kind->memkind, elements_per_proc[my_rank] * element_size);
  put_info_entry_into_state(dist_kind, buffer, size, element_size, number_elements, elements_per_proc[my_rank], elements_per_proc,
                            communicator);
  return buffer;
}

struct distmem_block* mpi_contiguous_distribution(int* numberBlocks, struct distmem* dist_kind, size_t element_size,
                                                  size_t number_elements, int nargs, ...) {
  va_list ap;
  va_start(ap, nargs);
  MPI_Comm communicator = va_arg(ap, MPI_Comm);

  size_t dimension_division, dimension_extra, current_block_start = 0;
  int size, i;
  MPI_Comm_size(communicator, &size);

  dimension_division = number_elements / size;
  dimension_extra = number_elements - (dimension_division * size);

  struct distmem_block* blocks = (struct distmem_block*)memkind_malloc(MEMKIND_DEFAULT, sizeof(struct distmem_block) * size);
  for (i = 0; i < size; i++) {
    blocks[i].process = i;
    blocks[i].startElement = current_block_start;
    current_block_start += dimension_division + (i < dimension_extra ? 1 : 0);
    blocks[i].endElement = current_block_start - 1;
  }
  return blocks;
}

struct distmem_mpi_memory_information* distmem_mpi_get_info(memkind_t kind, void* ptr) {
  struct distmem* dist_kind = distmem_get_distkind_by_name(kind->name);
  return (struct distmem_mpi_memory_information*)distmem_get_specific_entry(dist_kind, ptr);
}

static void put_info_entry_into_state(struct distmem* dist_kind, void* local_buffer, int procs_distributed_over, size_t element_size,
                                      size_t total_number_elements, size_t number_local_elements, size_t* elements_per_proc,
                                      MPI_Comm communicator) {
  struct distmem_mpi_memory_information* specific_state =
      (struct distmem_mpi_memory_information*)memkind_malloc(MEMKIND_DEFAULT, sizeof(struct distmem_mpi_memory_information));
  specific_state->local_buffer = local_buffer;
  specific_state->element_size = element_size;
  specific_state->total_number_elements = total_number_elements;
  specific_state->number_local_elements = number_local_elements;
  specific_state->procs_distributed_over = procs_distributed_over;
  specific_state->elements_per_process = elements_per_proc;
  specific_state->communicator = communicator;
  distmem_put_specific_entry_into_state(dist_kind, specific_state, local_buffer, deallocate_specific_information);
}

static void deallocate_specific_information(void* specific_info) {
  if (specific_info != NULL) {
    struct distmem_mpi_memory_information* specific_state = (struct distmem_mpi_memory_information*)specific_info;
    if (specific_state->elements_per_process != NULL) memkind_free(MEMKIND_DEFAULT, specific_state->elements_per_process);
    memkind_free(MEMKIND_DEFAULT, specific_state);
  }
}
