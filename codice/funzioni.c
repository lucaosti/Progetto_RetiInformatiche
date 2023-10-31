/* Contiene le funzioni necessarie al Server */
/* ----------------------------------------- */

#include "strutture.h"
#include "funzioni.h"

// Carica i tavoli dal file e li mette nell'array
void caricaTavoli() {
	FILE *f;
	char *buf;
	char str[1024];
   	int i;
	f = fopen("tavoli.txt","r");
	
	if (f == NULL){
       printf("Errore! Apertura file tavoli.txt non riuscita");
       exit(-1);
   	}

	for(i = 0; i < nTavoli; i++){
   		fgets(str, sizeof(str), f);
   		buf = strtok(str, " ");
   		tavoli[i].nPosti = atoi(buf);
   		buf = strtok(NULL, " ");
   		strcpy(tavoli[i].sala, buf);
   		buf = strtok(NULL, " ");
   		strcpy(tavoli[i].descrizione, buf);

		tavoli[i].numero = i+1;
   	}

	fclose(f);
}

// Carica il menu dal file e lo mette nell'array
void caricaMenu() {
	FILE *f;
	char str[1024];
	char *buf;
	int i;
	f = fopen("menu.txt","r");
	
	if (f == NULL){
       printf("Errore! Apertura file menu.txt non riuscita");
       exit(-1);
   	}

	for(i = 0; i < nPiatti; i++) {
		fgets(str, sizeof(str), f);
		struct piatto* p = malloc(sizeof(*p));
		buf = strtok(str, "€-");
		strcpy(p->codice, buf);
		buf = strtok(NULL, "€-");
		strcpy(p->nome, buf);
		buf = strtok(NULL, "€-");
		p->prezzo = (int)*buf;
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
		struct comanda *c = &comande[i];
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
	int i, j;
	pthread_mutex_lock(&comande_lock);
	for(i = 0; i < nTavoli; i++) {
		struct comanda *c;
		c = &comande[i];
		while(c != NULL) {
			strcat(buffer, "com");
			strcat(buffer, c->nComanda);
			strcat(buffer, " T");
			strcat(buffer, i);
			strcat(buffer, "\n");
			for(j = 0; j < nPiatti; j++) {
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
	pthread_mutex_unlock(&comande_lock);
}

// Prende un tavolo e inserisce dentro il buffer tutte le informazioni
// delle comande inerenti a quel tavolino
void elencoComandeTavolo(char* buffer, int tavolo) {
	int i;
	pthread_mutex_lock(&comande_lock);
	struct comanda *c;
	c = &comande[tavolo];
	while(c != NULL) {
		strcat(buffer, "com");
		strcat(buffer, c->nComanda);
		strcat(buffer, " ");
		strcat(buffer, c->stato);
		strcat(buffer, "\n");
		for(i = 0; i < nPiatti; i++) {
			if(c->quantita[i] != 0) {
				strcat(buffer, menu[i]->codice);
				strcat(buffer, " ");
				strcat(buffer, c->quantita[i]);
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
	phtread_mutex_lock(socket_lock);
	switch (c)
	{
	case 'c': // Client
		for(; j < nMaxClient; j++){
			if(socket_client[j] != -1){
				socket_client[j] = i;
				break;
			}
		}
		break;
	case 'k': // Kitchen device
		for(; j < nMaxKd; j++){
			if(socket_kd[j] != -1){
				socket_kd[j] = i;
				break;
			}
		}
		break;
	case 't': // Table device
		for(; j < nMaxTd; j++){
			if(socket_td[j] != -1){
				socket_td[j] = i;
				break;
			}
		}
		break;
	
	default:
		phtread_mutex_unlock(socket_lock);
		return -1;
	}

	phtread_mutex_unlock(socket_lock);
	return 1;
}

// Prende i parametri della find ed inserisce nel buffer le disponibilità
void cercaDisponibilita(int nPers, time_t dataora, char* buffer, char* disponibilita) {
	int index;
	pthread_mutex_lock(&tavoli_lock);
	pthread_mutex_lock(&prenotazioni_lock);
	int numero = 0;
	for(index = 0; index < nTavoli; index++) {
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
    pthread_mutex_unlock(&tavoli_lock);
    pthread_mutex_unlock(&prenotazioni_lock);
}

// Gestisce UNA richiesta da parte di UN client
void *gestisciClient(int socketId) {
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
		time_t dataora;

		struct tm tm;
		if ( strptime(token, "%Y-%m-%d %H", &tm) != NULL ) {
			dataora = mktime(&tm);
		}
		else {
			printf("Data inserita non valida\n");
			fflush(stdout);
			return;
		}

		char disponibilita[nTavoli];

retry:
		cercaDisponibilita(nPers, dataora, &buffer, &disponibilita);
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
			pthread_mutex_lock(&prenotazioni_lock);
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
			pthread_mutex_unlock(&prenotazioni_lock);

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
void *gestisciTd(int socketId) {
	char buffer[BUFFER_SIZE];
	
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
		int i, indice;
		// Parso la comanda ed inserisco
		pthread_mutex_lock(&comande_lock);
		struct comanda* punta = &comande[tavolo];
		
		struct comanda* com = malloc(sizeof(com));
		
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
		phtread_mutex_lock(socket_lock);
		for(indice = 0; indice < nMaxKd; indice++) {
			if(socket_kd[indice] != -1) {
				invia(socket_kd[indice], buffer);
			}
		}
		phtread_mutex_unlock(socket_lock);
	}
	else if(strcmp(token, "conto")) { // Terzo caso
		int indice;
		pthread_mutex_lock(&comande_lock);
		// Scorro l'array comande ed invio
		struct comanda* punta = &comande[tavolo];
		int totale = 0;
		while(punta != NULL) {
			for(indice = 0; indice < nPiatti; indice++) {
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
			struct comanda* puntaVecchia;
			puntaVecchia = punta;
			punta = punta->prossima;
			free(puntaVecchia);
		}
		pthread_mutex_unlock(&comande_lock);
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
void *gestisciKd(int socketId) {
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
			invia(socketId, "Non ci sono comande");
		}

		com->kd = socketId;
		com->stato = in_preparazione;

		strcpy(buffer, "com");
		strcat(buffer, itoa(com->nComanda));
		strcat(buffer, "\t");
		strcat(buffer, "T");
		strcat(buffer, itoa(nTav));
		strcat(buffer, "\n");
		for(indice = 0; indice < nPiatti; indice++) {
			if(com->quantita[indice] != 0) {
				strcat(buffer, menu[indice]->codice);
				strcat(buffer, "\t");
				strcat(buffer, itoa(com->quantita[indice]));
				strcat(buffer, "\n");
			}
		}
		invia(socketId, buffer);
		pthread_mutex_unlock(&comande_lock);
	}
	else if(strcmp(token, "show")) { // Secondo caso
		int indice, indice2;
		// Scorro l'array comande ed invio
		strcpy(buffer, "");
		pthread_mutex_lock(&comande_lock);
		for(indice = 0; indice < nTavoli; indice++) {
			struct comanda *punta = comande[indice];
			while(punta != NULL){
				if(punta->kd == socketId && punta->stato == in_preparazione) {
					strcat(buffer, "com");
					strcat(buffer, itoa(punta->nComanda));
					strcat(buffer, "\t");
					strcat(buffer, "T");
					strcat(buffer, itoa(indice));
					strcat(buffer, "\n");
					for(indice2 = 0; indice2 < nPiatti; indice2++) {
						if(punta->quantita[indice2] != 0) {
							strcat(buffer, menu[indice2]->codice);
							strcat(buffer, "\t");
							strcat(buffer, itoa(punta->quantita[indice2]));
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
	else if(strcmp(token, "ready")) { // Terzo caso
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
		invia(socketId, "COMANDA IN SERVIZIO");
		invia(socket_td[nTav], "ORDINAZIONE IN ARRIVO");
		pthread_mutex_unlock(&comande_lock);
	}
	else {
		// Errore, comando non riconosciuto
		printf("Errore comando Kitchen Device!\n");
		fflush(stdout);
	}
}

// Dealloca tutte le strutture
void deallocaStrutture() {
	int i;
	// Comande
	for(i = 0; i < nTavoli; i++) {
		struct comanda *c = &comande[i];
		while(c != NULL) {
			struct comanda *c2 = &c->prossima;
			free(c);
			c = c2;
		}
	}		
	// Thread
	struct lis_thread *lt = &listaThread;
	while(lt != NULL) {
		struct lis_thread *lt2 = &lt->prossimo;
		free(lt);
		lt = lt2;
	}		
	// Prenotazioni
	for(i = 0; i < nTavoli; i++) {
		struct prenotazione *p = &prenotazioni[i];
		while(p != NULL) {
			struct prenotazione *p2 = &p->prossima;
			free(p);
			p = p2;
		}
	}
}
