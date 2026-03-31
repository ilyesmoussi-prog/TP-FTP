#ifndef __FTPPROTO_H__
#define __FTPPROTO_H__

#include "csapp.h"

/* Ports et configuration */
#define MASTER_PORT     2121
#define SLAVE_BASE_PORT 3000
#define SLAVE_REG_PORT  2122
#define NB_SLAVES       2
#define CHUNK_SIZE      4096    /* Q8: Taille des blocs */
#define HEARTBEAT_INTERVAL 5    /* Q14: Intervalle heartbeat en secondes */

/* Q1: Types de requêtes */
typedef enum { 
    REQ_GET, 
    REQ_PUT, 
    REQ_LS, 
    REQ_RM, 
    REQ_BYE, 
    REQ_AUTH,
    REQ_REGISTER,       /* Q12: Enregistrement esclave */
    REQ_HEARTBEAT       /* Q14: Heartbeat pour esclave */
} typereq_t;

/* Q2: Structure de la requête client -> serveur */
typedef struct {
    typereq_t type;
    char filename[MAXLINE];
    off_t resume_offset;        /* Q10: Reprise */
    char username[MAXLINE];     /* Q17: Authentification */
    char password[MAXLINE];
    size_t file_size;           /* Pour PUT */
    int slave_id;               /* Q12: ID esclave */
    int slave_port;             /* Q12: Port esclave */
    char slave_host[MAXLINE];   /* Q12: Hôte esclave */
} request_t;

/* Codes de retour */
typedef enum {
    FTP_OK = 200,
    FTP_ERROR = 404,
    FTP_AUTH = 401,
    FTP_REDIRECT = 300,        /* Q13: Redirection */
    FTP_HEARTBEAT = 250         /* Q14: Heartbeat */
} ftp_code_t;

/* Structure de la réponse serveur -> client */
typedef struct {
    int code;                   /* ftp_code_t */
    size_t file_size;
    int redirect_port;          /* Q13: Port pour redirection */
    char redirect_host[MAXLINE]; /* Q13: Hôte pour redirection */
} response_t;

/* Informations pour la redirection */
typedef struct {
    char ip[INET_ADDRSTRLEN];
    int port;
} slave_info_t;

#endif