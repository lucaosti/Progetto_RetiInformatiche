all: server client td kd

server: server.c funzioni.c funzioni.h strutture.h
	gcc -Wall -o server server.c funzioni.c

client: client.c
	gcc -Wall -o client client.c

td: td.c
	gcc -Wall -o td td.c

kd: kd.c
	gcc -Wall -o kd kd.c

clean:
	rm -f server client td kd
