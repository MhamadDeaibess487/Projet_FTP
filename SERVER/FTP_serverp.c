/*
 * echoserveri.c - An iterative echo server
 */


#include "FTP_serverp.h"



#define MAX_NAME_LEN 256

pid_t tab[NPROC];

void sigchld_handler(int sig){
    while (waitpid(-1, NULL, WNOHANG) > 0);
}
void sigint_handler(int sig)
{
    for (int i = 0; i < NPROC; i++) {
        Kill(tab[i], SIGINT);
        
    }
    while (wait(NULL) > 0)
        ;
    exit(0);
}

/* 
 * Note that this code only works with IPv4 addresses
 * (IPv6 is not supported)
 */



void file_send(int connfd,request_t req) {
    size_t n;
    response_t res;

    char *file_mem = malloc(Block);
    if(!file_mem){
        res.status=-3;
        res.block_size = 0;

        Rio_writen(connfd, &res, sizeof(response_t));//on envoi l'erreur
        return;
    }


    
    //remplir un buffer le contenu du fichier
    int fp = open(req.filename,O_RDONLY,0);
    if(fp<0){
        
        res.status=-2;
        res.block_size = 0;

        Rio_writen(connfd, &res, sizeof(response_t));//on envoi l'erreur
        free(file_mem);
        return;
    }

    while((n = Read(fp,file_mem,Block))>0){// on envoi la taille du block 
        printf("j'envoi %ld block\n",n);//on envoie le block
        if(n<0){
            res.status = -4;
            res.block_size =0;
            Rio_writen(connfd, &res, sizeof(response_t));
        }
        res.status =0;
        res.block_size = n;
        Rio_writen(connfd, &res, sizeof(response_t));
        Rio_writen(connfd, file_mem,n);

        
    
    }
    

    res.status=0;
    res.block_size=0;
    Rio_writen(connfd, &res, sizeof(response_t));//on envoi 0 pour indiquer la fin du fichier
    
    Close(fp);
    free(file_mem);
}




int main(int argc, char **argv)
{
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_in clientaddr;
    char client_ip_string[INET_ADDRSTRLEN];
    char client_hostname[MAX_NAME_LEN];
    
   
    //rio_t *rio;
    //struct stat statbuf;
    Signal(SIGCHLD, sigchld_handler);
    Signal(SIGINT, sigint_handler);
    
    pid_t pid;
    
    printf("Server is running ...\n");
    clientlen = (socklen_t)sizeof(clientaddr);

    listenfd = Open_listenfd(PORT);
   
   for(int i = 0; i < NPROC; i++){
        
        pid = Fork();
        
        if(pid == 0){
          
            while (1) {

                clientlen = sizeof(clientaddr);
                connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
                
                /* determine the name of the client */
                Getnameinfo((SA *) &clientaddr, clientlen,
                            client_hostname, MAX_NAME_LEN, 0, 0, 0);
                
                /* determine the textual representation of the client's IP address */
                Inet_ntop(AF_INET, &clientaddr.sin_addr, client_ip_string,
                        INET_ADDRSTRLEN);
                
                printf("server connected to %s (%s)\n", client_hostname,
                    client_ip_string);
                request_t req;
                while (Rio_readn(connfd, &req, sizeof(request_t)) > 0){//tant qu'il ya encore des requetes a traiter
                
                
                    
                    //cas ou il y a erreur -> status pas bon
                    /*if (stat(req.filename, &statbuf) < 0) {
                        res.status = -1; 
                        res.block_size = 0;
                        Rio_writen(connfd, &res, sizeof(response_t));//on envoi l'erreur
                        
                        continue;
                    }*/

                    file_send(connfd,req);

            }
                Close(connfd);
                continue;
            }
                
            
        }else{
            tab[i] = pid;
        }
    }
    while (1) {
        pause();
    }
    
}

