/*
 * distmem_gaspi.c
 *
 *  Created on: 25 Nov 2015
 *      Author: nick
 */

#include "distmem_gaspi.h"
#include <memkind.h>
#include <memkind/internal/memkind_pmem.h>
#include <sys/param.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include "GASPI.h"

static void *my_pmem_mmap(struct memkind *, void *, size_t);

memkind_t GASPI_CONTIGUOUS_KIND;

struct memkind_ops MEMKIND_MY_OPS = {.create = memkind_pmem_create,
                                     .destroy = memkind_pmem_destroy,
                                     .malloc = memkind_arena_malloc,
                                     .calloc = memkind_arena_calloc,
                                     .posix_memalign = memkind_arena_posix_memalign,
                                     .realloc = memkind_arena_realloc,
                                     .free = memkind_default_free,
                                     .mmap = my_pmem_mmap,
                                     .get_mmap_flags = memkind_pmem_get_mmap_flags,
                                     .get_arena = memkind_thread_get_arena,
                                     .get_size = memkind_pmem_get_size, };

static struct distmem_ops GASPI_DISTRIBUTED_MEMORY_VTABLE = {.dist_malloc = distmem_gaspi_arena_malloc,
                                                             .dist_create = distmem_arena_create};

static struct distmem_ops GASPI_SEGMENT_MEMORY_VTABLE = {
    .dist_malloc = distmem_gaspi_arena_malloc, .dist_create = distmem_arena_create, .memkind_operations = &MEMKIND_MY_OPS};

static void put_info_entry_into_state(struct distmem *, void *, int, size_t, size_t, size_t);

void distmem_gaspi_init() {
  init_distmem();
  int err = distmem_create_default(&GASPI_DISTRIBUTED_MEMORY_VTABLE, "gaspi contiguous", &GASPI_CONTIGUOUS_KIND);
  if (err) {
    fprintf(stderr, "Error allocating distributed kind\n");
  }
}

static void *my_pmem_mmap(struct memkind *kind, void *addr, size_t size) {
  struct memkind_pmem *priv = kind->priv;
  void *tr = priv->addr + priv->offset;
  priv->offset += size;
  return mmap(tr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
}

void distmem_gaspi_create_segment_kind(const char *name, memkind_t *kind, gaspi_segment_id_t segment_id, gaspi_size_t segment_size) {
  distmem_create(&GASPI_SEGMENT_MEMORY_VTABLE, name, kind);
  gaspi_pointer_t segptr;
  gaspi_segment_ptr(segment_id, &segptr);

  void *addr = segptr;
  size_t Chunksize = 4 * 1024 * 1024;
  size_t s = sizeof(Chunksize);
  jemk_mallctl("opt.lg_chunk", &Chunksize, &s, NULL, 0);
  void *aligned_addr = (void *)roundup((uintptr_t)addr, Chunksize);
  struct memkind_pmem *priv = (*kind)->priv;
  priv->fd = 0;
  priv->addr = addr;
  priv->max_size = roundup((size_t)segment_size, Chunksize);
  priv->offset = (uintptr_t)aligned_addr - (uintptr_t)addr;
}

void *distmem_gaspi_malloc(memkind_t kind, size_t element_size, size_t number_elements, gaspi_group_t gaspi_group) {
  struct distmem *dist_kind = distmem_get_distkind_by_name(kind->name);
  return dist_kind->operations->dist_malloc(dist_kind, element_size, number_elements, 1, gaspi_group);
}

void *distmem_gaspi_arena_malloc(struct distmem *dist_kind, size_t element_size, size_t number_elements, int nargs, ...) {
  va_list ap;
  va_start(ap, nargs);
  gaspi_group_t gaspi_group = (gaspi_group_t)va_arg(ap, int);  // As always promoted to int when passed through vargs

  int i, size, my_rank = -1;
  gaspi_rank_t my_proc_rank;
  gaspi_group_size(gaspi_group, &size);
  gaspi_proc_rank(&my_proc_rank);
  gaspi_rank_t group_ranks[size];
  gaspi_group_ranks(gaspi_group, group_ranks);
  for (i = 0; i < size; i++) {
    if (group_ranks[i] == my_proc_rank) {
      my_rank = i;
      break;
    }
  }
  if (my_rank == -1) return NULL;
  size_t number_local_elements, dimension_division, dimension_extra;
  number_local_elements = number_elements / size;

  dimension_division = number_elements / size;
  dimension_extra = number_elements - (dimension_division * size);

  number_local_elements = dimension_division + (my_rank < dimension_extra ? 1 : 0);
  void *buffer = (char *)memkind_malloc(dist_kind->memkind, number_local_elements * element_size);
  put_info_entry_into_state(dist_kind, buffer, size, element_size, number_elements, number_local_elements);
  return buffer;
}

struct distmem_gaspi_memory_information *distmem_gaspi_get_info(memkind_t kind, void *ptr) {
  struct distmem *dist_kind = distmem_get_distkind_by_name(kind->name);
  return (struct distmem_gaspi_memory_information *)distmem_get_specific_entry(dist_kind, ptr);
}

static void put_info_entry_into_state(struct distmem *dist_kind, void *local_buffer, int procs_distributed_over, size_t element_size,
                                      size_t total_number_elements, size_t number_local_elements) {
  struct distmem_gaspi_memory_information *specific_state =
      (struct distmem_gaspi_memory_information *)memkind_malloc(MEMKIND_DEFAULT, sizeof(struct distmem_gaspi_memory_information));
  specific_state->local_buffer = local_buffer;
  specific_state->element_size = element_size;
  specific_state->total_number_elements = total_number_elements;
  specific_state->number_local_elements = number_local_elements;
  specific_state->procs_distributed_over = procs_distributed_over;
  distmem_put_specific_entry_into_state(dist_kind, specific_state, local_buffer, NULL);
}
