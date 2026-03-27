#ifndef PROTOCOL_COMMUN_H
#define PROTOCOL_COMMUN_H
#include "csapp.h"
#define NPROC 2
#define PORT 2121
#define Block 5555

typedef enum {
    O=-5,//ouverture
    R=-4,//reading
    M=-3,//allocation
    S=-1,//stat
}erreur_t;

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
    int block_size ;
}response_t;

#endif