#ifndef FTP_H
#define FTP_H

#include "csapp.h"

#define MASTER_PORT 2121
#define SLAVE_BASE_PORT 3000
#define SLAVE_REG_PORT 2122   /* port d'enregistrement des esclaves auprès du maître */
#define NB_SLAVES 2
#define CHUNK_SIZE 4096 // Q8: Taille des blocs pour le transfert

// Q1: Types de requêtes
typedef enum { 
    REQ_GET, 
    REQ_PUT, 
    REQ_LS, 
    REQ_RM, 
    REQ_BYE, 
    REQ_AUTH 
} typereq_t;

// Q2: Structure de la requête client -> serveur
typedef struct {
    typereq_t type;
    char filename[MAXLINE];
    off_t resume_offset;    // Q10: Offset pour la reprise après panne
    char username[MAXLINE]; // Q17: Authentification
    char password[MAXLINE];
    size_t file_size;       // Utile pour le PUT (non détaillé ici mais prévu)
} request_t;

// Structure de la réponse serveur -> client
typedef struct {
    int code; // 200: OK, 404: Not Found, 401: Unauthorized
    size_t file_size;
} response_t;

// Informations pour la redirection du Master vers l'Esclave
typedef struct {
    char ip[INET_ADDRSTRLEN];
    int port;
} slave_info_t;

#endif