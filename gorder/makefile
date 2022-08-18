CC      = g++
CPPFLAGS= -Wno-deprecated -O3 -c -m64 -march=native -std=c++11 -DGCC -DRelease
LDFLAGS = -static -O3 -m64
SOURCES = main.cpp Util.cpp Graph.cpp UnitHeap.cpp
OBJECTS = $(SOURCES:.cpp=.o)
EXECUTABLE=Gorder

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE) : $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o : 
	$(CC) $(CPPFLAGS) $< -o $@

clean:
	rm -f *.o

