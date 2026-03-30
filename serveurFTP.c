#include "csapp.h"
#include "ftpproto.h"

#define NB_PROC     4
#define SERVER_DIR  "./server_files"

static pid_t workers[NB_PROC];

/* Q4: Traitant SIGINT pour le père */
void sigint_handler_pere(int sig) {
    int i;
    printf("\n[Serveur] Arrêt en cours...\n");
    for (i = 0; i < NB_PROC; i++) {
        if (workers[i] > 0) Kill(workers[i], SIGINT);
    }
    for (i = 0; i < NB_PROC; i++) {
        if (workers[i] > 0) waitpid(workers[i], NULL, 0);
    }
    printf("[Serveur] Arrêt propre.\n");
    exit(0);
}

/* Q4: Traitant SIGINT pour les fils */
void sigint_handler_fils(int sig) {
    exit(0);
}

/* Q6+Q10: GET avec reprise et transfert par blocs */
void handle_get(int connfd, request_t *req, rio_t *rio) {
    response_t resp;
    struct stat st;
    char filepath[MAXFILENAME + sizeof(SERVER_DIR) + 2];
    int filefd = -1;
    char buf[BLOCK_SIZE];
    ssize_t n;
    long bytes_to_send;

    snprintf(filepath, sizeof(filepath), "%s/%s", SERVER_DIR, req->filename);

    if (stat(filepath, &st) < 0) {
        resp.retcode = FTP_ERROR;
        resp.filesize = 0;
        snprintf(resp.message, MAXLINE, "Erreur: fichier '%s' introuvable.", req->filename);
        Rio_writen(connfd, &resp, sizeof(response_t));
        return;
    }

    /* Q8: Transfert par blocs - envoi de la taille d'abord */
    resp.retcode = FTP_OK;
    resp.filesize = (long)st.st_size;
    snprintf(resp.message, MAXLINE, "OK");
    Rio_writen(connfd, &resp, sizeof(response_t));

    /* Q10: Validation offset reprise */
    if (req->resume_offset > st.st_size) {
        req->resume_offset = 0;
    } else if (req->resume_offset == st.st_size) {
        return;
    }

    filefd = Open(filepath, O_RDONLY, 0);
    lseek(filefd, req->resume_offset, SEEK_SET);
    bytes_to_send = st.st_size - req->resume_offset;

    /* Envoi par blocs */
    while (bytes_to_send > 0) {
        size_t to_read = (bytes_to_send < BLOCK_SIZE) ? bytes_to_send : BLOCK_SIZE;
        n = Read(filefd, buf, to_read);
        if (n <= 0) break;
        Rio_writen(connfd, buf, n);
        bytes_to_send -= n;
    }
    Close(filefd);
}

/* Q9: Gestion client avec plusieurs commandes */
void handle_client(int connfd) {
    rio_t rio;
    request_t req;

    Rio_readinitb(&rio, connfd);

    while (Rio_readnb(&rio, &req, sizeof(request_t)) > 0) {
        switch ((typereq_t)req.type) {
            case GET:
                handle_get(connfd, &req, &rio);
                break;
            case BYE:
                Close(connfd);
                return;
            default: {
                response_t resp;
                resp.retcode = FTP_ERROR;
                resp.filesize = 0;
                snprintf(resp.message, MAXLINE, "Commande inconnue.");
                Rio_writen(connfd, &resp, sizeof(response_t));
                break;
            }
        }
    }
    Close(connfd);
}

int main(int argc, char **argv) {
    int listenfd, connfd;
    int i;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;

    Signal(SIGINT, sigint_handler_pere);

    mkdir(SERVER_DIR, 0755);
    chdir(SERVER_DIR);

    listenfd = Open_listenfd(SERVER_PORT);
    printf("[Serveur] Écoute port %d (répertoire: %s)\n", SERVER_PORT, SERVER_DIR);

    for (i = 0; i < NB_PROC; i++) {
        if ((workers[i] = Fork()) == 0) {
            Signal(SIGINT, sigint_handler_fils);
            while (1) {
                clientlen = sizeof(struct sockaddr_storage);
                connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
                printf("[Fils %d] Client connecté.\n", getpid());
                handle_client(connfd);
                printf("[Fils %d] Client déconnecté.\n", getpid());
            }
            exit(0);
        }
    }

    while (1) pause();
    return 0;
}