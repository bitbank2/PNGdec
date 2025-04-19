CFLAGS=-Wall -O2 -g -fsanitize=address,undefined
LIBS = 

all: png_demo

png_demo: main.o
	$(CC) main.o -fsanitize=address,undefined -g $(LIBS) -o png_demo 

main.o: main.cpp
	$(CXX) $(CFLAGS) -c main.cpp

clean:
	rm -rf *.o png_demo
