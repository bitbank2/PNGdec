CFLAGS=-D__LINUX__ -ggdb -Wall -O2 
LIBS = 

all: libpngdec.a

libpngdec.a: PNGdec.o adler32.o crc32.o infback.o inffast.o inflate.o inftrees.o zutil.o
	ar -rc libpngdec.a PNGdec.o adler32.o crc32.o infback.o inffast.o inflate.o inftrees.o zutil.o ;\
	sudo cp libpngdec.a /usr/local/lib ;\
	sudo cp ../src/PNGdec.h /usr/local/include ;\
	sudo cp ../src/zutil.h /usr/local/include ;\
	sudo cp ../src/inftrees.h /usr/local/include ;\
	sudo cp ../src/inflate.h /usr/local/include

PNGdec.o: ../src/PNGdec.cpp ../src/png.inl ../src/PNGdec.h
	$(CXX) $(CFLAGS) -c ../src/PNGdec.cpp

adler32.o: ../src/adler32.c
	$(CC) $(CFLAGS) -c ../src/adler32.c

crc32.o: ../src/crc32.c
	$(CC) $(CFLAGS) -c ../src/crc32.c

infback.o: ../src/infback.c
	$(CC) $(CFLAGS) -c ../src/infback.c

inffast.o: ../src/inffast.c
	$(CC) $(CFLAGS) -c ../src/inffast.c

inflate.o: ../src/inflate.c
	$(CC) $(CFLAGS) -c ../src/inflate.c

inftrees.o: ../src/inftrees.c
	$(CC) $(CFLAGS) -c ../src/inftrees.c

zutil.o: ../src/zutil.c
	$(CC) $(CFLAGS) -c ../src/zutil.c

clean:
	rm -rf *.o libpngdec.a
