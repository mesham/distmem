CC=mpicc
CFLAGS=-g -I ../src -I ../../memkind/src -I ../../../GPI2-1.2.0/include

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

all: helloworld_mpi helloworld_gaspi

helloworld_mpi: helloworld_mpi.o
	$(CC) -o hello_mpi helloworld_mpi.o -ldistmem -lmemkind -L../ -L../../memkind/libs /home/nick/projects/intertwine/GPI2-1.2.0/lib64/libGPI2.a -lpthread

helloworld_gaspi: helloworld_gaspi.o
	$(CC) -o hello_gaspi helloworld_gaspi.o -ldistmem -lmemkind -L../ -L../../memkind/libs /home/nick/projects/intertwine/GPI2-1.2.0/lib64/libGPI2.a -lpthread