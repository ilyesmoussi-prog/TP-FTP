#include "ftpproto.h"
#include <time.h>
#include <string.h>

static int master_fd = -1;
static int slave_port;


/* Handler SIGCHLD pour éviter les processus zombies */
void sigchld_handler(int sig) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

/* Q4: Terminaison propre */
void sigint_handler(int sig) {
    printf("\n[Slave %d] Arrêt du serveur\n", slave_port);
    if (master_fd > 0) Close(master_fd);
    exit(0);
}

void handle_client(int connfd) {
    request_t req;
    response_t res;
    rio_t rio;
    int is_authenticated = 0;

    Rio_readinitb(&rio, connfd);

    while (Rio_readnb(&rio, &req, sizeof(request_t)) > 0) {
        
        /* Q17: Authentification */
        if (req.type == REQ_AUTH) {
            if (strcmp(req.username, "admin") == 0 && strcmp(req.password, "1234") == 0) {
                is_authenticated = 1;
                res.code = 200;
                printf("[Slave %d] Auth OK\n", slave_port);
            } else {
                res.code = 401;
                printf("[Slave %d] Auth FAILED\n", slave_port);
            }
            Rio_writen(connfd, &res, sizeof(response_t));
            continue;
        }
        
        /* Q16: Vérification des droits pour RM et PUT */
        if (!is_authenticated && (req.type == REQ_RM || req.type == REQ_PUT)) {
            res.code = 401;
            Rio_writen(connfd, &res, sizeof(response_t));
            continue;
        }

        if (req.type == REQ_GET) {
            int fd = open(req.filename, O_RDONLY);
            if (fd < 0) {
                res.code = 404;
                Rio_writen(connfd, &res, sizeof(response_t));
                printf("[Slave %d] GET: %s not found\n", slave_port, req.filename);
            } else {
                struct stat st;
                fstat(fd, &st);
                res.code = 200;
                res.file_size = st.st_size;
                Rio_writen(connfd, &res, sizeof(response_t));

                lseek(fd, req.resume_offset, SEEK_SET);

                char chunk[CHUNK_SIZE];
                int n;
                while ((n = read(fd, chunk, CHUNK_SIZE)) > 0) {
                    Rio_writen(connfd, chunk, n);
                }
                close(fd);
                printf("[Slave %d] GET: %s sent (%ld bytes)\n", slave_port, req.filename, st.st_size);
            }
        } 
        else if (req.type == REQ_LS) {
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
            unlink(".tmp_ls.txt");
        }
        else if (req.type == REQ_RM) {
            if (unlink(req.filename) == 0) {
                res.code = 200;
                printf("[Slave %d] RM: %s deleted\n", slave_port, req.filename);
            } else {
                res.code = 404;
            }
            Rio_writen(connfd, &res, sizeof(response_t));
        }
        else if (req.type == REQ_PUT) {
            int fd = open(req.filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) {
                res.code = 404;
                Rio_writen(connfd, &res, sizeof(response_t));
            } else {
                res.code = 200;
                Rio_writen(connfd, &res, sizeof(response_t));

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
                printf("[Slave %d] PUT: %s received (%zu bytes)\n", slave_port, req.filename, total_read);
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
    slave_port = port;
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_in clientaddr;

    Signal(SIGINT, sigint_handler);
    Signal(SIGCHLD, sigchld_handler);
    
    mkdir("server_files", 0755);
    chdir("server_files");
    printf("[Slave %d] Répertoire: %s\n", port, getcwd(NULL, 0));

    listenfd = Open_listenfd(port);
    printf("[Slave %d] Écoute sur port %d\n", port, port);

    /* Q12 : s'enregistrer auprès du maître */
    master_fd = Open_clientfd("localhost", SLAVE_REG_PORT);
    if (master_fd < 0) {
        printf("[Slave %d] Impossible de contacter le maître sur port %d\n", port, SLAVE_REG_PORT);
    } else {
        Rio_writen(master_fd, &port, sizeof(int));
        printf("[Slave %d] Enregistré auprès du maître\n", port);
    }

    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        
        if (Fork() == 0) {
            Close(listenfd);
            handle_client(connfd);
        }
        Close(connfd);
    }
}