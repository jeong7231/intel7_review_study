CC = gcc
CFLAGS = -Wall -O2
MYSQL_CFLAGS := $(shell mysql_config --cflags)
MYSQL_LIBS := $(shell mysql_config --libs)

TARGETS = client_jty server_jty

all: $(TARGETS)

client_jty: client_jty.c
	$(CC) $(CFLAGS) $(MYSQL_CFLAGS) -o $@ $< $(MYSQL_LIBS)

server_jty: server_jty.c
	$(CC) $(CFLAGS) $(MYSQL_CFLAGS) -o $@ $< $(MYSQL_LIBS)

clean:
	rm -f $(TARGETS) *.o

.PHONY: all clean
