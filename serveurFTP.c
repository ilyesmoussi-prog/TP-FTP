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
   Q6 : traitement d'une requête GET
   Envoie le fichier au client par blocs de BLOCK_SIZE octets
   --------------------------------------------------------------- */
void handle_get(int connfd, request_t *req)
{
    response_t resp;
    struct stat st;
    char filepath[MAXFILENAME + sizeof(SERVER_DIR) + 2];
    int filefd;
    char buf[BLOCK_SIZE];
    ssize_t n;

    /* construire le chemin complet vers le fichier */
    snprintf(filepath, sizeof(filepath), "%s/%s", SERVER_DIR, req->filename);

    /* vérifier que le fichier existe */
    if (stat(filepath, &st) < 0) {
        resp.retcode  = FTP_ERROR;
        resp.filesize = 0;
        snprintf(resp.message, MAXLINE,
                 "Erreur : fichier '%s' introuvable.", req->filename);
        Rio_writen(connfd, &resp, sizeof(response_t));
        return;
    }

    /* envoyer la réponse positive avec la taille du fichier */
    resp.retcode  = FTP_OK;
    resp.filesize = (long)st.st_size;
    snprintf(resp.message, MAXLINE, "OK");
    Rio_writen(connfd, &resp, sizeof(response_t));

    /* envoyer le fichier par blocs */
    filefd = Open(filepath, O_RDONLY, 0);
    while ((n = Read(filefd, buf, BLOCK_SIZE)) > 0)
        Rio_writen(connfd, buf, n);
    Close(filefd);
}

/* ---------------------------------------------------------------
   Q6 : Traitement d'une connexion client
   --------------------------------------------------------------- */
void handle_client(int connfd)
{
    rio_t     rio;
    request_t req;

    Rio_readinitb(&rio, connfd);

    /* recevoir la requête */
    if (Rio_readnb(&rio, &req, sizeof(request_t)) <= 0) {
        Close(connfd);
        return;
    }

    /* dispatcher selon le type de requête */
    switch ((typereq_t)req.type) {
        case GET:
            handle_get(connfd, &req);
            break;
        default: {
            response_t resp;
            resp.retcode  = FTP_ERROR;
            resp.filesize = 0;
            snprintf(resp.message, MAXLINE,
                     "Erreur : type de requête inconnu.");
            Rio_writen(connfd, &resp, sizeof(response_t));
            break;
        }
    }

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

    /* Q5 : créer le répertoire serveur s'il n'existe pas */
    mkdir(SERVER_DIR, 0755);

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