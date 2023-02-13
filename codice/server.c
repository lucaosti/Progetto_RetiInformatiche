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

int main(int argc, char* argv[]){
    // Carico dai file "tavoli.txt" e "menu.txt"
    caricaTavoli();
    caricaMenu();
    /* -------------------------------------- */

    
}