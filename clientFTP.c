#include "csapp.h"
#include "ftpproto.h"
#include <time.h>

/* ---------------------------------------------------------------
   Q5 : Configuration
   --------------------------------------------------------------- */
#define CLIENT_DIR  "./client_files"  /* répertoire de sauvegarde client */

/* ---------------------------------------------------------------
   Q7 : traitement de la commande get
   --------------------------------------------------------------- */
void cmd_get(int sockfd, const char *filename)
{
    request_t  req;
    response_t resp;
    rio_t      rio;
    char       filepath[MAXFILENAME + sizeof(CLIENT_DIR) + 2];
    char       buf[BLOCK_SIZE];
    ssize_t    n;
    long       received = 0;
    int        filefd;
    struct timespec t_start, t_end;
    double     elapsed;

    /* préparer et envoyer la requête GET */
    memset(&req, 0, sizeof(request_t));
    req.type = GET;
    strncpy(req.filename, filename, MAXFILENAME - 1);
    Rio_writen(sockfd, &req, sizeof(request_t));

    /* recevoir la réponse du serveur */
    Rio_readinitb(&rio, sockfd);
    Rio_readnb(&rio, &resp, sizeof(response_t));

    if (resp.retcode != FTP_OK) {
        fprintf(stderr, "%s\n", resp.message);
        return;
    }

    /* ouvrir le fichier de destination */
    snprintf(filepath, sizeof(filepath), "%s/%s", CLIENT_DIR, filename);
    filefd = Open(filepath, O_WRONLY | O_CREAT | O_TRUNC, DEF_MODE);

    /* recevoir le fichier par blocs et mesurer le temps */
    clock_gettime(CLOCK_MONOTONIC, &t_start);
    while (received < resp.filesize) {
        long   remaining = resp.filesize - received;
        size_t toread    = (remaining < BLOCK_SIZE) ? (size_t)remaining
                                                     : BLOCK_SIZE;
        n = Rio_readnb(&rio, buf, toread);
        if (n <= 0) break;
        Rio_writen(filefd, buf, n);
        received += n;
    }
    clock_gettime(CLOCK_MONOTONIC, &t_end);

    elapsed = (t_end.tv_sec  - t_start.tv_sec)
            + (t_end.tv_nsec - t_start.tv_nsec) / 1e9;

    Close(filefd);

    printf("Transfert terminé avec succès.");
    printf("%ld octets reçus en %.2f secondes (%.2f Koctets/s).\n",
           received, elapsed,
           elapsed > 0 ? (received / 1024.0) / elapsed : 0.0);
}

/* ---------------------------------------------------------------
   Q3+Q5+Q7 : Programme principal
   --------------------------------------------------------------- */
int main(int argc, char **argv)
{
    int  sockfd;
    char cmd[MAXLINE];
    char filename[MAXFILENAME];

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <nom_serveur>", argv[0]);
        exit(1);
    }

    /* Q5 : créer le répertoire client s'il n'existe pas */
    mkdir(CLIENT_DIR, 0755);

    /* connexion au serveur */
    sockfd = Open_clientfd(argv[1], SERVER_PORT);
    printf("Connecte à %s.", argv[1]);

    /* boucle de commandes */
    printf("ftp> ");
    fflush(stdout);
    while (fgets(cmd, MAXLINE, stdin) != NULL) {
        cmd[strcspn(cmd, "\n")] = '\0';

        if (strncmp(cmd, "get ", 4) == 0) {
            strncpy(filename, cmd + 4, MAXFILENAME - 1);
            cmd_get(sockfd, filename);
        } else if (strcmp(cmd, "bye") == 0) {
            break;
        } else {
            printf("Commandes disponibles : get <fichier>, bye\n");
        }

        printf("ftp> ");
        fflush(stdout);
    }

    Close(sockfd);
    return 0;
}
