/* Contiene i prototipi delle funzioni necessarie al Server */
/* -------------------------------------------------------- */

// Ritorna 1 nel caso ci siano attualmente delle comande
// "in_preparazione" o "in_servizio". 0 altrimenti
int comandeInSospeso();

// Invia al socket in input il messaggio dentro buffer
int invia(int j, char* buffer);

// Riceve dal socket in input il messaggio e lo mette dentro buffer
int ricevi(int j, int lunghezza, char* buffer);

// Ricevi dal socket in input la lunghezza del messaggio e lo mette dentro lmsg
int riceviLunghezza(int j, int *lmsg);

// Prende uno stato_comanda e inserisce dentro il buffer tutte le informazioni
// delle comande in quello stato di qualunque tavolino
void elencoComande(char* buffer, enum stato_comanda stato);

// Prende un tavolo e inserisce dentro il buffer tutte le informazioni
// delle comande inerenti a quel tavolino
void elencoComandeTavolo(char* buffer, int tavolo);

// Inserisce in base alla lettera c, il socket id nell'array relativo
int inserisci(int i, char c);

// Gestisce UNA richiesta da parte di un client
void gestisciClient(int socketId, char* b);

// Gestisce UNA richiesta da parte di un table device
void gestisciTd(int socketId, char* b);

// Gestisce UNA richiesta da parte di un kitchen device
void gestisciKd(int socketId, char* b);

// Dealloca tutte le strutture
void deallocaStrutture();