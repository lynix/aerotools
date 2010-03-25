CC = gcc
CFLAGS = -Wall -pedantic -I /usr/include -lusb
DESTDIR = /usr/local

aerocli : aerocli.o device.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

aerocli.o : aerocli.c aerocli.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ -c $<

device.o : device.c device.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ -c $<

install: aerocli
	mkdir -p $(DESTDIR)/sbin
	install -m 755 vcp $(DESTDIR)/sbin/aerocli

clean :
	rm -f aerocli *.o

.PHONY: clean install
