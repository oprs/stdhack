
CPPFLAGS += -O2 -Wall -Wstrict-prototypes
LDFLAGS  +=

CC       := gcc
RM       := rm -f
INSTALL  := /usr/bin/install
DESTDIR  := /usr/local

default: stdhack

stdhack: stdhack.o
	$(CC) $(LDFLAGS) -o $@ $<

stdhack.o: stdhack.c
	$(CC) $(CPPFLAGS) -o $@ -c $<

install:
	$(INSTALL) -d -m 0755 $(DESTDIR)/bin
	$(INSTALL) -s -m 0755 stdhack $(DESTDIR)/bin/stdhack

clean:
	$(RM) stdhack stdhack.o

