CXX      = icpc
CC       = icc
FLAGS    = -g -Wall -O3 -fpic
SRCS	 = timer.c
OBJS     = $(SRCS:.c=.o)
LIB      = libftimer.so
EXEC	 = query_cpu_freq


all: $(LIB) $(EXEC)


.c.o:
	$(CC) $(FLAGS) -c $< -o $@
	

.cc.o:
	$(CXX) $(FLAGS) -std=c++11 -c $< -o $@
	
	
$(LIB): $(OBJS)
	$(CC) -shared -o $@ $^


$(EXEC): query_cpu_freq.o
	$(CXX) -o $@ $^


clean:
	rm -f $(OBJS) *~ $(LIB) $(EXEC) query_cpu_freq.o

