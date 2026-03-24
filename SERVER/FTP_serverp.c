/*
 * echoserveri.c - An iterative echo server
 */

#include "../csapp.h"
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

 // a bien implementer la fonction de transfert de fichier
 /*
int file_send(size_t size,int connfd){
    char buf[size];
    for(size_t i=0;i<size;i++){
       
        
        Rio_writen(connfd, buf, MAXBUF);
        
    }
    return 0;

}
*/

void file_send(int connfd, char *file_mem, size_t size) {
    Rio_writen(connfd, file_mem, size);
}




int main(int argc, char **argv)
{
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_in clientaddr;
    char client_ip_string[INET_ADDRSTRLEN];
    char client_hostname[MAX_NAME_LEN];
    //rio_t rio;
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
                //Rio_readinitb(&rio, connfd);
                request_t req;
                response_t res;
                struct stat statbuf;
                while (Rio_readn(connfd, &req, sizeof(request_t)) > 0){
                
                
                
                //cas ou il y a erreur -> status pas bon
                if (stat(req.filename, &statbuf) < 0) {
                    res.status = -1; 
                    res.file_size = 0;
                    Rio_writen(connfd, &res, sizeof(response_t));//on envoi l'erreur
                    
                    continue;
                }

                char *file_mem = malloc(statbuf.st_size);
                if(!file_mem){
                    res.status=-3;
                    Rio_writen(connfd, &res, sizeof(response_t));//on envoi l'erreur
                    continue;
                }

              
                
                //remplir un buffer le contenu du fichier
                FILE *fp = fopen(req.filename, "rb");
                if(!fp){
                    printf("probleme avec l'ouverture du fichier\n");
                    res.status=-5;
                    Rio_writen(connfd, &res, sizeof(response_t));//on envoi l'erreur
                    free(file_mem);
                    continue;
                }
                size_t n =fread(file_mem, 1, statbuf.st_size, fp);
                if(n!=statbuf.st_size){
                    printf("probleme de lecture du fichier\n");
                    res.status =-4;
                    Rio_writen(connfd, &res, sizeof(response_t));//on envoi l'erreur
                    Fclose(fp);
                    free(file_mem);
                    continue;
                }
                  //cas ou y a pas derrers -> status bon
                res.status=0;
                res.file_size = statbuf.st_size;
                Rio_writen(connfd, &res, sizeof(response_t));
                fclose(fp);
                file_send(connfd, file_mem, statbuf.st_size);
                free(file_mem);
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

