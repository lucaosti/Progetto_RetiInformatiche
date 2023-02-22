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

// Carica i tavoli dal file e li mette nell'array
void caricaTavoli() {
	
}

// Carica il menu dal file e lo mette nell'array
void caricaMenu() {
	
}

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

// Riceve dal socket in input il messaggio e lo mette dentro buffer
int ricevi(int j, int lunghezza,char* buffer) {
	int ret;
	ret = recv(j, (void*)buffer, lunghezza, 0);
	return ret;
}

// Ricevi dal socket in input la lunghezza del messaggio e lo mette dentro lmsg
int riceviLunghezza(int j, int *lmsg) {
	int ret;
	ret = recv(j, (void*)lmsg, sizeof(uint16_t), 0);
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

// Inserisce in base alla lettera c, il socket id nell'array relativo
int inserisci(int i, char c) {
	int j = 0;
	switch (c)
	{
	case 'c': // Client
		for(; j < nMaxClient; j++){
			if(socket_client[j] != -1){
				socket_client[j] = i;
				break;
			}
		}
		return 1;
	case 't': // Kitchen device
		for(; j < nMaxKd; j++){
			if(socket_kd[j] != -1){
				socket_kd[j] = i;
				break;
			}
		}
		return 1;
	case 'k': // Table device
		for(; j < nMaxTd; j++){
			if(socket_td[j] != -1){
				socket_td[j] = i;
				break;
			}
		}
		return 1;
	
	default:
		return -1;
	}
}

// Gestisce UNA richiesta da parte di un client
void gestisciClient(int socketId, char* b) {
	// Come prima cosa copio il buffer in entrata (per riferimento) in quello locale
	char buffer[1024];
	strcpy(buffer, b);
	
	// Gestisce tre tipi di comandi:
	//   - find, e, di conseguenza, dopo;
	//       - book;
	//   - disconnessione.
}

// Gestisce UNA richiesta da parte di un table device
void gestisciTd(int socketId, char* b) {
	// Come prima cosa copio il buffer in entrata (per riferimento) in quello locale
	char buffer[1024];
	strcpy(buffer, b);
}

// Gestisce UNA richiesta da parte di un kitchen device
void gestisciKd(int socketId, char* b) {
	// Come prima cosa copio il buffer in entrata (per riferimento) in quello locale
	char buffer[1024];
	strcpy(buffer, b);
}

// Dealloca tutte le strutture
void deallocaStrutture() {
	/* NELLA FORMA
	void List_destory(List * list){
	if(list == NULL)
		return;
	List_destroy(list->next);
	free(list->str);
	free(list);
	}
	*/
}