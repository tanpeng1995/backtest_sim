HDF5_HOME := ${HDF5_HOME}
CC := mpicc
CFLAGS := -O2 -g -I$(HDF5_HOME)/include
LDFLAGS := -L$(HDF5_HOME)/lib -lhdf5 -pthread


EXES := sim
SRCS := $(EXES:%=%.c)

all: $(EXES)

sim: sim.o config.o
	mpicc $^ $(LDFLAGS) -o $@

sim.o: sim.c

config.o: config.h config.c

.PHONY: clean

clean:
	rm -f $(EXES) *.o
