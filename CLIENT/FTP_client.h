#ifndef FTP_CLIENT_H
#define FTP_CLIENT_H
#include "../csapp.h"

#define PORT 2121
#define NB_PROC 2    
#define Block 55555

typedef enum{
    GET=1,
    PUT,
    LS,
}typereq_t;

typedef struct{
    typereq_t type;
    char filename[MAXLINE];
}request_t;

typedef struct{
    int status;
    int file_size;
}response_t;



void response(int clientfd, char *filename);
#endif