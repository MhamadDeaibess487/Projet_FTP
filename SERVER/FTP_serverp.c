/*
 * echoserveri.c - An iterative echo server
 */

#include "../csapp.h"
#include "FTP_serverp.h"

#define MAX_NAME_LEN 256

void echo(int connfd);

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
int file_send(size_t size,int connfd){
    char buf[size];
    for(size_t i=0;i<size;i++){
       
        
        Rio_writen(connfd, buf, MAXBUF);
        
    }
    return 0;

}





int main(int argc, char **argv)
{
    int listenfd, connfd, port;
    socklen_t clientlen;
    struct sockaddr_in clientaddr;
    char client_ip_string[INET_ADDRSTRLEN];
    char client_hostname[MAX_NAME_LEN];
    rio_t rio;
    //struct stat statbuf;
    Signal(SIGCHLD, sigchld_handler);
    Signal(SIGINT, sigint_handler);
    
    pid_t pid;
    
    
    port = PORT;
    printf("Server is running ...\n");
    clientlen = (socklen_t)sizeof(clientaddr);

    listenfd = Open_listenfd(port);
   
   for(int i = 0; i < NPROC; i++){
        
        pid = Fork();
        
        if(pid == 0){
            while (1) {
                clientlen = sizeof(clientaddr);
                connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
                Rio_readinitb(&rio, connfd);
                char buf[MAXLINE];
                ssize_t n = Rio_readlineb(&rio, buf, MAXLINE);
                if (n > 0) {
                    buf[n] = '\0';
                    printf("Le client a envoyé : %s\n", buf);
                }
                FILE *fp = fopen(buf, "r");
                if (fp == NULL) {
                    fprintf(stderr, "Erreur : impossible d'ouvrir le fichier %s\n", buf);
                    Close(connfd);
                    continue;
                }
                file_send(MAXBUF,connfd);//pas complete
                fclose(fp);
                                
                
                /* determine the name of the client */
                Getnameinfo((SA *) &clientaddr, clientlen,
                            client_hostname, MAX_NAME_LEN, 0, 0, 0);
                
                /* determine the textual representation of the client's IP address */
                Inet_ntop(AF_INET, &clientaddr.sin_addr, client_ip_string,
                        INET_ADDRSTRLEN);
                
                printf("server connected to %s (%s)\n", client_hostname,
                    client_ip_string);
                
                
               
                
                Close(connfd);
                
                
            }
        }else{
            tab[i] = pid;
        }
    }
    while (1) {
        pause();
    }
    
}

