#include "ftpproto.h"
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <string.h>

/* Utilitaire pour obtenir la taille d'un fichier local */
off_t get_local_file_size(const char *filename) {
    struct stat st;
    if (stat(filename, &st) == 0) return st.st_size;
    return 0;
}

/* Q14: Reconnexion au maître pour obtenir un nouvel esclave */
int reconnect_to_master(char *master_host, char *new_ip, int *new_port) {
    int master_fd;
    slave_info_t slave_info;
    rio_t rio_master;
    
    master_fd = Open_clientfd(master_host, MASTER_PORT);
    if (master_fd < 0) return -1;
    
    Rio_readinitb(&rio_master, master_fd);
    if (Rio_readnb(&rio_master, &slave_info, sizeof(slave_info_t)) <= 0) {
        Close(master_fd);
        return -1;
    }
    Close(master_fd);
    
    /* Copier l'IP au lieu de stocker le pointeur */
    strcpy(new_ip, slave_info.ip);
    *new_port = slave_info.port;
    return 0;
}

/* Q14: GET avec gestion de panne et reconnexion */
int robust_get(int *slave_fd, char *master_host, char *filename, off_t resume_offset) {
    request_t req;
    response_t res;
    rio_t rio_slave;
    int retry = 0;
    int max_retry = 3;
    char new_ip[INET_ADDRSTRLEN];
    int new_port;
    int fd = -1;
    
    while (retry < max_retry) {
        memset(&req, 0, sizeof(request_t));
        req.type = REQ_GET;
        strcpy(req.filename, filename);
        req.resume_offset = resume_offset;
        
        /* Envoyer la requête */
        errno = 0;
        Rio_writen(*slave_fd, &req, sizeof(request_t));
        
        /* Vérifier si la socket est toujours valide avec un select non bloquant */
        fd_set fds;
        struct timeval tv = {0, 0};
        FD_ZERO(&fds);
        FD_SET(*slave_fd, &fds);
        int ret = select(*slave_fd + 1, NULL, &fds, NULL, &tv);
        
        if (ret < 0 || (ret == 0 && errno != 0)) {
            printf("\n[Reconnexion] Esclave indisponible, tentative %d/%d...\n", retry + 1, max_retry);
            Close(*slave_fd);
            
            if (reconnect_to_master(master_host, new_ip, &new_port) < 0) {
                printf("[Reconnexion] Impossible de joindre le maître\n");
                return -1;
            }
            
            *slave_fd = Open_clientfd(new_ip, new_port);
            if (*slave_fd < 0) {
                printf("[Reconnexion] Impossible de se connecter au nouvel esclave\n");
                return -1;
            }
            Rio_readinitb(&rio_slave, *slave_fd);
            retry++;
            continue;
        }
        
        /* Attendre la réponse */
        Rio_readinitb(&rio_slave, *slave_fd);
        if (Rio_readnb(&rio_slave, &res, sizeof(response_t)) <= 0) {
            printf("\n[Reconnexion] Connexion perdue avec l'esclave, tentative %d/%d...\n", retry + 1, max_retry);
            Close(*slave_fd);
            
            if (reconnect_to_master(master_host, new_ip, &new_port) < 0) {
                printf("[Reconnexion] Impossible de joindre le maître\n");
                return -1;
            }
            
            *slave_fd = Open_clientfd(new_ip, new_port);
            if (*slave_fd < 0) {
                printf("[Reconnexion] Impossible de se connecter au nouvel esclave\n");
                return -1;
            }
            Rio_readinitb(&rio_slave, *slave_fd);
            retry++;
            continue;
        }
        
        if (res.code == 200) {
            /* Ouvrir le fichier en écriture */
            if (resume_offset > 0) {
                fd = open(filename, O_WRONLY | O_APPEND, 0644);
            } else {
                fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            }
            
            if (fd < 0) {
                printf("Erreur: impossible d'ouvrir le fichier local\n");
                return -1;
            }
            
            size_t bytes_to_read = res.file_size - resume_offset;
            size_t total_read = 0;
            char chunk[CHUNK_SIZE];
            
            while (total_read < bytes_to_read) {
                size_t to_read = (bytes_to_read - total_read < CHUNK_SIZE) 
                                 ? (bytes_to_read - total_read) : CHUNK_SIZE;
                int n = Rio_readnb(&rio_slave, chunk, to_read);
                if (n <= 0) {
                    /* Panne pendant le transfert - on sauvegarde la progression */
                    printf("\n[Reconnexion] Panne pendant transfert, progression sauvegardée (%zu/%zu octets)\n", 
                           total_read, bytes_to_read);
                    close(fd);
                    return robust_get(slave_fd, master_host, filename, resume_offset + total_read);
                }
                write(fd, chunk, n);
                total_read += n;
            }
            close(fd);
            printf("\nTransfert terminé (%zu octets reçus).\n", total_read);
            return 0;
        } else {
            printf("Erreur: code serveur %d\n", res.code);
            return -1;
        }
    }
    printf("[Reconnexion] Échec après %d tentatives\n", max_retry);
    return -1;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <master_host>\n", argv[0]);
        exit(0);
    }
    char *master_host = argv[1];
    
    /* --- PHASE 1: Connexion au Maître --- */
    int master_fd = Open_clientfd(master_host, MASTER_PORT);
    slave_info_t slave_info;
    rio_t rio_master;
    Rio_readinitb(&rio_master, master_fd);
    if (Rio_readnb(&rio_master, &slave_info, sizeof(slave_info_t)) <= 0) {
        fprintf(stderr, "Erreur: impossible de recevoir la redirection du maître\n");
        Close(master_fd);
        exit(1);
    }
    Close(master_fd);
    
    printf("Redirigé par le Maître vers l'Esclave [%s:%d]\n", slave_info.ip, slave_info.port);
    
    /* --- PHASE 2: Connexion à l'Esclave --- */
    int slave_fd = Open_clientfd(slave_info.ip, slave_info.port);
    if (slave_fd < 0) {
        fprintf(stderr, "Erreur: impossible de se connecter à l'esclave\n");
        exit(1);
    }
    rio_t rio_slave;
    Rio_readinitb(&rio_slave, slave_fd);
    
    char cmd_line[MAXLINE];
    request_t req;
    response_t res;
    
    /* Q5: Dossier de travail du client */
    mkdir("./client_files", 0755);
    chdir("./client_files");
    printf("[Client] Répertoire de travail: %s\n", getcwd(NULL, 0));
    
    printf("Connecté. Tapez 'help' pour voir les commandes.\n");
    
    while (1) {
        printf("ftp> ");
        if (Fgets(cmd_line, MAXLINE, stdin) == NULL) break;
        cmd_line[strcspn(cmd_line, "\n")] = 0;
        
        char cmd[MAXLINE], arg1[MAXLINE], arg2[MAXLINE];
        int args = sscanf(cmd_line, "%s %s %s", cmd, arg1, arg2);
        
        memset(&req, 0, sizeof(request_t));
        
        if (strcmp(cmd, "get") == 0 && args == 2) {
            off_t resume_offset = get_local_file_size(arg1);
            if (resume_offset > 0) {
                printf("Reprise du téléchargement à partir de %ld octets...\n", resume_offset);
            }
            
            /* Q14: Utiliser la version robuste avec reconnexion */
            if (robust_get(&slave_fd, master_host, arg1, resume_offset) < 0) {
                printf("Erreur: Échec du téléchargement après reconnexion\n");
            }
        } 
        else if (strcmp(cmd, "put") == 0 && args == 2) {
            struct stat st;
            if (stat(arg1, &st) < 0) {
                printf("Erreur : fichier local '%s' introuvable.\n", arg1);
                continue;
            }
            req.type = REQ_PUT;
            strcpy(req.filename, arg1);
            req.file_size = st.st_size;
            
            Rio_writen(slave_fd, &req, sizeof(request_t));
            if (Rio_readnb(&rio_slave, &res, sizeof(response_t)) <= 0) {
                printf("Erreur: perte de connexion avec l'esclave\n");
                continue;
            }
            
            if (res.code == 200) {
                int fd = open(arg1, O_RDONLY);
                if (fd < 0) {
                    printf("Erreur: impossible d'ouvrir le fichier local\n");
                    continue;
                }
                char chunk[CHUNK_SIZE];
                int n;
                while ((n = read(fd, chunk, CHUNK_SIZE)) > 0) {
                    Rio_writen(slave_fd, chunk, n);
                }
                close(fd);
                printf("Fichier '%s' envoyé avec succès.\n", arg1);
            } else if (res.code == 401) {
                printf("Erreur : non autorisé. Utilisez 'auth admin 1234'.\n");
            } else {
                printf("Erreur lors de l'envoi du fichier.\n");
            }
        }
        else if (strcmp(cmd, "auth") == 0 && args == 3) {
            memset(&req, 0, sizeof(request_t));
            req.type = REQ_AUTH;
            strcpy(req.username, arg1);
            strcpy(req.password, arg2);
            Rio_writen(slave_fd, &req, sizeof(request_t));
            if (Rio_readnb(&rio_slave, &res, sizeof(response_t)) <= 0) {
                printf("Erreur: perte de connexion avec l'esclave\n");
                continue;
            }
            if (res.code == 200) printf("Authentification réussie.\n");
            else printf("Échec de l'authentification.\n");
        }
        else if (strcmp(cmd, "ls") == 0) {
            req.type = REQ_LS;
            Rio_writen(slave_fd, &req, sizeof(request_t));
            if (Rio_readnb(&rio_slave, &res, sizeof(response_t)) <= 0) {
                printf("Erreur: perte de connexion avec l'esclave\n");
                continue;
            }
            
            size_t total_read = 0;
            char chunk[CHUNK_SIZE];
            while (total_read < res.file_size) {
                size_t to_read = (res.file_size - total_read < CHUNK_SIZE) ? (res.file_size - total_read) : CHUNK_SIZE;
                int n = Rio_readnb(&rio_slave, chunk, to_read);
                if (n <= 0) break;
                fwrite(chunk, 1, n, stdout);
                total_read += n;
            }
        }
        else if (strcmp(cmd, "rm") == 0 && args == 2) {
            req.type = REQ_RM;
            strcpy(req.filename, arg1);
            Rio_writen(slave_fd, &req, sizeof(request_t));
            if (Rio_readnb(&rio_slave, &res, sizeof(response_t)) <= 0) {
                printf("Erreur: perte de connexion avec l'esclave\n");
                continue;
            }
            if (res.code == 200) printf("Fichier supprimé.\n");
            else if (res.code == 401) printf("Erreur: Non autorisé. Utilisez 'auth admin 1234'.\n");
            else printf("Erreur: Fichier introuvable.\n");
        }
        else if (strcmp(cmd, "bye") == 0 || strcmp(cmd, "quit") == 0) {
            req.type = REQ_BYE;
            Rio_writen(slave_fd, &req, sizeof(request_t));
            break;
        }
        else if (strcmp(cmd, "help") == 0) {
            printf("Commandes : get <fichier> | put <fichier> | ls | rm <fichier> | auth <user> <pass> | bye\n");
        }
        else {
            printf("Commandes : get <fichier> | put <fichier> | ls | rm <fichier> | auth <user> <pass> | bye\n");
        }
    }
    Close(slave_fd);
    return 0;
}