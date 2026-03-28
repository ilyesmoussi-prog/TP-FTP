#include "csapp.h"
#include "ftpproto.h"

/* ---------------------------------------------------------------
   Q3 : Configuration du pool de processus
   --------------------------------------------------------------- */
#define NB_PROC     4                /* nombre de processus dans le pool */
#define SERVER_DIR  "./server_files" /* répertoire servi par le serveur  */

/* ---------------------------------------------------------------
   Q4 : tableau des PIDs des fils pour la terminaison propre
   --------------------------------------------------------------- */
static pid_t workers[NB_PROC];

/* Q4 : traitant SIGINT pour le père — retransmet SIGINT à tous
   les fils et attend leur terminaison avant de quitter        */
void sigint_handler_pere(int sig)
{
    int i;
    printf("\n[serveur] arrêt en cours...\n");
    for (i = 0; i < NB_PROC; i++) {
        if (workers[i] > 0)
            Kill(workers[i], SIGINT);
    }
    for (i = 0; i < NB_PROC; i++) {
        if (workers[i] > 0)
            waitpid(workers[i], NULL, 0);
    }
    printf("[serveur] arrêt propre.\n");
    exit(0);
}

/* Q4 : traitant SIGINT pour les fils — terminer proprement */
void sigint_handler_fils(int sig)
{
    exit(0);
}

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
   Q3+Q4 : Programme principal
   --------------------------------------------------------------- */
int main(int argc, char **argv)
{
    int listenfd, connfd;
    int i;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;

    /* Q4 : le père installe son traitant SIGINT */
    Signal(SIGINT, sigint_handler_pere);

    listenfd = Open_listenfd(SERVER_PORT);
    printf("[serveur] en écoute sur le port %d (répertoire : %s)\n",
           SERVER_PORT, SERVER_DIR);

    /* Q3 : créer le pool de NB_PROC processus fils */
    for (i = 0; i < NB_PROC; i++) {
        if ((workers[i] = Fork()) == 0) {
            /* Q4 : le fils installe son propre traitant SIGINT */
            Signal(SIGINT, sigint_handler_fils);
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