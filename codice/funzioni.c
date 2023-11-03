/* Contiene le funzioni necessarie al Server */
/* ----------------------------------------- */

#include "strutture.h"
#include "funzioni.h"

// Carica i tavoli dal file e li mette nell'array
void caricaTavoli() {
	FILE *f;
	char buffer[BUFFER_SIZE];
	char* serverCommand;
	int i;
	f = fopen("../txts/tavoli.txt","r");
	
	if (f == NULL){
       printf("Errore! Apertura file tavoli.txt non riuscita\n");
       exit(-1);
	}

	for(i = 0; i < nTavoli; i++){
		fgets(buffer, sizeof(buffer), f);
		serverCommand = strtok(buffer, " ");
		tavoli[i].nPosti = atoi(serverCommand);
		serverCommand = strtok(NULL, " ");
		strcpy(tavoli[i].sala, serverCommand);
		serverCommand = strtok(NULL, " ");
		strcpy(tavoli[i].descrizione, serverCommand);

		tavoli[i].numero = i+1;
	}

	fclose(f);
}

// Carica il menu dal file e lo mette nell'array
void caricaMenu() {
	FILE *f;
	char buffer[BUFFER_SIZE];
	char *serverCommand;
	int i;
	f = fopen("../txts/menu.txt","r");
	
	if (f == NULL){
       printf("Errore! Apertura file menu.txt non riuscita\n");
       exit(-1);
	}

	for(i = 0; i < nPiatti; i++) {
		fgets(buffer, sizeof(buffer), f);
		struct piatto* p = malloc(sizeof(*p));
		serverCommand = strtok(buffer, "€-");
		strcpy(p->codice, serverCommand);
		serverCommand = strtok(NULL, "€-");
		strcpy(p->nome, serverCommand);
		serverCommand = strtok(NULL, "€-");
		p->prezzo = (int)*serverCommand;
		menu[i] = p;
	}

	fclose(f);
}

// Ritorna 1 nel caso ci siano attualmente delle comande
// "in_preparazione" o "in_servizio"; 0 altrimenti
int comandeInSospeso() {
	int i;
	pthread_mutex_lock(&comande_lock);
	for (i = 0; i < nTavoli; i++) {
		struct comanda *c = comande[i];
		while(c->prossima != NULL) {
			if(c->stato == in_attesa || c->stato == in_preparazione)
				return 1;
		}
	}
	pthread_mutex_unlock(&comande_lock);
	return 0;
}

