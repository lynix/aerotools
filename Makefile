CC = gcc
CFLAGS = -O2 -Wall -pedantic -std=gnu99 -I /usr/include -g
PREFIX ?= /usr
INSTALLDIR = $(DESTDIR)$(PREFIX)

all : aerocli aerod

aerocli : aerocli.o libaquaero.o
	$(CC) $(CFLAGS) -o $@ $^ -lusb-1.0 $(LDFLAGS)
	
aerod : aerod.o libaquaero.o
	$(CC) $(CFLAGS) -o $@ $^ -lusb-1.0 -lpthread $(LDFLAGS) 

aerocli.o : aerocli.c aerocli.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ -c $<
	
aerod.o : aerod.c aerod.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ -c $<

libaquaero.o : libaquaero.c libaquaero.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ -c $<

install: aerocli aerod
	mkdir -p $(INSTALLDIR)/bin
	install -m 755 aerocli $(INSTALLDIR)/bin/aerocli
	install -m 755 aerod $(INSTALLDIR)/bin/aerod

uninstall:
	rm -f $(INSTALLDIR)/bin/aerocli
	rm -f $(INSTALLDIR)/bin/aerod

clean :
	rm -f aerocli aerod *.o

.PHONY: clean install
