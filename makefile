CC=mpicc
CFLAGS=-g -fpic -I ../memkind/src -I ../../GPI2-1.2.0/include

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

distmem: src/distmem.o src/distmem_mpi.o src/distmem_gaspi.o
	gcc -shared -o libdistmem.so src/distmem.o src/distmem_mpi.o src/distmem_gaspi.o