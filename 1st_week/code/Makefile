CC = gcc
CFLAGS = -Wall -O2

TARGETS = client_jty server_jty

all: $(TARGETS)

client_jty: client_jty.c
	$(CC) $(CFLAGS) -o $@ $<

server_jty: server_jty.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGETS) *.o

.PHONY: all clean
