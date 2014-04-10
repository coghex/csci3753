CC           = gcc

CFLAGSFUSE   = `pkg-config fuse --cflags`
LLIBSFUSE    = `pkg-config fuse --libs`
LLIBSOPENSSL = -lcrypto

CFLAGS = -c -g -Wall -Wextra
LFLAGS = -g -Wall -Wextra

pa4: pa4.o
	$(CC) $(LFLAGS) $^ -o $@ $(LLIBSFUSE)
pa4.o: pa4.c
	$(CC) $(CFLAGS) $(CFLAGSFUSE) $<
clean:
	rm -f *.o
	rm -f pa4
	fusermount -u ./root

