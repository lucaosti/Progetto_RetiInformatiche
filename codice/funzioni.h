/* Contiene i prototipi delle funzioni necessarie al Server */
/* -------------------------------------------------------- */

// Ritorna 1 nel caso ci siano attualmente delle comande
// "in_preparazione" o "in_servizio". 0 altrimenti
int comandeInSospeso();

// Invia al socket in input il messaggio dentro buffer
int invia(int j, char* buffer);

// Prende uno stato_comanda e inserisce dentro il buffer tutte le informazioni
// delle comande in quello stato di qualunque tavolino
void elencoComande(char* buffer, enum stato_comanda stato);

// Prende un tavolo e inserisce dentro il buffer tutte le informazioni
// delle comande inerenti a quel tavolino
void elencoComandeTavolo(char* buffer, int tavolo);