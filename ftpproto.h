#ifndef __FTPPROTO_H__
#define __FTPPROTO_H__

#define SERVER_PORT  2121
#define MAXFILENAME  256
#define BLOCK_SIZE   4096

/* Q1: Types de requêtes - Étape 1 et 2 uniquement */
typedef enum {
    GET = 1,   /* télécharger un fichier */
    BYE        /* terminer la connexion */
} typereq_t;

/* Q2: Structure d'une requête */
typedef struct {
    int  type;                     /* type de la requête */
    char filename[MAXFILENAME];    /* nom du fichier */
    long resume_offset;            /* Q10: reprise */
} request_t;

/* Codes de retour */
typedef enum {
    FTP_OK    = 0,   
    FTP_ERROR = 1
} retcode_t;

/* Structure d'une réponse */
typedef struct {
    int  retcode;
    long filesize;
    char message[MAXLINE];
} response_t;

#endif