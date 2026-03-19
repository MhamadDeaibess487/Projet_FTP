#ifndef FTP_CLIENT_H
#define FTP_CLIENT_H


#define PORT 2121
#define NB_PROC 2    

typedef enum{
    GET=1,
    PUT,
    LS,
}typereq_t;

typedef struct{
    typereq_t type;
    char filename[MAXLINE];
}request_t;

#endif