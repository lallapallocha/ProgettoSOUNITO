CC = gcc

CFLAGS = -g -Wvla -Wextra -Werror -Wall

DEBUG_CFLAGS = $(CFLAGS) -D DEBUG

DEPS = src/utils.h Makefile

BINARIES = bin/master bin/alimentazione bin/atomo bin/attivatore

all: $(BINARIES)

bin/utils.o: src/utils.c $(DEPS)
	$(CC) $(CFLAGS) -c $< -o $@

bin/alimentazione: src/alimentazione.c bin/utils.o $(DEPS)
	$(CC) $(CFLAGS) $< bin/utils.o -o $@

bin/atomo: src/atomo.c bin/utils.o $(DEPS)
	$(CC) $(CFLAGS) $< bin/utils.o -o $@

bin/attivatore: src/attivatore.c bin/utils.o $(DEPS)
	$(CC) $(CFLAGS) $< bin/utils.o -o $@

bin/master: src/master.c bin/utils.o $(DEPS)
	$(CC) $(CFLAGS) $< bin/utils.o -o $@

debug: bin/utils.o $(DEPS)
	$(CC) $(DEBUG_CFLAGS) src/master.c bin/utils.o -o bin/master
	$(CC) $(DEBUG_CFLAGS) src/alimentazione.c bin/utils.o -o bin/alimentazione
	$(CC) $(DEBUG_CFLAGS) src/atomo.c bin/utils.o -o bin/atomo
	$(CC) $(DEBUG_CFLAGS) src/attivatore.c bin/utils.o -o bin/attivatore

run: 
	./bin/master

clean:
	rm -f bin/*

gdb:
	gdb ./bin/master