CC = gcc
CFLAGS = -Wall -pthread -pedantic
OBJ = main

all: $(OBJ)

%: %.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	@rm -f $(OBJ)
