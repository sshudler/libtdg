CXX      = icpc
CC       = icc
FLAGS    = -g -Wall -O3 -fpic -std=c++11 -I. -Itimer -DHAVE_PAPI #-DLIBTDG_TRACE
LDFLAGS  = -shared -Ltimer -lftimer -lpapi
SRCS     = init.cc callbacks.cc graph.cc metrics.cc
SRCSE    = init_empty.cc callbacks_empty.cc graph.cc metrics.cc
OBJS     = $(SRCS:.cc=.o)
OBJSE    = $(SRCSE:.cc=.o)
LIB      = libtdg.so


all: $(LIB)


.cc.o:
	$(CXX) $(FLAGS) -c $< -o $@


$(LIB): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^


empty: $(OBJSE)
	$(CXX) $(LDFLAGS) -o $(LIB) $^


clean:
	rm -f $(OBJS) $(OBJSE) *~ $(LIB)

