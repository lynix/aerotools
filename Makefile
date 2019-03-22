CC        ?= gcc
CFLAGS     = -Wall -ansi -std=gnu99 -pedantic -I /usr/include -O2
PREFIX    ?= /usr
INSTALLDIR = $(DESTDIR)$(PREFIX)
BINARIES   = bin/aerocli bin/aerod

ifdef DEBUG  
	CFLAGS += -g
endif


all : $(BINARIES)


$(BINARIES) : bin/%: obj/%.o obj/libaquaero.o
	@mkdir -p bin
	$(CC) $(CFLAGS) -o $@ $^ -pthread -lusb-1.0 $(LDFLAGS)
	
obj/%.o : src/%.c src/%.h
	@mkdir -p obj
	$(CC) $(CFLAGS) -o $@ -c $<
	

install: $(BINARIES)
	mkdir -p $(INSTALLDIR)/bin
	install -m 755 bin/aerocli $(INSTALLDIR)/bin/aerocli
	install -m 755 bin/aerod $(INSTALLDIR)/bin/aerod

uninstall:
	rm -f $(INSTALLDIR)/bin/aerocli
	rm -f $(INSTALLDIR)/bin/aerod

clean :
	rm -f $(BINARIES) obj/*.o

.PHONY: clean install uninstall
