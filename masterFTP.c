#include "ftpproto.h"
#include <time.h>
#include <string.h>

/* Structure pour stocker les esclaves avec timestamp */
typedef struct {
    char ip[INET_ADDRSTRLEN];
    int port;
    time_t last_heartbeat;
    int active;
    int id;
} slave_with_heartbeat_t;

static slave_with_heartbeat_t slaves[NB_SLAVES];
static int slave_count = 0;

/* Q14: Thread pour surveiller les heartbeats */
void *heartbeat_monitor(void *arg) {
    while (1) {
        sleep(HEARTBEAT_INTERVAL * 2);
        time_t now = time(NULL);
        for (int i = 0; i < NB_SLAVES; i++) {
            if (slaves[i].active && (now - slaves[i].last_heartbeat) > HEARTBEAT_INTERVAL * 3) {
                printf("[Master] Esclave %d (%s:%d) ne répond plus. Marqué inactif.\n",
                       i, slaves[i].ip, slaves[i].port);
                slaves[i].active = 0;
            }
        }
    }
    return NULL;
}

int main() {
    int listenfd_slaves, listenfd_clients, connfd;
    socklen_t clientlen;
    struct sockaddr_in clientaddr;
    int turn = 0;
    int i;
    pthread_t monitor_thread;

    /* Initialiser la table des esclaves */
    memset(slaves, 0, sizeof(slaves));
    for (i = 0; i < NB_SLAVES; i++) {
        slaves[i].active = 0;
        slaves[i].id = i;
    }

    /* Q14: Démarrer le thread de monitoring */
    pthread_create(&monitor_thread, NULL, heartbeat_monitor, NULL);

    /* Q12 : le maître ouvre un port d'écoute pour les esclaves */
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
        if (Rio_readnb(&rio, &slaves[i].port, sizeof(int)) <= 0) {
            printf("[Master] Erreur: esclave %d n'a pas envoyé son port\n", i);
            Close(connfd);
            i--;
            continue;
        }
        
        /* Enregistrer l'esclave */
        slaves[i].last_heartbeat = time(NULL);
        slaves[i].active = 1;
        
        Close(connfd);

        printf("[Master] Esclave %d enregistré : %s:%d (actif)\n",
               i, slaves[i].ip, slaves[i].port);
        slave_count++;
    }
    Close(listenfd_slaves);

    /* Q11 : ouvrir le port d'écoute pour les clients */
    listenfd_clients = Open_listenfd(MASTER_PORT);
    printf("[Master] Prêt. En écoute clients sur le port %d\n", MASTER_PORT);
    printf("[Master] Esclaves actifs: ");
    for (i = 0; i < NB_SLAVES; i++) {
        if (slaves[i].active) {
            printf("%s:%d ", slaves[i].ip, slaves[i].port);
        }
    }
    printf("\n");

    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd_clients, (SA *)&clientaddr, &clientlen);

        /* Q14: Trouver un esclave actif */
        int found = -1;
        for (int retry = 0; retry < NB_SLAVES; retry++) {
            int idx = (turn + retry) % NB_SLAVES;
            if (slaves[idx].active) {
                found = idx;
                break;
            }
        }
        
        if (found == -1) {
            printf("[Master] Aucun esclave actif disponible !\n");
            Close(connfd);
            continue;
        }
        
        /* Q13 : envoyer les infos de l'esclave choisi au client */
        slave_info_t slave_to_send;
        strcpy(slave_to_send.ip, slaves[found].ip);
        slave_to_send.port = slaves[found].port;
        
        Rio_writen(connfd, &slave_to_send, sizeof(slave_info_t));
        printf("[Master] Client redirigé vers esclave %d (%s:%d) [actif]\n",
               found, slave_to_send.ip, slave_to_send.port);

        /* Round-Robin */
        turn = (found + 1) % NB_SLAVES;
        Close(connfd);
    }
    return 0;
}