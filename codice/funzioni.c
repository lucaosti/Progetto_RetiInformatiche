/* Contiene le funzioni necessarie al Server */
/* ----------------------------------------- */

#include "strutture.h"
#include "funzioni.h"

// Carica i tavoli dal file e li mette nell'array
void caricaTavoli() {
	
}

// Carica il menu dal file e lo mette nell'array
void caricaMenu() {
	for(int i = 0; i < nPiatti; i++) {
		struct piatto* p = malloc(sizeof(*p));
		strcpy(p->codice, "a");
		strcpy(p->nome, "a");
		p->prezzo = 1;
		menu[i] = p;
	}
}

// Ritorna 1 nel caso ci siano attualmente delle comande
// "in_preparazione" o "in_servizio"; 0 altrimenti
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

// Ricevi dal socket in input la lunghezza del messaggio e lo mette dentro lmsg
int riceviLunghezza(int j, int *lmsg) {
	int ret;
	ret = recv(j, (void*)lmsg, sizeof(uint16_t), 0);
	return ret;
}

// Riceve dal socket in input il messaggio e lo mette dentro buffer
int ricevi(int j, int lunghezza,char* buffer) {
	int ret;
	ret = recv(j, (void*)buffer, lunghezza, 0);
	return ret;
}

// Prende uno stato_comanda e inserisce dentro il buffer tutte le informazioni
// delle comande in quello stato di qualunque tavolino
void elencoComande(char* buffer, enum stato_comanda stato) {
	for(int i = 0; i < nTavoli; i++) {
		struct comanda *c;
		c = &comande[i];
		while(c != NULL) {
			strcat(buffer, "com");
			strcat(buffer, c->nComanda);
			strcat(buffer, " T");
			strcat(buffer, i);
			strcat(buffer, "\n");
			for(int j = 0; j < nPiatti; j++) {
				if(c->quantita[j] != 0) {
					strcat(buffer, menu[j]->codice);
					strcat(buffer, " ");
					strcat(buffer, c->quantita[j]);
					strcat(buffer, "\n");
				}
			}
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
		strcat(buffer, "com");
		strcat(buffer, c->nComanda);
		strcat(buffer, " ");
		strcat(buffer, c->stato);
		strcat(buffer, "\n");
		for(int j = 0; j < nPiatti; j++) {
			if(c->quantita[j] != 0) {
				strcat(buffer, menu[j]->codice);
				strcat(buffer, " ");
				strcat(buffer, c->quantita[j]);
				strcat(buffer, "\n");
			}
		}
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
	case 'k': // Kitchen device
		for(; j < nMaxKd; j++){
			if(socket_kd[j] != -1){
				socket_kd[j] = i;
				break;
			}
		}
		return 1;
	case 't': // Table device
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

// Prende i parametri della find ed inserisce nel buffer le disponibilità
void cercaDisponibilita(int nPers, time_t dataora, char* buffer, char* disponibilita[nTavoli]) {
	int numero = 0;
	for(int index = 0; index < nTavoli; index++) {
		if(tavoli[index].nPosti < nPers){
			disponibilita[index] = 0;
			continue;
		}
		struct prenotazione* punta = &prenotazioni[index];
		char esito = 1; // Non esiste bool
		while(punta->prossima != NULL) {
			if(punta->data_ora == dataora){
				esito = 0;
				break;
			}
		}
		if(!esito)
			continue;
		// Tavolo buono
		disponibilita[index] = 1;
		strcat(buffer, itoa(numero));
		strcat(buffer, ") T");
		strcat(buffer, itoa(index));
		strcat(buffer, " ");
		strcat(buffer, tavoli[index].sala);
		strcat(buffer, " ");
		strcat(buffer, tavoli[index].descrizione);
		strcat(buffer, "\n");
		numero++;
	}
}

// Gestisce UNA richiesta da parte di UN client
void gestisciClient(int socketId) {
	char buffer[BUFFER_SIZE];

	// Ricevi il messaggio
	int ret;
	int lmsg = 0;
	ret = riceviLunghezza(socketId, &lmsg);
	if(ret == 0) {
		printf("Client disconnesso\n");
		fflush(stdout);
	}
	ret = ricevi(socketId, lmsg, buffer);
	if(ret == 0) {
		printf("Client disconnesso\n");
		fflush(stdout);
	}

	// Gestisce i tipi di comandi:
	//   - find, e, di conseguenza, dopo;
	//       - book, e termina il thread;
	//       - disconnessione, cancella il socketId da socket_client e termina il thread.
	char* token;
	token = strtok(buffer, " ");
	if(strcmp(token, "find")) { // Primo caso
		// Parsa la stringa e cerca i tavoli liberi
		token = strtok(NULL, " ");
		char cognome[64];
		strcpy(cognome, token);

		token = strtok(NULL, " ");
		int nPers;
		strcpy(nPers, token);

		token = strtok(NULL, " ");
		struct tm when;
		time_t dataora;
		dataora = ; // Devo parsarla in qualche modo

		char disponibilita[nTavoli];

		cercaDisponibilita(nPers, dataora, &buffer, &disponibilita);
retry:
		// Invia il buffer con le possibilità
		ret = invia(socketId, buffer);

		// Aspetta una book o una disconnessione
		ret = riceviLunghezza(socketId, &lmsg);
		if(ret == 0) {
			printf("Client disconnesso\n");
			fflush(stdout);
		}
		ret = ricevi(socketId, lmsg, buffer);
		if(ret == 0) {
			printf("Client disconnesso\n");
			fflush(stdout);
		}

		if(strcmp(token, "book")) { // Caso book
			token = strtok(NULL, " ");
			
			// Converto l'indice in tavolo
			int tavolo = 0;
			int v = atoi(token);
			for(tavolo = 0; tavolo <= nTavoli && !v; tavolo++){
				while(!disponibilita[tavolo])
					tavolo++;
				v--;
			}
			// Cerco le disponibilità attuali
			cercaDisponibilita(nPers, dataora, &buffer, &disponibilita);

			if(disponibilita[tavolo] == 0) {
				// Caso in cui non sia più disponibile l'opzione
				goto retry;
			}
			// Salvo la prenotazione
			struct prenotazione* p = malloc(sizeof(*p));
			strcpy(p->cognome, cognome);
			p->data_ora = dataora;
			p->prossima = NULL;

			// Inserisco in lista prenotazioni
			struct prenotazione* punta = &prenotazioni[tavolo];
			if(punta == NULL) {
				punta = p;
			}
			else {
				while(punta->prossima != NULL && punta->prossima->data_ora < p->data_ora) {
					punta = punta->prossima;
				}
				p->prossima = punta->prossima;
				punta->prossima = p;
			}

			printf("Client %d ha effettuato una prenotazione\n", socketId);
			fflush(stdout);

			strcpy(buffer, "PRENOTAZIONE EFFETTUATA");
			invia(socketId, buffer);
		}
	}
	else if(strcmp(token, "book")) {
		// Errore, non sono state fatte precedenti find
		strcpy(buffer, "Errore, non sono state fatte precedenti find");
		invia(socketId, buffer);
	}
	else {
		// Errore, comando non riconosciuto
		printf("Errore comando Client!\n");
		fflush(stdout);
	}
	return;
}

// Gestisce UNA richiesta da parte di UN table device
void gestisciTd(int socketId) {
	char buffer[BUFFER_SIZE];
	
	// Trovo il tavolo collegato al TD
	int tavolo;
	for(tavolo = 0; tavolo < nTavoli; tavolo++)
		if(socketId == tavolo_td[tavolo])
			break;

	// Ricevi il messaggio
	int ret;
	int lmsg = 0;
	ret = riceviLunghezza(socketId, &lmsg);
	if(ret == 0) {
		printf("Client disconnesso\n");
		fflush(stdout);
	}
	ret = ricevi(socketId, lmsg, buffer);
	if(ret == 0) {
		printf("Client disconnesso\n");
		fflush(stdout);
	}

	// Gestisce i tipi di comandi:
	//   - menu;
	//   - comanda;
	//   - conto;
	char* token;
	token = strtok(buffer, " ");
	if(strcmp(token, "menu")) { // Primo caso
		// Invio il menu
		strcpy(buffer, menu);
		invia(socketId, buffer);
	}
	else if(strcmp(token, "comanda")) { // Secondo caso
		// Parso la comanda ed inserisco
		struct comanda* punta = &comande[tavolo];
		while(punta != NULL && punta->prossima != NULL)
			punta = punta->prossima;

		struct comanda* com = malloc(sizeof(com));
		punta->prossima = com;

		token = strtok(NULL, " ");
		while(token != NULL) {
			
			token = strtok(NULL, "-");
			atoi(token);
			token = strtok(NULL, " ");
		}
		com->timestamp = time(NULL);
		com->kd = socketId;
		com->nComanda = numeroComanda++;
		com->prossima = NULL;
		com->stato = in_attesa;
	}
	else if(strcmp(token, "conto")) { // Terzo caso
		// Scorro l'array comande ed invio
		struct comanda* punta = &comande[tavolo];
		int totale = 0;
		while(punta != NULL){
			for(int indice = 0; indice < nPiatti; indice++){
				if(punta->quantita[indice] == 0) 
					continue;

				strcat(buffer, menu[indice]->codice);
				strcat(buffer, " ");
				strcat(buffer, itoa(punta->quantita[indice]));
				strcat(buffer, " ");
				strcat(buffer, itoa(punta->quantita[indice]*menu[indice]->prezzo));
				strcat(buffer, "\n");
				totale += punta->quantita[indice] * menu[indice]->prezzo;
			}
			punta = punta->prossima;
		}
		strcat(buffer, "Totale: ");
		strcat(buffer,totale);
		strcat(buffer, "\n");
	}
	else {
		// Errore, comando non riconosciuto
		printf("Errore comando Table Device!\n");
		fflush(stdout);
	}
}

// Gestisce UNA richiesta da parte di UN kitchen device
void gestisciKd(int socketId) {
	char buffer[BUFFER_SIZE];

	// Ricevi il messaggio
	int ret;
	int lmsg = 0;
	ret = riceviLunghezza(socketId, &lmsg);
	if(ret == 0) {
		printf("Client disconnesso\n");
		fflush(stdout);
	}
	ret = ricevi(socketId, lmsg, buffer);
	if(ret == 0) {
		printf("Client disconnesso\n");
		fflush(stdout);
	}

	// Gestisce i tipi di comandi:
	//   - take, funzionante solo nel caso siano presenti delle comande in_attesa;
	//   - show, comande in_preparazione presso il kd;
	//   - ready <comanda>, solo se presente tra quelle in_preparazione del kd.
	char* token;
	token = strtok(buffer, " ");
	if(strcmp(token, "take")) { // Primo caso
		// Scorro l'array comande ed invio
		
	}
	else if(strcmp(token, "show")) { // Secondo caso
		// Scorro l'array comande ed invio

	}
	else if(strcmp(token, "ready")) { // Terzo caso
		// Parso il comando e notifico il td

	}
	else {
		// Errore, comando non riconosciuto
		printf("Errore comando Kitchen Device!\n");
		fflush(stdout);
	}
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
