SOURCE=$(wildcard *.c)
PROGRAM=file2bin


CFLAGS+=-Wall -Werror -g3

all: $(PROGRAM)

$(PROGRAM): $(SOURCE) Makefile
	$(CC) $(SOURCE) $(CFLAGS) -o $@ 
clean:
	rm -f $(PROGRAM)
