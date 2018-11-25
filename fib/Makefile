CXX      = icpc
CC       = icc
FLAGS    = -g -Wall -O3 -qopenmp -I../../../llvm_omp_root/include -I../timer
LDFLAGS  = -L../../../llvm_omp_root/lib -L../timer -lgomp -lftimer
SRCS     = fib.cc
OBJS     = $(SRCS:.cc=.o)


all: fib


.cc.o:
	$(CXX) $(FLAGS) -c $< -o $@
	
	
fib: $(OBJS)
	$(CXX) $(LDFLAGS) $^ -o $@
	

clean:
	rm -f $(OBJS) *~ *.dot *.log fib
