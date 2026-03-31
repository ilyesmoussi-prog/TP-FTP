#ifndef __FTPPROTO_H__
#define __FTPPROTO_H__

#include "csapp.h"

#define MASTER_PORT 2121
#define SLAVE_BASE_PORT 3000
#define SLAVE_REG_PORT 2122
#define NB_SLAVES 2
#define CHUNK_SIZE 4096

typedef enum { 
    REQ_GET, 
    REQ_PUT, 
    REQ_LS, 
    REQ_RM, 
    REQ_BYE, 
    REQ_AUTH 
} typereq_t;

typedef struct {
    typereq_t type;
    char filename[MAXLINE];
    off_t resume_offset;
    char username[MAXLINE];
    char password[MAXLINE];
    size_t file_size;
} request_t;

typedef struct {
    int code;
    size_t file_size;
} response_t;

typedef struct {
    char ip[INET_ADDRSTRLEN];
    int port;
} slave_info_t;

#endif
