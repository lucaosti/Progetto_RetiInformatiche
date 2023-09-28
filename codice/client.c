#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define BENVENUTO "find --> ricerca la disponibilità per una prenotazione\nbook --> invia una prenotazione\nesc --> termina il client\n"

#define BUFFER_SIZE 1024
#define nTavoli 16

int main(int argc, char* argv[]){
	int ret, sd, i;

	struct sockaddr_in srv_addr;
	char buffer[BUFFER_SIZE];

	// Set di descrittori da monitorare
	fd_set master;

	// Set dei descrittori pronti
	fd_set read_fds;

	// Descrittore max
	int fdmax;

	/* Creazione socket */
	sd = socket(AF_INET,SOCK_STREAM,0);
	
	/* Creazione indirizzo del server */
	memset(&srv_addr, 0, sizeof(srv_addr));
	srv_addr.sin_family = AF_INET;
	srv_addr.sin_port = htons(4242);
	inet_pton(AF_INET, "127.0.0.1", &srv_addr.sin_addr);
	
	/* Connessione */
	ret = connect(sd, (struct sockaddr*)&srv_addr, sizeof(srv_addr));
	
	if(ret < 0){
		perror("Errore in fase di connessione: \n");
		exit(1);
	}

	// Reset dei descrittori
    FD_ZERO(&master);
    FD_ZERO(&read_fds);

	// Aggiungo il socket di ascolto 'listener' e 'stdin' (0) ai socket monitorati
    FD_SET(sd, &master); 
    FD_SET(0, &master); 

	// Tengo traccia del nuovo fdmax
    fdmax = sd; 

    // Stampo i comandi che il client può digitare
    printf(BENVENUTO);

	for(;;){
		// Inizializzo il set read_fds, manipolato dalla select()
        read_fds = master;

        
	}
}