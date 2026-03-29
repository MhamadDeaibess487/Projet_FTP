#ifndef PROTOCOL_COMMUN_H
#define PROTOCOL_COMMUN_H
#include "csapp.h"

//macros etape 1
#define NPROC 2
#define PORT 2121

//macro etape 2
#define Block 5555

//macros etape 3
#define NB_SLAVES 2        
#define PORT_MASTER 2121     
#define PORT_SLAVE_BASE 2122 //ports 2122, 2123...


//-------------------------------------------------------------------------------------


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
    long offset;
}request_t;

typedef struct{
    int status;
    int block_size ;
}response_t;

typedef struct {
    char ip[16];  
    int port;
    int fonctionne;
} slave;

#endif