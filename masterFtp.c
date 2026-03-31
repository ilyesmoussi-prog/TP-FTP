#include "ftpproto.h"

int main() {
    int listenfd_slaves, listenfd_clients, connfd;
    socklen_t clientlen;
    struct sockaddr_in clientaddr;
    slave_info_t slaves[NB_SLAVES];
    int turn = 0;
    int i;

    /* Q12 : le maître ouvre un port d'écoute pour les esclaves
             et attend que NB_SLAVES esclaves se connectent
             avant d'accepter des clients                        */
    listenfd_slaves = Open_listenfd(SLAVE_REG_PORT);
    printf("[Master] En attente de %d esclave(s) sur le port %d...\n",
           NB_SLAVES, SLAVE_REG_PORT);

    for (i = 0; i < NB_SLAVES; i++) {
        rio_t rio;
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd_slaves, (SA *)&clientaddr, &clientlen);

        /* récupérer l'IP de l'esclave */
        inet_ntop(AF_INET, &clientaddr.sin_addr, slaves[i].ip, INET_ADDRSTRLEN);

        /* récupérer le port que l'esclave a envoyé */
        Rio_readinitb(&rio, connfd);
        Rio_readnb(&rio, &slaves[i].port, sizeof(int));
        Close(connfd);

        printf("[Master] Esclave %d enregistré : %s:%d\n",
               i, slaves[i].ip, slaves[i].port);
    }
    Close(listenfd_slaves);

    /* Q11 : ouvrir le port d'écoute pour les clients */
    listenfd_clients = Open_listenfd(MASTER_PORT);
    printf("[Master] Prêt. En écoute clients sur le port %d\n", MASTER_PORT);

    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd_clients, (SA *)&clientaddr, &clientlen);

        /* Q13 : envoyer les infos de l'esclave choisi au client */
        Rio_writen(connfd, &slaves[turn], sizeof(slave_info_t));
        printf("[Master] Client redirigé vers esclave %d (%s:%d)\n",
               turn, slaves[turn].ip, slaves[turn].port);

        /* Round-Robin */
        turn = (turn + 1) % NB_SLAVES;
        Close(connfd);
    }
    return 0;
}
