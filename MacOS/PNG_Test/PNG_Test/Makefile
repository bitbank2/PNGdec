CFLAGS=-D__LINUX__ -Wall -O2 
LIBS = 

all: png_demo

png_demo: main.o
	$(CC) main.o $(LIBS) -o png_demo 

main.o: main.cpp
	$(CXX) $(CFLAGS) -c main.cpp

clean:
	rm -rf *.o png_demo
