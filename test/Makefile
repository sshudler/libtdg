CXX      = icpc
CC       = icc
FLAGS    = -g -Wall -qopenmp -I../../../llvm_omp_root/include -I../timer
LDFLAGS  = -L../../../llvm_omp_root/lib -L../timer -lgomp -lftimer
SRCS     = loop.c
OBJS     = $(SRCS:.c=.o)


all: loop


.c.o:
	$(CC) $(FLAGS) -c $< -o $@
	
	
loop: $(OBJS)
	$(CC) $(LDFLAGS) $^ -o $@
	

clean:
	rm -f $(OBJS) *~ *.dot *.log loop loop-dev