// Invia al socket in input il messaggio dentro buffer
int invia(int j, char* buffer) {
	int len = htons(strlen(buffer));
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
int ricevi(int j, int lunghezza, char* buffer) {
	int ret;
	ret = recv(j, (void*)buffer, lunghezza, 0);
	return ret;
}

// Prende uno stato_comanda e inserisce dentro il buffer tutte le informazioni
// delle comande in quello stato di qualunque tavolino
void elencoComande(char* buffer, enum stato_comanda stato) {
	char numeroString[BUFFER_SIZE];
	int i, j;
	pthread_mutex_lock(&comande_lock);
	for(i = 0; i < nTavoli; i++) {
		struct comanda *c;
		c = comande[i];
		while(c != NULL) {
			strcat(buffer, "com");
			sprintf(numeroString, "%d", c->nComanda);
			strcat(buffer, numeroString);
			strcat(buffer, " T");
			sprintf(numeroString, "%d", i);
			strcat(buffer, numeroString);
			strcat(buffer, "\n");
			for(j = 0; j < nPiatti; j++) {
				if(c->quantita[j] != 0) {
					strcat(buffer, menu[j]->codice);
					strcat(buffer, " ");
					sprintf(numeroString, "%d", c->quantita[j]);
					strcat(buffer, numeroString);
					strcat(buffer, "\n");
				}
			}
			// Vado avanti
			c = c->prossima;
		}
	}
	pthread_mutex_unlock(&comande_lock);
}

// Prende un tavolo e inserisce dentro il buffer tutte le informazioni
// delle comande inerenti a quel tavolino
void elencoComandeTavolo(char* buffer, int tavolo) {
	char numeroString[BUFFER_SIZE];
	int i;
	pthread_mutex_lock(&comande_lock);
	struct comanda *c;
	c = comande[tavolo];
	while(c != NULL) {
		strcat(buffer, "com");
		sprintf(numeroString, "%d", c->nComanda);
		strcat(buffer, numeroString);
		strcat(buffer, " ");
		switch (c->stato) {
			case in_attesa:
				strcpy(numeroString,"In attesa");
				break;
			case in_preparazione:
				strcpy(numeroString,"In preparazione");
				break;
			case in_servizio:
				strcpy(numeroString,"In servizio");
				break;
			default:
				strcpy(numeroString,"Sconosciuto");
		}
		strcat(buffer, numeroString);
		strcat(buffer, "\n");
		for(i = 0; i < nPiatti; i++) {
			if(c->quantita[i] != 0) {
				strcat(buffer, menu[i]->codice);
				strcat(buffer, " ");
				sprintf(numeroString, "%d", c->quantita[i]);
				strcat(buffer, numeroString);
				strcat(buffer, "\n");
			}
		}
		// Vado avanti
		c = c->prossima;
	}
	pthread_mutex_unlock(&comande_lock);
}

// Inserisce in base alla lettera c, il socket id nell'array relativo
int inserisci(int i, char c) {
	int j = 0;
	int ret;
	pthread_mutex_lock(&socket_lock);
	switch (c)
	{
	case 'c': // Client
		for(; j < nMaxClient; j++){
			if(socket_client[j] != -1){
				socket_client[j] = i;
				ret = 0;
				break;
			}
		}
		break;
	case 'k': // Kitchen device
		for(; j < nMaxKd; j++){
			if(socket_kd[j] != -1){
				socket_kd[j] = i;
				ret = 1;
				break;
			}
		}
		break;
	case 't': // Table device
		for(; j < nMaxTd; j++){
			if(socket_td[j] != -1){
				socket_td[j] = i;
				ret = 2;
				break;
			}
		}
		break;
	
	default:
		ret = -1;
		break;
	}

	pthread_mutex_unlock(&socket_lock);
	return ret;
}

// Prende i parametri della find ed inserisce nel buffer le disponibilità
void cercaDisponibilita(int nPers, time_t dataora, char* buffer, char* disponibilita) {
	char numeroString[BUFFER_SIZE];
	int index;
	pthread_mutex_lock(&tavoli_lock);
	pthread_mutex_lock(&prenotazioni_lock);
	int numero = 0;
	for(index = 0; index < nTavoli; index++) {
		if(tavoli[index].nPosti < nPers){
			disponibilita[index] = 0;
			continue;
		}
		struct prenotazione* punta = prenotazioni[index];
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
		sprintf(numeroString, "%d", numero);
		strcat(buffer, numeroString);
		strcat(buffer, ") T");
		sprintf(numeroString, "%d", index);
		strcat(buffer, numeroString);
		strcat(buffer, " ");
		strcat(buffer, tavoli[index].sala);
		strcat(buffer, " ");
		strcat(buffer, tavoli[index].descrizione);
		strcat(buffer, "\n");
		numero++;
	}
    pthread_mutex_unlock(&tavoli_lock);
    pthread_mutex_unlock(&prenotazioni_lock);
}

// Gestisce UNA richiesta da parte di UN client
void *gestisciClient(void* i) {
	int* sId = (int*)i;
	int socketId = *sId;
	char buffer[BUFFER_SIZE];

	// Ricevi il messaggio
	int ret;
	int lmsg = 0;
	ret = riceviLunghezza(socketId, &lmsg);
	if(ret == 0) {
		printf("Client disconnesso\n");
		fflush(stdout);
		close(socketId);
		FD_CLR(socketId, &master);
		return NULL;
	}
	ret = ricevi(socketId, lmsg, buffer);
	if(ret == 0) {
		printf("Client disconnesso\n");
		fflush(stdout);
		close(socketId);
		FD_CLR(socketId, &master);
		return NULL;
	}

	// Gestisce i tipi di comandi:
	//   - find, e, di conseguenza, dopo;
	//       - book, e termina il thread;
	//       - disconnessione, cancella il socketId da socket_client e termina il thread.
	char* token;
	token = strtok(buffer, " ");
	if(strcmp(token, "find") == 0) { // Primo caso
		// Parsa la stringa e cerca i tavoli liberi
		struct tm tm_;
		char cognome[64];
		int nPers;
		time_t dataora;
		char disponibilita[nTavoli];
		
		token = strtok(NULL, " ");
		strcpy(cognome, buffer);

		token = strtok(NULL, " ");
		nPers = atoi(buffer);

		token = strtok(NULL, " ");

		if (strptime(buffer, "%Y-%m-%d %H", &tm_) != NULL) {
			dataora = mktime(&tm_);
		}
		else {
			printf("Data inserita non valida\n");
			fflush(stdout);
			return NULL;
		}

retry:
		cercaDisponibilita(nPers, dataora, buffer, disponibilita);
		// Invia il buffer con le possibilità
		ret = invia(socketId, buffer);

		// Aspetta una book o una disconnessione
		ret = riceviLunghezza(socketId, &lmsg);
		if(ret == 0) {
			printf("Client disconnesso\n");
			fflush(stdout);
			close(socketId);
			FD_CLR(socketId, &master);
			return NULL;
		}
		ret = ricevi(socketId, lmsg, buffer);
		if(ret == 0) {
			printf("Client disconnesso\n");
			fflush(stdout);
			close(socketId);
			FD_CLR(socketId, &master);
			return NULL;
		}

		token = strtok(buffer, " ");

		if(strcmp(token, "book") == 0) { // Caso book
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
			cercaDisponibilita(nPers, dataora, buffer, disponibilita);

			if(disponibilita[tavolo] == 0) {
				// Caso in cui non sia più disponibile l'opzione
				goto retry;
			}
			// Salvo la prenotazione
			struct prenotazione* p = malloc(sizeof(struct prenotazione));
			strcpy(p->cognome, cognome);
			p->data_ora = dataora;
			p->prossima = NULL;

			// Inserisco in lista prenotazioni
			pthread_mutex_lock(&prenotazioni_lock);
			struct prenotazione* punta = prenotazioni[tavolo];
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
			pthread_mutex_unlock(&prenotazioni_lock);

			printf("Client %d ha effettuato una prenotazione\n", socketId);
			fflush(stdout);

			strcpy(buffer, "PRENOTAZIONE EFFETTUATA\n");
			invia(socketId, buffer);
		}
	}
	else if(strcmp(token, "book") == 0) {
		// Errore, non sono state fatte precedenti find
		strcpy(buffer, "Errore, non sono state fatte precedenti find\n");
		invia(socketId, buffer);
	}
	else {
		// Errore, comando non riconosciuto
		printf("Errore comando Client!\n");
		fflush(stdout);
	}
	return NULL;
}

// Gestisce UNA richiesta da parte di UN table device
void *gestisciTd(void* i) {
	int* sId = (int*)i;
	int socketId = *sId;
	char buffer[BUFFER_SIZE];
	char numeroString[BUFFER_SIZE];
	
	// Trovo il tavolo collegato al TD
	int tavolo;
	for(tavolo = 0; tavolo < nTavoli; tavolo++)
		if(socketId == socket_td[tavolo])
			break;

	// Ricevi il messaggio
	int ret;
	int lmsg = 0;
	ret = riceviLunghezza(socketId, &lmsg);
	if(ret == 0) {
		printf("TD disconnesso\n");
		fflush(stdout);
		close(socketId);
		FD_CLR(socketId, &master);
		return NULL;
	}
	ret = ricevi(socketId, lmsg, buffer);
	if(ret == 0) {
		printf("TD disconnesso\n");
		fflush(stdout);
		close(socketId);
		FD_CLR(socketId, &master);
		return NULL;
	}

	// Gestisce i tipi di comandi:
	//   - menu;
	//   - comanda;
	//   - conto.
	char* token;
	token = strtok(buffer, " ");
	if(strcmp(token, "menu") == 0) { // Primo caso
		// Invio il menu
		strcpy(buffer, menu_text);
		invia(socketId, buffer);
	}
	else if(strcmp(token, "comanda") == 0) { // Secondo caso
		int i, indice;
		// Parso la comanda ed inserisco
		pthread_mutex_lock(&comande_lock);
		struct comanda* punta = comande[tavolo];
		
		struct comanda* com = malloc(sizeof(struct comanda));
		
		if(punta == NULL) {
			punta = com;
		}
		else {
			while(punta->prossima != NULL)
				punta = punta->prossima;
			punta->prossima = com;
		}
		com->prossima = NULL;

		token = strtok(NULL, " ");
		while(token != NULL) {
			for (i = 0; i < nPiatti; i++) {
				if(strcmp(token, menu[i]->codice) != 0)
					continue;
				token = strtok(NULL, "-");
				com->quantita[i] = atoi(token);
				token = strtok(NULL, " ");
			}
		}
		com->timestamp = time(NULL);
		com->kd = socketId;
		com->nComanda = numeroComanda++;
		com->stato = in_attesa;

		pthread_mutex_unlock(&comande_lock);

		// Notifico tutti i KD
		strcpy(buffer, "Nuova comanda!");
		pthread_mutex_lock(&socket_lock);
		for(indice = 0; indice < nMaxKd; indice++) {
			if(socket_kd[indice] != -1) {
				invia(socket_kd[indice], buffer);
			}
		}
		pthread_mutex_unlock(&socket_lock);
	}
	else if(strcmp(token, "conto") == 0) { // Terzo caso
		int indice;
		pthread_mutex_lock(&comande_lock);
		// Scorro l'array comande ed invio
		struct comanda* punta = comande[tavolo];
		int totale = 0;
		while(punta != NULL) {
			for(indice = 0; indice < nPiatti; indice++) {
				if(punta->quantita[indice] == 0) 
					continue;

				strcat(buffer, menu[indice]->codice);
				strcat(buffer, " ");
				sprintf(numeroString, "%d", punta->quantita[indice]);
				strcat(buffer, numeroString);
				strcat(buffer, " ");
				sprintf(numeroString, "%d", punta->quantita[indice]*menu[indice]->prezzo);
				strcat(buffer, numeroString);
				strcat(buffer, "\n");
				totale += punta->quantita[indice] * menu[indice]->prezzo;
			}
			struct comanda* puntaVecchia;
			puntaVecchia = punta;
			punta = punta->prossima;
			free(puntaVecchia);
		}
		comande[tavolo] = NULL;
		pthread_mutex_unlock(&comande_lock);
		strcat(buffer, "Totale: ");
		sprintf(numeroString, "%d", totale);
		strcat(buffer, numeroString);
		strcat(buffer, "\n");
		invia(socketId, buffer);
	}
	else {
		// Errore, comando non riconosciuto
		printf("Errore comando Table Device!\n");
		fflush(stdout);
	}
	return NULL;
}

// Gestisce UNA richiesta da parte di UN kitchen device
void *gestisciKd(void* i) {
	int* sId = (int*)i;
	int socketId = *sId;
	char buffer[BUFFER_SIZE];
	char numeroString[BUFFER_SIZE];

	// Ricevi il messaggio
	int ret;
	int lmsg = 0;
	ret = riceviLunghezza(socketId, &lmsg);
	if(ret == 0) {
		printf("KD disconnesso\n");
		fflush(stdout);
		close(socketId);
		FD_CLR(socketId, &master);
		return NULL;
	}
	ret = ricevi(socketId, lmsg, buffer);
	if(ret == 0) {
		printf("KD disconnesso\n");
		fflush(stdout);
		close(socketId);
		FD_CLR(socketId, &master);
		return NULL;
	}

	// Gestisce i tipi di comandi:
	//   - take, funzionante solo nel caso siano presenti delle comande in_attesa;
	//   - show, comande in_preparazione presso il kd;
	//   - ready <comanda>, solo se presente tra quelle in_preparazione del kd.
	char* token;
	token = strtok(buffer, " ");
	if(strcmp(token, "take") == 0) { // Primo caso
		int indice;
		pthread_mutex_lock(&comande_lock);

		// Scorro l'array comande ed invio
		struct comanda* com = NULL;
		int nTav = -1;
		for(indice = 0; indice < nTavoli; indice++) {
			struct comanda *punta = comande[indice];
			while(punta != NULL) {
				if((com == NULL || punta->timestamp < com->timestamp) && punta->stato == in_attesa) {
					com = punta;
					nTav = indice;
				}
				punta = punta->prossima;
			}
		}
		if(nTav == -1) {
			invia(socketId, "Non ci sono comande\n");
		}

		com->kd = socketId;
		com->stato = in_preparazione;

		strcpy(buffer, "com");
		sprintf(numeroString, "%d", com->nComanda);
		strcat(buffer, numeroString);
		strcat(buffer, "\t");
		strcat(buffer, "T");
		sprintf(numeroString, "%d", nTav);
		strcat(buffer, numeroString);
		strcat(buffer, "\n");
		for(indice = 0; indice < nPiatti; indice++) {
			if(com->quantita[indice] != 0) {
				strcat(buffer, menu[indice]->codice);
				strcat(buffer, "\t");
				sprintf(numeroString, "%d", com->quantita[indice]);
				strcat(buffer, numeroString);
				strcat(buffer, "\n");
			}
		}
		invia(socketId, buffer);
		pthread_mutex_unlock(&comande_lock);
	}
	else if(strcmp(token, "show") == 0) { // Secondo caso
		int indice, indice2;
		// Scorro l'array comande ed invio
		strcpy(buffer, "");
		pthread_mutex_lock(&comande_lock);
		for(indice = 0; indice < nTavoli; indice++) {
			struct comanda *punta = comande[indice];
			while(punta != NULL){
				if(punta->kd == socketId && punta->stato == in_preparazione) {
					strcat(buffer, "com");
					sprintf(numeroString, "%d", punta->nComanda);
					strcat(buffer, numeroString);
					strcat(buffer, "\t");
					strcat(buffer, "T");
					sprintf(numeroString, "%d", indice);
					strcat(buffer, numeroString);
					strcat(buffer, "\n");
					for(indice2 = 0; indice2 < nPiatti; indice2++) {
						if(punta->quantita[indice2] != 0) {
							strcat(buffer, menu[indice2]->codice);
							strcat(buffer, "\t");
							sprintf(numeroString, "%d", punta->quantita[indice2]);
							strcat(buffer, numeroString);
							strcat(buffer, "\n");
						}
					}
				}
				punta = punta->prossima;
			}
		}
		pthread_mutex_unlock(&comande_lock);
		invia(socketId, buffer);
	}
	else if(strcmp(token, "ready") == 0) { // Terzo caso
		// Parso il comando e notifico il td
		int nCom, nTav, indice;
		token = strtok(NULL, " com-");
		nCom = atoi(token);
		token = strtok(NULL, "T");
		nTav = atoi(token);


		pthread_mutex_lock(&comande_lock);
		struct comanda* punta = comande[nTav];
		for(indice = 0; indice < nCom; indice++)
			punta = punta->prossima;

		punta->stato = in_servizio;
		invia(socketId, "COMANDA IN SERVIZIO\n");
		invia(socket_td[nTav], "ORDINAZIONE IN ARRIVO\n");
		pthread_mutex_unlock(&comande_lock);
	}
	else {
		// Errore, comando non riconosciuto
		printf("Errore comando Kitchen Device!\n");
		fflush(stdout);
	}
	return NULL;
}

// Dealloca tutte le strutture
void deallocaStrutture() {
	int i;
	// Comande
	for(i = 0; i < nTavoli; i++) {
		struct comanda *c = comande[i];
		while(c != NULL) {
			struct comanda *c2 = c->prossima;
			free(c);
			c = c2;
		}
	}		
	// Thread
	struct lis_thread *lt = listaThread;
	while(lt != NULL) {
		struct lis_thread *lt2 = lt->prossimo;
		free(lt);
		lt = lt2;
	}		
	// Prenotazioni
	for(i = 0; i < nTavoli; i++) {
		struct prenotazione *p = prenotazioni[i];
		while(p != NULL) {
			struct prenotazione *p2 = p->prossima;
			free(p);
			p = p2;
		}
	}
}

// Trova il massimo tra 3 interi
int max(int v1, int v2, int v3) {
    int max_value = v1;

    if (v2 > max_value) {
        max_value = v2;
    }

    if (v3 > max_value) {
        max_value = v3;
    }

    return max_value;
}