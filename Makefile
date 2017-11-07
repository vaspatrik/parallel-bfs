CXX=g++
RM=rm -f
CPPFLAGS=-pthread -std=c++14 -g 
LDFLAGS=-L/usr/libx86_64-linux-gnu -L/usr/local/lib
LDLIBS=-ltbb -lemon

SRCS=bfs.cpp 
OBJS=$(SRCS:.cpp=.o)

all: bfs

bfs: $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS) 

depend: .depend

.depend: $(SRCS)
	$(RM) ./.depend
	$(CXX) $(CPPFLAGS) -MM $^>>./.depend;

clean:
	$(RM) $(OBJS)

distclean: clean
	$(RM) *~ .depend

include .depend

