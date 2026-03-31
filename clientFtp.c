#include "ftpproto.h"

// Utilitaire pour obtenir la taille d'un fichier local (pour la reprise de téléchargement - Q10)
off_t get_local_file_size(const char *filename) {
    struct stat st;
    if (stat(filename, &st) == 0) return st.st_size;
    return 0;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <master_host>\n", argv[0]);
        exit(0);
    }

    // --- PHASE 1: Connexion au Maître --- (Q13)
    int master_fd = Open_clientfd(argv[1], MASTER_PORT);
    slave_info_t slave_info;
    rio_t rio_master;
    Rio_readinitb(&rio_master, master_fd);
    Rio_readnb(&rio_master, &slave_info, sizeof(slave_info_t));
    Close(master_fd);
    
    printf("Redirigé par le Maître vers l'Esclave [%s:%d]\n", slave_info.ip, slave_info.port);

    // --- PHASE 2: Connexion à l'Esclave ---
    int slave_fd = Open_clientfd(slave_info.ip, slave_info.port);
    rio_t rio_slave;
    Rio_readinitb(&rio_slave, slave_fd);
    
    char cmd_line[MAXLINE];
    request_t req;
    response_t res;

    // Q5: Dossier de travail du client
    // mkdir("./client_dir", 0777); chdir("./client_dir");

    printf("Connecté. Tapez 'help' pour voir les commandes.\n");

    // Q9: Maintien de la connexion (Interface Interactive)
    while (1) {
        printf("ftp> ");
        if (Fgets(cmd_line, MAXLINE, stdin) == NULL) break;
        cmd_line[strcspn(cmd_line, "\n")] = 0; // Enlever le \n final

        char cmd[MAXLINE], arg1[MAXLINE], arg2[MAXLINE];
        int args = sscanf(cmd_line, "%s %s %s", cmd, arg1, arg2);
        
        memset(&req, 0, sizeof(request_t));

        if (strcmp(cmd, "get") == 0 && args == 2) {
            req.type = REQ_GET;
            strcpy(req.filename, arg1);
            
            // Q10: Gestion des pannes / Reprise
            req.resume_offset = get_local_file_size(arg1); 
            if (req.resume_offset > 0) {
                printf("Reprise du téléchargement à partir de %ld octets...\n", req.resume_offset);
            }

            Rio_writen(slave_fd, &req, sizeof(request_t));
            Rio_readnb(&rio_slave, &res, sizeof(response_t));

            if (res.code == 200) {
                // Ouverture en mode Append (ajout) si reprise, sinon Write (écrase)
                int flags = (req.resume_offset > 0) ? (O_WRONLY | O_APPEND) : (O_WRONLY | O_CREAT | O_TRUNC);
                int fd = open(arg1, flags, 0644);
                
                size_t bytes_to_read = res.file_size - req.resume_offset;
                size_t total_read = 0;
                char chunk[CHUNK_SIZE];
                
                // Q8: Réception par blocs
                while (total_read < bytes_to_read) {
                    size_t to_read = (bytes_to_read - total_read < CHUNK_SIZE) ? (bytes_to_read - total_read) : CHUNK_SIZE;
                    int n = Rio_readnb(&rio_slave, chunk, to_read);
                    if (n <= 0) {
                        printf("\nErreur: connexion interrompue. Fichier partiellement téléchargé.\n");
                        break; 
                    }
                    write(fd, chunk, n);
                    total_read += n;
                }
                close(fd);
                if(total_read == bytes_to_read) printf("Transfert terminé (%zu octets reçus).\n", total_read);
            } else {
                printf("Erreur: Fichier inexistant (404) sur le serveur.\n");
            }
        } 
        else if (strcmp(cmd, "put") == 0 && args == 2) { // Q16
            struct stat st;
            if (stat(arg1, &st) < 0) {
                printf("Erreur : fichier local '%s' introuvable.\n", arg1);
                continue;
            }
            req.type = REQ_PUT;
            strcpy(req.filename, arg1);
            req.file_size = st.st_size;

            Rio_writen(slave_fd, &req, sizeof(request_t));
            Rio_readnb(&rio_slave, &res, sizeof(response_t));

            if (res.code == 200) {
                int fd = open(arg1, O_RDONLY);
                char chunk[CHUNK_SIZE];
                int n;
                while ((n = read(fd, chunk, CHUNK_SIZE)) > 0)
                    Rio_writen(slave_fd, chunk, n);
                close(fd);
                printf("Fichier '%s' envoyé avec succès.\n", arg1);
            } else if (res.code == 401) {
                printf("Erreur : non autorisé. Utilisez 'auth admin 1234'.\n");
            } else {
                printf("Erreur lors de l'envoi du fichier.\n");
            }
        }
        else if (strcmp(cmd, "auth") == 0 && args == 3) { // Q17
            strcpy(req.username, arg1);
            strcpy(req.password, arg2);
            Rio_writen(slave_fd, &req, sizeof(request_t));
            Rio_readnb(&rio_slave, &res, sizeof(response_t));
            if (res.code == 200) printf("Authentification réussie.\n");
            else printf("Échec de l'authentification.\n");
        }
        else if (strcmp(cmd, "ls") == 0) { // Q15
            req.type = REQ_LS;
            Rio_writen(slave_fd, &req, sizeof(request_t));
            Rio_readnb(&rio_slave, &res, sizeof(response_t));
            
            size_t total_read = 0;
            char chunk[CHUNK_SIZE];
            while (total_read < res.file_size) {
                size_t to_read = (res.file_size - total_read < CHUNK_SIZE) ? (res.file_size - total_read) : CHUNK_SIZE;
                int n = Rio_readnb(&rio_slave, chunk, to_read);
                fwrite(chunk, 1, n, stdout);
                total_read += n;
            }
        }
        else if (strcmp(cmd, "rm") == 0 && args == 2) { // Q16
            req.type = REQ_RM;
            strcpy(req.filename, arg1);
            Rio_writen(slave_fd, &req, sizeof(request_t));
            Rio_readnb(&rio_slave, &res, sizeof(response_t));
            if (res.code == 200) printf("Fichier supprimé.\n");
            else if (res.code == 401) printf("Erreur: Non autorisé. Utilisez 'auth admin 1234'.\n");
            else printf("Erreur: Fichier introuvable.\n");
        }
        else if (strcmp(cmd, "bye") == 0 || strcmp(cmd, "quit") == 0) {
            req.type = REQ_BYE;
            Rio_writen(slave_fd, &req, sizeof(request_t));
            break;
        }
        else {
            printf("Commandes disponibles : get <fichier> | put <fichier> | ls | rm <fichier> | auth <user> <pass> | bye\n");
        }
    }
    Close(slave_fd);
    return 0;
}