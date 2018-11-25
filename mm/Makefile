CXX      = icpc
CC       = icc
FLAGS    = -g -Wall -O3 -qopenmp -I$(HOME)/ompt/llvm_omp_root/include -I$(HOME)/ompt/repo/libtdg/timer
LDFLAGS  = -L$(HOME)/ompt/llvm_omp_root/lib -L$(HOME)/ompt/repo/libtdg/timer -lomp -lftimer
SRCS     = omp_mm.c
OBJS     = $(SRCS:.c=.o)
EXEC	 = omp_mm


all: $(EXEC)


.c.o:
	$(CC) $(FLAGS) -c $< -o $@


$(EXEC): $(OBJS)
	$(CC) $(LDFLAGS) $^ -o $@


clean:
	rm -f $(OBJS) *~ *.dot *.log $(EXEC)
