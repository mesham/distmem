/*
 * helloworld_gaspi.c
 *
 *  Created on: 25 Nov 2015
 *      Author: nick
 */

#include <stdio.h>
#include <mpi.h>
#include "distmem_gaspi.h"
#include "GASPI.h"

int main(int argc, char *argv[]) {
	MPI_Init(&argc, &argv);
	gaspi_proc_init(GASPI_BLOCK);
	gaspi_group_commit(GASPI_GROUP_ALL, GASPI_BLOCK);
	distmem_gaspi_init();
	// Contiguous which is created by our code in initialise
	int * buffer=(int*) distmem_gaspi_malloc(GASPI_CONTIGUOUS_KIND, 4, 17,  GASPI_GROUP_ALL);
	struct distmem_gaspi_memory_information * alloc_info=distmem_gaspi_get_info(GASPI_CONTIGUOUS_KIND, buffer);
	printf("[Contiguous mem] Total number elements=%zu, distributed over %d procs, local number elements=%zu\n", alloc_info->total_number_elements,
			alloc_info->procs_distributed_over, alloc_info->number_local_elements);
	memkind_free(GASPI_CONTIGUOUS_KIND, buffer);

	// Now create a GASPI segment and create memory kind that represents this, managing this as a heap locally & distributed
	memkind_t GASPI_SEGMENT_KIND;
	gaspi_size_t segment_size = 25000000;
	gaspi_segment_id_t segment_id = 1;

	int ierr=gaspi_segment_alloc(segment_id, segment_size, GASPI_MEM_UNINITIALIZED);
	if (ierr != GASPI_SUCCESS) printf("ERROR!\n");

	distmem_gaspi_create_segment_kind("segment_kind", &GASPI_SEGMENT_KIND, segment_id, segment_size);
	gaspi_rank_t * my_rank=(gaspi_rank_t*) memkind_malloc(GASPI_SEGMENT_KIND, sizeof(gaspi_rank_t));
	gaspi_proc_rank(my_rank);
	printf("My rank held in segment is %d\n", *my_rank);
	memkind_free(GASPI_SEGMENT_KIND, my_rank);

	int * buffer2=(int*) distmem_gaspi_malloc(GASPI_SEGMENT_KIND, 4, 20,  GASPI_GROUP_ALL);
	alloc_info=distmem_gaspi_get_info(GASPI_SEGMENT_KIND, buffer2);
	printf("[Segment mem] Total number elements=%zu, distributed over %d procs, local number elements=%zu\n", alloc_info->total_number_elements,
			alloc_info->procs_distributed_over, alloc_info->number_local_elements);
	memkind_free(GASPI_SEGMENT_KIND, buffer2);
	gaspi_proc_term(GASPI_BLOCK);
	MPI_Finalize();
	return 0;
}
