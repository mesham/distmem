CC=mpicc
CFLAGS=-g -fpic -I ../memkind_build/include -I ../../GPI2-1.2.0/include

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)
	
distmem_mpi: src/distmem.o src/distmem_mpi.o
	gcc -shared -o libdistmem.so src/distmem.o src/distmem_mpi.o
	
distmem_gaspi: src/distmem.o src/distmem_gaspi.o
	gcc -shared -o libdistmem.so src/distmem.o src/distmem_gaspi.o

distmem: src/distmem.o src/distmem_mpi.o src/distmem_gaspi.o
	gcc -shared -o libdistmem.so src/distmem.o src/distmem_mpi.o src/distmem_gaspi.o
	
clean:
	rm -f src/*.o libdistmem.so	