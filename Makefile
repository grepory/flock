CC	= cc
CFLAGS	= -g -O -W -Wall
LDFLAGS	=
INSTALL	= install
LIBS    =
prefix	= /usr/local
BINDIR	= $(prefix)/bin
MANDIR	= $(prefix)/man/man1
VERSION = $(shell cat version)

SOURCES = flock.c flock.1 flock.spec Makefile version

all:	flock

flock: flock.c version
	$(CC) $(CFLAGS) $(LDFLAGS) -DVERSION=\"$(VERSION)\" -o $@ $< $(LIBS)

install:
	mkdir -m 755 -p $(INSTALLROOT)$(BINDIR) $(INSTALLROOT)$(MANDIR)
	$(INSTALL) -m 755 flock $(INSTALLROOT)$(BINDIR)
	$(INSTALL) -m 644 flock.1 $(INSTALLROOT)$(MANDIR)

clean:
	rm -f flock

distclean: clean
	rm -f *~ \#*


tar:
	rm -rf flock-$(VERSION) flock-$(VERSION).tar flock-$(VERSION).tar.gz
	mkdir flock-$(VERSION)
	cp $(SOURCES) flock-$(VERSION)
	tar cvvf flock-$(VERSION).tar flock-$(VERSION)
	gzip -9 flock-$(VERSION).tar
	rm -rf flock-$(VERSION)
