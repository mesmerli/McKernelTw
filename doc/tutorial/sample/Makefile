PPOS_INSTALL=/opt/ppos
CC            = icc
FORT          = ifort
#FLAGS         = -mmic
FLAGS         =
#LDFLAGS       = -Wl,-rpath=$(PPOS_INSTALL)/attached/lib/ -Wl,-rpath=/usr/linux-k1om-4.7/linux-k1om/lib64/ -Wl,-rpath=/usr/linux-k1om-4.7/x86_64-k1om-linux/lib64/ -Wl,-rpath=/opt/intel/composerxe/lib/mic/
LDFLAGS       =
PROGRAM       = chello fhello picpth picomp pifomp


all:            $(PROGRAM)

chello:         hello.c 
		$(CC) $(FLAGS) $(LDFLAGS) -o chello hello.c

fhello:         hello.f90
		$(FORT) $(FLAGS) $(LDFLAGS) -o fhello hello.f90

picpth:         pi_pth.c 
		$(CC) $(FLAGS) $(LDFLAGS) -lpthread -o picpth pi_pth.c

picomp:         pi_omp.c 
		$(CC) $(FLAGS) $(LDFLAGS) -openmp -o picomp pi_omp.c

pifomp:         pi_omp.f90 
		$(FORT) $(FLAGS) $(LDFLAGS) -openmp -o pifomp pi_omp.f90

clean:;         rm -f $(PROGRAM)


