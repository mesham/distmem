#include "distmem_mpi.h"
#include <memkind.h>
#include <mpi.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
	MPI_Init(&argc, &argv);
	int myrank;
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
	distmem_mpi_init();
	int * buffer=(int*) distmem_mpi_malloc(MPI_CONTIGUOUS_KIND, 4, 17, MPI_COMM_WORLD);
	struct distmem_mpi_memory_information * alloc_info=distmem_mpi_get_info(MPI_CONTIGUOUS_KIND, buffer);
	printf("Total number elements=%zu, distributed over %d procs, local number elements=%zu\n", alloc_info->total_number_elements,
			alloc_info->procs_distributed_over, alloc_info->number_local_elements);
	int i;
	for (i=0;i<alloc_info->number_local_elements;i++) {
		buffer[i]=i*(myrank+1);
	}
	memkind_free(MPI_CONTIGUOUS_KIND, buffer);
	MPI_Finalize();
	return 0;
}
