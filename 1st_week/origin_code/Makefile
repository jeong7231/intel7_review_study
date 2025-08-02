CC = gcc
CFLAGS = -Wall -O2

TARGETS = iot_client iot_server

all: $(TARGETS)

iot_client: iot_client.c
	$(CC) $(CFLAGS) -o $@ $<

iot_server: iot_server.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGETS) *.o

.PHONY: all clean
