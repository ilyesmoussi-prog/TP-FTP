#ifndef __FTPPROTO_H__
#define __FTPPROTO_H__

#define SERVER_PORT  2121          /* port d'écoute du serveur FTP */
#define MAXFILENAME  256           /* taille max d'un nom de fichier */
#define BLOCK_SIZE   4096          /* taille des blocs de transfert */

/* ---------------------------------------------------------------
   Q1 : Types de requêtes possibles
   --------------------------------------------------------------- */
typedef enum {
    GET = 1,   /* télécharger un fichier depuis le serveur */
    PUT,       /* envoyer un fichier vers le serveur       */
    LS,        /* lister le répertoire du serveur          */
    BYE        /* terminer la connexion                    */
} typereq_t;

/* ---------------------------------------------------------------
   Q2 : Structure d'une requête client serveur
   --------------------------------------------------------------- */
typedef struct {
    int  type;                     /* type de la requête (typereq_t) */
    char filename[MAXFILENAME];    /* nom du fichier concerné */
} request_t;

/* ---------------------------------------------------------------
   Q2 : Structure d'une réponse serveur client
   --------------------------------------------------------------- */
typedef enum {
    FTP_OK    = 0,   
    FTP_ERROR = 1    
} retcode_t;

typedef struct {
    int  retcode;           /* code de retour (retcode_t) */
    long filesize;         
    char message[MAXLINE];  
} response_t;

#endif /* __FTPPROTO_H__ */