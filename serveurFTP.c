#include "csapp.h"
#include "ftpproto.h"

/* ---------------------------------------------------------------
   Q3 : Configuration du pool de processus
   --------------------------------------------------------------- */
#define NB_PROC     4               /* nombre de processus dans le pool */
#define SERVER_DIR  "./server_files" /* répertoire servi par le serveur  */

/* ---------------------------------------------------------------
   Q3 : Traitement d'une connexion client
   (sera complété aux questions suivantes)
   --------------------------------------------------------------- */
void handle_client(int connfd)
{
    /* TODO : recevoir et traiter la requête */
    Close(connfd);
}

/* ---------------------------------------------------------------
   Q3 : Programme principal
   --------------------------------------------------------------- */
int main(int argc, char **argv)
{
    int listenfd, connfd;
    int i;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;

    listenfd = Open_listenfd(SERVER_PORT);
    printf("[serveur] en écoute sur le port %d (répertoire : %s)\n",
           SERVER_PORT, SERVER_DIR);

    /* créer le pool de NB_PROC processus fils */
    for (i = 0; i < NB_PROC; i++) {
        if (Fork() == 0) {
            /* --- code du fils : boucle d'acceptation --- */
            while (1) {
                clientlen = sizeof(struct sockaddr_storage);
                connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
                handle_client(connfd);
            }
            exit(0); /* jamais atteint */
        }
    }

    /* le père attend indéfiniment */
    while (1)
        pause();

    return 0;
}
