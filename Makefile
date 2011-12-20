SOURCE=$(wildcard *.c)
PROGRAM=file2bin
EMPTY=

ifeq ($(shell which clang),$(empty))
$(info * Using default compiler)
else
$(info * Using clang compiler)
	CC = $(shell which clang)
endif

CFLAGS+=-Wextra -Wall -Werror -g3 -std=c99 -D_GNU_SOURCE

all: $(PROGRAM)

$(PROGRAM): $(SOURCE) Makefile
	$(CC) $(SOURCE) $(CFLAGS) -o $@ 
clean:
	rm -f $(PROGRAM)
