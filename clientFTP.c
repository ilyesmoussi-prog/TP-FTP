#include "csapp.h"
#include "ftpproto.h"
#include <time.h>

#define CLIENT_DIR "./client_files"

/* Q10: Taille d'un fichier local */
long get_local_file_size(const char *filename) {
    struct stat st;
    if (stat(filename, &st) == 0) return st.st_size;
    return 0;
}

/* Q7+Q10: GET avec reprise et statistiques */
void cmd_get(int sockfd, const char *filename) {
    request_t req;
    response_t resp;
    rio_t rio;
    char filepath[MAXFILENAME + sizeof(CLIENT_DIR) + 2];
    char buf[BLOCK_SIZE];
    ssize_t n;
    long received = 0;
    int filefd;
    struct timespec t_start, t_end;
    double elapsed;
    long resume_offset;

    snprintf(filepath, sizeof(filepath), "%s/%s", CLIENT_DIR, filename);
    resume_offset = get_local_file_size(filepath);
    
    if (resume_offset > 0) {
        printf("Reprise à partir de %ld octets...\n", resume_offset);
    }

    memset(&req, 0, sizeof(request_t));
    req.type = GET;
    strncpy(req.filename, filename, MAXFILENAME - 1);
    req.resume_offset = resume_offset;
    Rio_writen(sockfd, &req, sizeof(request_t));

    Rio_readinitb(&rio, sockfd);
    Rio_readnb(&rio, &resp, sizeof(response_t));

    if (resp.retcode != FTP_OK) {
        fprintf(stderr, "%s\n", resp.message);
        return;
    }

    int flags = (resume_offset > 0) ? (O_WRONLY | O_APPEND) : (O_WRONLY | O_CREAT | O_TRUNC);
    filefd = Open(filepath, flags, 0644);

    long bytes_to_read = resp.filesize - resume_offset;
    clock_gettime(CLOCK_MONOTONIC, &t_start);
    
    while (received < bytes_to_read) {
        long remaining = bytes_to_read - received;
        size_t toread = (remaining < BLOCK_SIZE) ? (size_t)remaining : BLOCK_SIZE;
        n = Rio_readnb(&rio, buf, toread);
        if (n <= 0) break;
        Write(filefd, buf, n);
        received += n;
    }
    
    clock_gettime(CLOCK_MONOTONIC, &t_end);
    elapsed = (t_end.tv_sec - t_start.tv_sec) + (t_end.tv_nsec - t_start.tv_nsec) / 1e9;
    Close(filefd);

    printf("Transfert terminé.\n");
    printf("%ld octets reçus en %.2f secondes (%.2f Ko/s).\n",
           received, elapsed, elapsed > 0 ? (received / 1024.0) / elapsed : 0.0);
}

int main(int argc, char **argv) {
    int sockfd;
    char cmd_line[MAXLINE];
    char op[MAXLINE], arg1[MAXLINE];

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <adresse_serveur>\n", argv[0]);
        exit(1);
    }

    mkdir(CLIENT_DIR, 0755);
    chdir(CLIENT_DIR);

    sockfd = Open_clientfd(argv[1], SERVER_PORT);
    printf("Connecté au serveur FTP.\n");

    while (1) {
        printf("\nftp> ");
        if (Fgets(cmd_line, MAXLINE, stdin) == NULL) break;
        cmd_line[strcspn(cmd_line, "\n")] = '\0';

        int args = sscanf(cmd_line, "%s %s", op, arg1);

        if (strcmp(op, "get") == 0 && args >= 2) {
            cmd_get(sockfd, arg1);
        } 
        else if (strcmp(op, "bye") == 0 || strcmp(op, "quit") == 0) {
            request_t req;
            memset(&req, 0, sizeof(request_t));
            req.type = BYE;
            Rio_writen(sockfd, &req, sizeof(request_t));
            printf("Déconnexion.\n");
            break;
        }
        else {
            printf("Commandes: get <fichier> | bye\n");
        }
    }

    Close(sockfd);
    return 0;
}