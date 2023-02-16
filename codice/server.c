/* Codice del server */
/* ----------------- */

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

int main(int argc, char* argv[]){
	/* --- Strutture --- */

	// Array per i Socket
	int socket_client[nMaxClient];
	int socket_td[nMaxTd];
	int socket_kd[nMaxKd];
	// Strutture
	struct tavolo tavoli[nTavoli];
	struct prenotazione prenotazioni[nTavoli];
	struct piatto piatti[nPiatti];
	struct comanda comande[nTavoli];
	struct lis_thread listaThread;
	
	// Carico dai file "tavoli.txt" e "menu.txt"
	caricaTavoli();
	caricaMenu();

	/* --- Inizio ---*/

	// Stampo a video il "benvenuto" del server
	printf(BENVENUTO_SERVER);  
	fflush(stdout);

	struct sockaddr_in my_addr, cl_addr;
	int ret, newfd, listener, addrlen, i, len;
	char buffer[BUFFER_SIZE];
	int portNumber = atoi(argv[1]);

	// Set di descrittori da monitorare
	fd_set master;

	// Set dei descrittori pronti
	fd_set read_fds;

	// Descrittore max
	int fdmax;

	// Creo il Socket listener
	listener = socket(AF_INET, SOCK_STREAM, 0);

	// Creazione indirizzo di bind
	memset(&my_addr, 0, sizeof(my_addr));
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(portNumber);
	my_addr.sin_addr.s_addr = INADDR_ANY;

	/* Aggancio */
	ret = bind(listener, (struct sockaddr*)&my_addr, sizeof(my_addr));
	if( ret<0){
		perror("Bind non riuscita\n");
		exit(0);
	}

	/* Apro la coda */
	listen(listener, 10);

	// Reset dei descrittori
	FD_ZERO(&master);
	FD_ZERO(&read_fds);

	// Aggiungo il socket di ascolto 'listener' e 'stdin' (0) ai socket monitorati
	FD_SET(listener, &master);
	FD_SET(0, &master);

	// Tengo traccia del nuovo fdmax
	fdmax = listener;

	// Ciclo principale
	for(;;) {
		//Imposto il set di socket da monitorare in lettura per la select()
		read_fds = master;

		// Mi blocco (potenzialmente) in attesa di descrittori pronti
		ret = select(fdmax+1, &read_fds, NULL, NULL, NULL);
		if(ret < 0) {
			perror("Errore nella select:");
			exit(-1);
		}

		// Scorro ogni descrittore 'i'
		for(i=0; i<=fdmax; i++) {
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
					if(strcmp(serverCommand, "stat") == 0) { // Primo caso: stat
						// Ci sono diversi casi
						//   - Nessun parametro, restituisce lo stato di tutte le comande giornaliere;
						//   - "T<nTavolo>"", restituisce tutte le comande relative al tavolo table relative al pasto in corso;
						//   - "a", restituisce tutte le comande in attesa
						//   - "p", restituisce tutte le comande in preparazione
						//   - "s", restituisce tutte le comande in servizio

						// Prendo il secondo termine del comando
						serverCommand = strtok(NULL, " ");

						if(serverCommand == NULL) { // da capire se farlo o meno
							// Non necessario, quindi:
							printf("Comando 'stat' senza parametri inesistente!\n");
							fflush(stdout);
						}
						if(strcmp(serverCommand[0], "T") == 0) { // Chiede lo stato di un tavolo
							// Cerco il numero del tavolo e stampo l'esito
							serverCommand = strtok(NULL, "T");
							int tavolo = atoi(serverCommand);
							
							elencoComandeTavolo(buffer, tavolo);

							printf(buffer);
							fflush(stdout);
						}
						else if(strcmp(serverCommand, "a") == 0) { // Chiedo le comande in attesa
							// Scorro tutto l'array di liste, nel caso sia in attesa, la aggiungo al buffer
							elencoComande(buffer, in_attesa);

							printf(buffer);
							fflush(stdout);
						}
						else if(strcmp(serverCommand, "p") == 0) { // Chiedo le comande in preparazione
							// Scorro tutto l'array di liste, nel caso sia in preparazione, la aggiungo al buffer
							elencoComande(buffer, in_preparazione);

							printf(buffer);
							fflush(stdout);
						}
						else if(strcmp(serverCommand, "s") == 0) { // Chiedo le comande in servizio
							// Scorro tutto l'array di liste, nel caso sia in servizio, la aggiungo al buffer
							elencoComande(buffer, in_servizio);

							printf(buffer);
							fflush(stdout);
						}
						else { // Comando non riconosciuto
							printf("Comando 'stat' con elementi non riconosciuti!\n");
							fflush(stdout);
						}
					}
					else if(strcmp(serverCommand, "stop") == 0) { // Secondo caso: stop
						// Se posso stopparmi, mando una notifica a tutti i dispositivi connessi e mi interrompo.
						// Nel caso io non possa fermarmi (ci sono delle comande in attesa o preparazione), lo comunico e non faccio niente.
						if( !comandeInSospeso() ) { // Mi posso fermare
							// Comunico l'esecuzione del comando
							printf("Comunico a tutti la chiusura del server e lo termino.\n");
							fflush(stdout);

							// Invio ad ogni dispositivo connesso il messaggio "STOP"
							strcpy(buffer,"STOP\0");
							for(int j = 1; j < fdmax; j++) {
								if( j == listener) continue; // Salta il listener
								ret = invia(j, buffer);
								if(ret < 0){
									perror("Errore: \n");
									exit(1);
								}
								close(j);
								FD_CLR(j, &master);
							}
							close(listener);

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
					newfd = accept(listener, (struct sockaddr *)&cl_addr, &addrlen);

					// Aggiungo il socket connesso al set dei descrittori monitorati
					FD_SET(newfd, &master); 

					// Aggiorno l'ID del massimo descrittore
					if(newfd > fdmax) { 
						fdmax = newfd;  
					}
				}
				else { // Terzo caso: richiesta da un socket già connesso
					// Cerco il socket nelle mie strutture, se non c'è mi sta per forza comunicando cosa è: tramite un byte;
					// c = client, t = table device, k = kitchen device.
					// Altrimenti, se l'ho trovato, so cosa è e lo gestisco mediante un thread.
					
				}
			}
		}
	}
}