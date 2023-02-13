/* Strutture necessarie al server */
/* ------------------------------ */

#define nTavoli 16
#define nPiatti 8
#define nMaxClient 16
#define nMaxTd nTavoli
#define nMaxKd 8

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
	char sala[32];
	char descrizione[64];
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
	char cognome[64];
	time_t data_ora;	// Non della richiesta, ma della prenotazione
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
	char codice[2];
	char nome[64];
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

enum stato_comanda{in_attesa, in_preparazione, in_attesa};

struct comanda
{
	int quantita[nPiatti];
	/* 
	Quantità dell'i-esimo piatto corrispondete
	nell'array dei piatti.
	*/
	time_t timestamp;	// utilizzato per trovare la meno recente
	enum stato_comanda stato;
	/* 
	Il tavolo non è necessario poiché corrisponde
	all'indice in cui viene salvato all'interno
	dell'array "comande".
	*/
	struct comanda *prossima;
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