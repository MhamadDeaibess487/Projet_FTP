#ifndef FTP_SERVERP_H
#define FTP_SERVERP_H

#define NPROC 2
#define PORT 2121

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


#endif