#include "ftpproto.h"

// Handler SIGCHLD pour éviter les processus zombies
void sigchld_handler(int sig) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

// Q4: Terminaison propre (nettoyage des processus fils)
void sigint_handler(int sig) {
    printf("\n[Slave] Arrêt du serveur, nettoyage des processus...\n");
    kill(0, SIGKILL); // Tue tous les processus du même groupe
    exit(0);
}

void handle_client(int connfd) {
    request_t req;
    response_t res;
    rio_t rio;
    int is_authenticated = 0; // Q17: booléen pour l'authentification

    Rio_readinitb(&rio, connfd);

    // Q9: Plusieurs requêtes par connexion (Boucle while)
    while (Rio_readnb(&rio, &req, sizeof(request_t)) > 0) {
        
        // Q17: Authentification
        if (req.type == REQ_AUTH) {
            if (strcmp(req.username, "admin") == 0 && strcmp(req.password, "1234") == 0) {
                is_authenticated = 1;
                res.code = 200;
            } else {
                res.code = 401;
            }
            Rio_writen(connfd, &res, sizeof(response_t));
            continue;
        }

        // Q16: Vérification des droits pour RM (et PUT)
        if (!is_authenticated && (req.type == REQ_RM || req.type == REQ_PUT)) {
            res.code = 401; // Unauthorized
            Rio_writen(connfd, &res, sizeof(response_t));
            continue;
        }

        if (req.type == REQ_GET) {
            int fd = open(req.filename, O_RDONLY);
            if (fd < 0) {
                res.code = 404;
                Rio_writen(connfd, &res, sizeof(response_t));
            } else {
                struct stat st;
                fstat(fd, &st);
                res.code = 200;
                res.file_size = st.st_size;
                Rio_writen(connfd, &res, sizeof(response_t));

                // Q10: Reprise de téléchargement (On avance dans le fichier)
                lseek(fd, req.resume_offset, SEEK_SET);

                // Q8: Envoi par blocs (Chunking) pour gros fichiers et fichiers binaires
                char chunk[CHUNK_SIZE];
                int n;
                while ((n = read(fd, chunk, CHUNK_SIZE)) > 0) {
                    Rio_writen(connfd, chunk, n);
                }
                close(fd);
            }
        } 
        else if (req.type == REQ_LS) { // Q15: Commande ls
            system("ls -la > .tmp_ls.txt");
            int fd = open(".tmp_ls.txt", O_RDONLY);
            struct stat st;
            fstat(fd, &st);
            res.code = 200;
            res.file_size = st.st_size;
            Rio_writen(connfd, &res, sizeof(response_t));
            
            char chunk[CHUNK_SIZE];
            int n;
            while ((n = read(fd, chunk, CHUNK_SIZE)) > 0) {
                Rio_writen(connfd, chunk, n);
            }
            close(fd);
            unlink(".tmp_ls.txt"); // On supprime le fichier temporaire
        }
        else if (req.type == REQ_RM) { // Q16: Commande rm
            if (unlink(req.filename) == 0) res.code = 200;
            else res.code = 404;
            Rio_writen(connfd, &res, sizeof(response_t));
        }
        else if (req.type == REQ_PUT) { // Q16: Commande put
            int fd = open(req.filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) {
                res.code = 404;
                Rio_writen(connfd, &res, sizeof(response_t));
            } else {
                res.code = 200;
                Rio_writen(connfd, &res, sizeof(response_t));

                // Recevoir le fichier par blocs
                size_t total_read = 0;
                char chunk[CHUNK_SIZE];
                while (total_read < req.file_size) {
                    size_t to_read = (req.file_size - total_read < CHUNK_SIZE)
                                     ? req.file_size - total_read : CHUNK_SIZE;
                    int n = Rio_readnb(&rio, chunk, to_read);
                    if (n <= 0) break;
                    write(fd, chunk, n);
                    total_read += n;
                }
                close(fd);
                printf("[Slave] Fichier '%s' reçu (%zu octets).\n",
                       req.filename, total_read);
            }
        }
        else if (req.type == REQ_BYE) {
            break;
        }
    }
    Close(connfd);
    exit(0);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }
    int port = atoi(argv[1]);
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_in clientaddr;

    Signal(SIGINT, sigint_handler);
    Signal(SIGCHLD, sigchld_handler);

    listenfd = Open_listenfd(port);
    printf("[Slave] Esclave en écoute sur le port %d\n", port);

    /* Q12 : s'enregistrer auprès du maître en lui envoyant notre port */
    int fd_master = Open_clientfd("localhost", SLAVE_REG_PORT);
    Rio_writen(fd_master, &port, sizeof(int));
    Close(fd_master);
    printf("[Slave] Enregistré auprès du maître.\n");

    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        
        // Q3: Serveur concurrent (Pool basique avec fork)
        if (Fork() == 0) {
            Close(listenfd); // Le fils ferme l'écoute du serveur
            // Q5: Dossier de travail du serveur (pourrait être un chdir("./serveur_dir"))
            handle_client(connfd);
        }
        Close(connfd); // Le père ferme la connexion traitée par le fils
    }
}