/* Codice del server */
/* ----------------- */

#include "strutture.h"	// Ci sono anche gli altri include
#include "funzioni.h"	// Firme delle funzioni

int main(int argc, char* argv[]){
	/* --- Strutture --- */
	// Le strutture vengono definite in "strutture.h" per
	// permettere l'utilizzo della libreria "funzioni.c"

	// Le inizializzo
	int indice;
	for(indice = 0; indice < nMaxClient; indice++)
		socket_client[indice] = -1;
	for(indice = 0; indice < nMaxTd; indice++)
		socket_td[indice] = -1;
	for(indice = 0; indice < nMaxKd; indice++)
		socket_kd[indice] = -1;

	pthread_mutex_init(&tavoli_lock, NULL);
	pthread_mutex_init(&prenotazioni_lock, NULL);
	pthread_mutex_init(&comande_lock, NULL);
	pthread_mutex_init(&listaThread_lock, NULL);
	pthread_mutex_init(&socket_lock, NULL);

	numeroComanda = 0;

	// Carico dai file "tavoli.txt" e "menu.txt"
	caricaTavoli();
	caricaMenu();
	
	printf(tavoli[3].descrizione);
	fflush(stdout);

	/* --- Inizio ---*/

	// Stampo a video il "benvenuto" del server
	printf(BENVENUTO_SERVER);
	fflush(stdout);

	struct sockaddr_in my_addr, cl_addr;
	int ret, newfd, listener, addrlen, i;
	char buffer[BUFFER_SIZE];
	char bufferOut[BUFFER_SIZE];
	int portNumber = atoi(argv[1]);

	// Set di descrittori da monitorare
	fd_set master;

	// Set dei descrittori pronti
	fd_set read_fds;

	// Descrittore max
	int fdmax;

	/* Creazione indirizzo del server */
	listener = socket(AF_INET, SOCK_STREAM, 0);

	/* Creazione indirizzo di bind */
	memset(&my_addr, 0, sizeof(my_addr));
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(portNumber);
	my_addr.sin_addr.s_addr = INADDR_ANY;

	/* Aggancio */
	ret = bind(listener, (struct sockaddr*)&my_addr, sizeof(my_addr));
	if(ret < 0){
		perror("Bind non riuscita\n");
		exit(0);
	}

	/* Apro la coda */
	listen(listener, 10);

	/* Reset dei descrittori */
	FD_ZERO(&master);
	FD_ZERO(&read_fds);

	// Aggiungo il socket di ascolto 'listener' e 'stdin' (0) ai socket monitorati
	FD_SET(listener, &master);
	FD_SET(0, &master);

	// Tengo traccia del nuovo fdmax
	fdmax = listener;

	// Ciclo principale
	for(;;) {
		printf("test: for sempre\n");
		fflush(stdout);
		//Imposto il set di socket da monitorare in lettura per la select()
		read_fds = master;

		// Mi blocco (potenzialmente) in attesa di descrittori pronti
		ret = select(fdmax+1, &read_fds, NULL, NULL, NULL);
		if(ret < 0) {
			perror("Errore nella select!");
			exit(-1);
		}

		// Scorro ogni descrittore 'i'
		for(i = 0; i <= fdmax; i++) {
			// Il descrittore 'i' è pronto se la select lo ha lasciato nel set "read_fds"
			if(FD_ISSET(i, &read_fds)) {
				// Ci sono tre casi:
				//   - ho ricevuto un comando (stdin);
				//   - ho ricevuto una nuova richiesta di connessione;
				//   - devo gestire una richiesta da un socket già connesso.
				if(i == 0) { // Primo caso: comando da stdin
					char *serverCommand;
					scanf(" %[^\n]", buffer); // Lo inserisco nel buffer e poi lo analizzo
					serverCommand = strtok(buffer, " ");
					// Ci sono due casi:
					//   - stat, con i relativi sotto casi;
					//   - stop.
					if(strcmp(buffer, "stat") == 0) { // Primo caso: stat
						// Ci sono diversi casi
						//   - Nessun parametro, restituisce lo stato di tutte le comande giornaliere;
						//   - "T<nTavolo>"", restituisce tutte le comande relative al tavolo table relative al pasto in corso;
						//   - "a", restituisce tutte le comande in attesa;
						//   - "p", restituisce tutte le comande in preparazione;
						//   - "s", restituisce tutte le comande in servizio.

						// Prendo il secondo termine del comando
						serverCommand = strtok(NULL, " ");

						if(serverCommand == NULL) {
							// Non necessario, quindi:
							printf("Comando 'stat' senza parametri inesistente!\n");
							fflush(stdout);
						}
						else if(strcmp(buffer, "T") == 0) { // Chiede lo stato di un tavolo
							// Cerco il numero del tavolo e stampo l'esito
							serverCommand = strtok(NULL, "T");
							int tavolo = atoi(serverCommand);

							if(tavolo > nMaxTd || tavolo == 0) {
								printf("Tavolo inesistente!\n");
								fflush(stdout);
								break;
							}
							elencoComandeTavolo(bufferOut, tavolo);

							printf(bufferOut);
							fflush(stdout);
						}
						else if(strcmp(buffer, "a") == 0) { // Chiedo le comande in attesa
							// Scorro tutto l'array di liste, nel caso sia in attesa, la aggiungo al buffer
							elencoComande(bufferOut, in_attesa);

							printf(bufferOut);
							fflush(stdout);
						}
						else if(strcmp(buffer, "p") == 0) { // Chiedo le comande in preparazione
							// Scorro tutto l'array di liste, nel caso sia in preparazione, la aggiungo al buffer
							elencoComande(bufferOut, in_preparazione);

							printf(bufferOut);
							fflush(stdout);
						}
						else if(strcmp(buffer, "s") == 0) { // Chiedo le comande in servizio
							// Scorro tutto l'array di liste, nel caso sia in servizio, la aggiungo al buffer
							elencoComande(bufferOut, in_servizio);

							printf(bufferOut);
							fflush(stdout);
						}
						else { // Comando non riconosciuto
							printf("Comando 'stat' con elementi non riconosciuti!\n");
							fflush(stdout);
						}
					}
					else if(strcmp(buffer, "stop") == 0) { // Secondo caso: stop
						// Se posso stopparmi, mando una notifica a tutti i dispositivi connessi e mi interrompo.
						// Nel caso io non possa fermarmi (ci sono delle comande in attesa o preparazione), lo comunico e non faccio niente.
						if( !comandeInSospeso() ) { // Mi posso fermare
							int j;
							// Comunico l'esecuzione del comando
							printf("Comunico a tutti la chiusura del server e lo termino.\n");
							fflush(stdout);

							// Invio ad ogni dispositivo connesso il messaggio "STOP"
							strcpy(bufferOut,"STOP\0");
							for(j = 1; j < fdmax; j++) {
								if( j == listener) continue; // Salta il listener
								ret = invia(j, bufferOut);
								if(ret < 0){
									perror("Errore: \n");
									exit(1);
								}
								close(j);
								FD_CLR(j, &master);
							}
							close(listener);

							// Aspetto la fine di TUTTI i thread
							struct lis_thread *lt;
							lt = listaThread;
							while(lt != NULL) {
								pthread_join(*(lt->t),NULL);
								lt = lt->prossimo;
							}

							deallocaStrutture();

							// Termino il server con esito positivo
							return 1;
						}
						else { // Non posso fermarmi
							printf("Sono presenti delle comande in preparazione e in attesa!\nNon è possibile fermare il server.\n");
							fflush(stdout);
						}
					}
					else { // Comando non riconosciuto
						printf("Comando non riconosciuto!\n");
						fflush(stdout);
					}
				}
				else if(i == listener) { // Secondo caso: nuova connessione
					// Calcolo la lunghezza dell'indirizzo del client
					addrlen = sizeof(cl_addr);

					// Accetto la connessione e creo il socket connesso ('newfd')
					newfd = accept(listener, (struct sockaddr *)&cl_addr, (socklen_t*)&addrlen);

					// Aggiungo il socket connesso al set dei descrittori monitorati
					FD_SET(newfd, &master); 

					// Aggiorno l'ID del massimo descrittore
					if(newfd > fdmax) { 
						fdmax = newfd;  
					}
				}
				else { // Terzo caso: richiesta da un socket già connesso
					// Cerco il socket nelle mie strutture, se non c'è mi sta per forza comunicando cosa è: tramite un char (1 byte);
					// c = client, t = table device, k = kitchen device.
					// Altrimenti, se l'ho trovato, so cosa è e lo gestisco mediante un thread, potrebbe essere una disconnessione.
					int tipo = -1; // 0 = client; 1 = table device; 2 = kitchen device.
					int j;
					for(j = 0; j <= nMaxClient+nMaxTd+nMaxKd; j++) {
						// Mutua esclusione
						if (socket_client[j%nMaxClient] == i){
							tipo = 0;
							break;
						}
						if (socket_td[j%nMaxTd] == i){
							tipo = 1;
							break;
						}
						if (socket_kd[j%nMaxKd] == i){
							tipo = 2;
							break;
						}
					}
					
					struct lis_thread *p;
					struct lis_thread *inserisciThread;
					switch(tipo) {
					case -1: // Si sta presentando
						ret = ricevi(i, 1, buffer);
						ret = inserisci(i, buffer[0]);
						if(ret < 0) {
							printf("Presentazione non riuscita: come primo messaggio non è arrivato il tipo.\n");
							fflush(stdout);
						}
						break;
					case 0: // Client che vuole utilizzare servizi
						// Creo un nuovo elemento della lista di thread e lo alloco
						p = malloc(sizeof(struct lis_thread));
						p->t = malloc(sizeof(pthread_t));
						// Creo il thread
						(void) pthread_create(p->t, NULL, gestisciClient, (void*)&i);
						// Creo un puntatore per inserirlo in lista
						inserisciThread = listaThread;
						while(inserisciThread->prossimo != NULL)
							inserisciThread = inserisciThread->prossimo;
						// Lo inserisco
						inserisciThread->prossimo = p;
						break;
					case 1: // Table device che vuole utilizzare servizi
						// Creo un nuovo elemento della lista di thread e lo alloco
						p = malloc(sizeof(struct lis_thread));
						p->t = malloc(sizeof(pthread_t));
						// Creo il thread
						(void) pthread_create(p->t, NULL, gestisciTd, (void*)&i);
						// Creo un puntatore per inserirlo in lista
						inserisciThread = listaThread;
						while(inserisciThread->prossimo != NULL)
							inserisciThread = inserisciThread->prossimo;
						// Lo inserisco
						inserisciThread->prossimo = p;
						break;
					case 2: // Kitchen device che vuole utilizzare servizi
						// Creo un nuovo elemento della lista di thread e lo alloco
						p = malloc(sizeof(struct lis_thread));
						p->t = malloc(sizeof(pthread_t));
						// Creo il thread
						(void) pthread_create(p->t, NULL, gestisciKd, (void*)&i);
						// Creo un puntatore per inserirlo in lista
						inserisciThread = listaThread;
						while(inserisciThread->prossimo != NULL)
							inserisciThread = inserisciThread->prossimo;
						// Lo inserisco
						inserisciThread->prossimo = p;
						break;
					default:
						perror("Errore nell'identificare il socket\n");
						exit(-1);
						break;
					}
				}
			}
		}
	}
}