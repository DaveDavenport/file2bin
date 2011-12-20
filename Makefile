PROGRAM=file2bin
PREFIX?=/usr/local/


CFLAGS+=-Wextra -Wall -Werror -g3 -std=c99 -D_GNU_SOURCE
SOURCE=$(wildcard *.c)
INSTALL=install
EMPTY=

# Check if clang compiler is available on the system, if so
# use this.
ifeq ($(shell which clang),$(empty))
$(info * Using default compiler)
else
$(info * Using clang compiler)
	CC = $(shell which clang)
endif

all: $(PROGRAM)

$(PROGRAM): $(SOURCE) Makefile
	$(info * Compiling $@ from $(SOURCE))
	@$(CC) $(SOURCE) $(CFLAGS) -o $@ 
	$(info * Done.)

clean:
	$(info * Removing $(PROGRAM))
	@rm -f $(PROGRAM)

install: $(PROGRAM)
	$(info * Installing $^ into $(PREFIX))
	@$(INSTALL) $(PROGRAM) $(PREFIX)/bin/
