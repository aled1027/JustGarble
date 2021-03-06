# ***********************************************
#                    JustGarble
# ***********************************************

CC=gcc
INCLUDES = -Iinclude
CFLAGS=-O3 -Wall -maes -msse4 -lmsgpack -lcrypto -lssl -march=native $(INCLUDES)

SRCDIR   = src
SOURCES := $(wildcard $(SRCDIR)/*.c)
CIRCDIR = circuit
CIRCUITS := $(wildcard $(CIRCDIR)/*.c)
TESTDIR   = test

BINDIR   = bin

SRCOBJS = $(SOURCES:.c=.o)
CIRCOBJS = $(CIRCUITS:.c=.o)

AND = ANDTest
AES = AESFullTest
LARGE = LargeCircuitTest
FILE = CircuitFileTest
rm = rm -f

.PHONEY: clean

all: AND AES LARGE FILE

AND: $(SRCOBJS) $(CIRCOBJS)
	$(CC) $(SRCOBJS) $(CIRCOBJS) $(TESTDIR)/$(AND).c -o $(BINDIR)/$(AND).out $(CFLAGS) 

AES: $(SRCOBJS) $(CIRCOBJS)
	$(CC) $(SRCOBJS) $(CIRCOBJS) $(TESTDIR)/$(AES).c -o $(BINDIR)/$(AES).out $(CFLAGS) 

LARGE: $(SRCOBJS) $(CIRCOBJS)
	$(CC) $(SRCOBJS) $(CIRCOBJS) $(TESTDIR)/$(LARGE).c -o $(BINDIR)/$(LARGE).out $(CFLAGS) 

FILE: $(SRCOBJS) $(CIRCOBJS)
	$(CC) $(SRCOBJS) $(CIRCOBJS) $(TESTDIR)/$(FILE).c -o $(BINDIR)/$(FILE).out $(CFLAGS)

.c.o:
	$(CC) -c $< -o $@ $(CFLAGS)

clean:
	@$(rm) $(SRCDIR)/*.o
	@$(rm) $(CIRCDIR)/*.o
	@$(rm) $(BINDIR)/$(AES).out
	@$(rm) $(BINDIR)/$(LARGE).out
	@$(rm) $(BINDIR)/$(FILE).out

