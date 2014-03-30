CC = gcc
CFLAGS = -Wall -pedantic
OBJ = main

all: $(OBJ)

%: %.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	@rm -f $(OBJ)
