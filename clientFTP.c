#include "csapp.h"
#include "ftpproto.h"

/* ---------------------------------------------------------------
   Q3 : Configuration
   --------------------------------------------------------------- */
#define CLIENT_DIR  "./client_files"  /* répertoire de sauvegarde client */

/* ---------------------------------------------------------------
   Q3 : Programme principal
   --------------------------------------------------------------- */
int main(int argc, char **argv)
{
    int  sockfd;
    char cmd[MAXLINE];

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <hostname>\n", argv[0]);
        exit(1);
    }

    /* connexion au serveur */
    sockfd = Open_clientfd(argv[1], SERVER_PORT);
    printf("Connected to %s.\n", argv[1]);

    /* boucle de commandes (sera complétée aux questions suivantes) */
    printf("ftp> ");
    fflush(stdout);
    while (fgets(cmd, MAXLINE, stdin) != NULL) {
        cmd[strcspn(cmd, "\n")] = '\0';

        if (strcmp(cmd, "bye") == 0) {
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
