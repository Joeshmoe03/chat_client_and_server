CFLAGS=-Wall -g -pedantic

PROGS=chat-server chat-client

chat-server: chat-server.c
	gcc $(CFLAGS) -o $@ $^

chat-client: chat-client.c
	gcc $(CFLAGS) -o $@ $^

.PHONY: clean
clean:
	rm -f *.o $(PROGS)
