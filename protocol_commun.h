#ifndef PROTOCOL_COMMUN_H
#define PROTOCOL_COMMUN_H
#include "csapp.h"
#include <string.h>

//macros etape 1
#define NPROC 2
#define PORT 2121

//macro etape 2
#define Block 100

//macros etape 3
#define NB_SLAVES 2        
#define PORT_MASTER 2121     
#define PORT_SLAVE_BASE 3000 //ports 3000, 3001...

//-------------------------------------------------------------------------------------


typedef enum {
    C=-1,//connexion
    O=-5,//ouverture
    R=-4,//reading
    M=-3,//allocation
    S= 0,//sucess
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
    char options[256]; //pour les options futures
}request_t;

typedef struct{
    erreur_t status;
    int block_size ;
}response_t;

typedef struct {
    pid_t pid;
    char ip[16];  
    int port;
    int socket_fd;
} slave_t;

typedef struct {
    char ip[16];
    int port;
} redirect_t;

slave_t SLAVES[NB_SLAVES]; //tableau pour stocker les pid des esclaves


#endif