/* Contiene le funzioni necessarie al Server */
/* ----------------------------------------- */

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include "strutture.h"
#include "funzioni.h"

// Ritorna 1 nel caso ci siano attualmente delle comande
// "in_preparazione" o "in_servizio". 0 altrimenti
int comandeInSospeso() {
	for (int i = 0; i < nTavoli; i++) {
		struct comanda *c;
		while(c->prossima != NULL) {
			if(c->stato == in_attesa || c->stato == in_preparazione) return 1;
		}
	}
	return 0;
}

// Invia al socket in input il messaggio dentro buffer
int invia(int j, char* buffer) {
	int len = hton(strlen(buffer));
	int lmsg = htons(len);
	int ret;

	// Invio la dimensione del messaggio
	ret = send(j, (void*) &lmsg, sizeof(uint16_t), 0);
	// Invio il messaggio
	ret = send(j, (void*) buffer, len, 0);

	// Comunico l'esito
	return ret;
}

// Prende uno stato_comanda e inserisce dentro il buffer tutte le informazioni
// delle comande in quello stato di qualunque tavolino
void elencoComande(char* buffer, enum stato_comanda stato) {
	for (int i = 0; i < nTavoli; i++) {
		struct comanda *c;
		c = &comande[i];
		while(c != NULL) {
			strcat(buffer, ""); // Manca il modo di inserire i dettagli della comanda
			strcat(buffer, "\n");
			// Vado avanti
			c = c->prossima;
		}
	}
}

// Prende un tavolo e inserisce dentro il buffer tutte le informazioni
// delle comande inerenti a quel tavolino
void elencoComandeTavolo(char* buffer, int tavolo) {
	struct comanda *c;
	c = &comande[tavolo];
	while(c != NULL) {
		strcat(buffer, ""); // Manca il modo di inserire i dettagli della comanda
		strcat(buffer, "\n");
		// Vado avanti
		c = c->prossima;
	}
}