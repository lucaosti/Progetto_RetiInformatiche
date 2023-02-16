all: server cli td kd

server: server.c strutture.h funzioni.h
	gcc -Wall server.c -o server

funzioni.h: funzioni.c
	gcc -Wall funzioni.c -o funzioni

client: client.c
	gcc -Wall client.c -o cli

td: td.c
	gcc -Wall td.c -o td

kd: kd.c
	gcc -Wall kd.c -o kd

clean:
	rm -f server client td kd