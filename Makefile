.PHONY: all clean

.SUFFIXES:

CC      = gcc
CFLAGS  = -Wall
LDFLAGS =
LIBS    += -lpthread

INCLUDE = csapp.h ftpproto.h
OBJS    = csapp.o
INCLDIR = -I.

PROGS = q1 q3 slaveFtp masterFtp clientFtp

all: $(PROGS)

%.o: %.c $(INCLUDE)
	$(CC) $(CFLAGS) $(INCLDIR) -c -o $@ $<

%: %.o $(OBJS)
	$(CC) -o $@ $(LDFLAGS) $^ $(LIBS)

clean:
	rm -f $(PROGS) *.o
