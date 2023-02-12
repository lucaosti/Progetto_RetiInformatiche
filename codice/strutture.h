/* Strutture necessarie al server */
/* ----------------------------- */

#define nTavoli 16
#define nPiatti ??
#define nMaxClient ??
#define nMaxKd ??
#define nMaxKd ??

/* 			 Gestione dei tavoli 
   ---------------------------------------
   Viene parsato il file "tavoli.txt" dopo
   l'accensione del server.
   --------------------------------------- 
*/

struct tavolo
{
	int numero;	// Univoco nel ristorante
	int nPosti;
	char *sala;
	char *descrizione;
};

struct tavolo tavoli[nTavoli];

/*       Gestione delle prenotazioni 
   ---------------------------------------
   Per ogni tavolo, esiste una lista di 
   prenotazioni ordinata in base al time-
   stamp di esecuzione.
   --------------------------------------- 
*/

struct prenotazione
{
	char *cognome;
	time_t data_ora;
	struct prenotazione *prossima;
	/* 
	Il tavolo non è necessario poiché corrisponde
	all'indice in cui viene salvato all'interno
	dell'array "prenotazioni".
	*/
};

struct prenotazione prenotazioni[nTavoli];

/*           Gestione dei piatti 
   ---------------------------------------
   Viene parsato il file "menu.txt" dopo
   l'accensione del server.
   --------------------------------------- 
*/

struct piatto
{
	char *codice;
	char *nome;
	int prezzo;
	/* 
	Il tavolo non è necessario poiché corrisponde
	all'indice in cui viene salvato all'interno
	dell'array "comande".
	*/
};

struct piatto piatti[nPiatti];

/*         Gestione delle comande 
   ---------------------------------------
   Per ogni tavolo, esiste una lista di 
   comande ordinata in base al time-
   stamp di richiesta. Vengono cancellate
   alla richiesta del conto.
   --------------------------------------- 
*/

struct comanda
{
	int quantita[nPiatti];
	/* 
	Quantità dell'i-esimo piatto corrispondete
	nell'array dei piatti.
	*/
	time_t timestamp;
	/* 
	Il tavolo non è necessario poiché corrisponde
	all'indice in cui viene salvato all'interno
	dell'array "comande".
	*/
};

struct comanda comande[nTavoli];

/*    Gestione del tipo di dispositivi
   ---------------------------------------
   Ho 3 array di interi che tengono gli
   ID dei socket dei relativi dispositivi.
   ---------------------------------------
*/

int socket_client[nMaxClient];
int socket_td[nMaxTd];
int socket_kd[nMaxKd];