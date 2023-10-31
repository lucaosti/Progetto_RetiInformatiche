all: server client td kd

server: server.c strutture.h funzioni.c funzioni.h
	gcc -Wall -std=c11 -o server server.c funzioni.c

client: client.c
	gcc -Wall -std=c11 -o client client.c

td: td.c
	gcc -Wall -std=c11 -o td td.c

kd: kd.c
	gcc -Wall -std=c11 -o kd kd.c

clean:
	rm -f server client td kd
