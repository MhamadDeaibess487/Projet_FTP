/*
 * FTP_serverp.c 
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



erreur_t file_send(int connfd,request_t req) {
    ssize_t n;
    response_t res;
    ssize_t rc;
    char *file_mem = malloc(Block);
    if(!file_mem){
        res.status=M;
        res.block_size = 0;

        rc = rio_writen(connfd, &res, sizeof(response_t));//on envoi l'erreur
        if (rc < 0) {
            printf("errno = %d\n", errno);
            printf("message = %s\n", strerror(errno));
        }
        return M;
    }

    //remplir un buffer le contenu du fichier
    int fp = open(req.filename,O_RDONLY,0);
    if(fp<0){
        
        res.status=O;
        res.block_size = 0;

        rc = rio_writen(connfd, &res, sizeof(response_t));//on envoi l'erreur
        if (rc < 0) {
            printf("errno = %d\n", errno);
            printf("message = %s\n", strerror(errno));
        }
        free(file_mem);
        return O;
    }

    //cas ou le client a crash, on peut continuer l'ecriture dans le fichier a partir de l'endroit ou on s'est arreter (indique dans la struct request)
    if (req.offset > 0) {
        lseek(fp, req.offset, SEEK_SET);
        printf("[Fils %d] Reprise du transfert à %ld octets\n", getpid(), req.offset);
    }


    while((n = read(fp,file_mem,Block))>0){// on envoi la taille du block 
    
        //sleep(2); //pour tester le crash du client
       
        res.status =S;
        res.block_size = n;
        rc = rio_writen(connfd, &res, sizeof(response_t));//on utilise cett version pour qu'on puisse gerer les coupure nous meme du client et pas juste une erreur de connexion
        if(rc < 0) {
            printf("errno = %d\n", errno);
            printf("message = %s\n", strerror(errno));
            free(file_mem);
            Close(fp);
            return C;
        }
        rc = rio_writen(connfd, file_mem, n);
        if (rc < 0) {
            printf("errno = %d\n", errno);
            printf("message = %s\n", strerror(errno));
            free(file_mem);
            Close(fp);
            return C;
        }
    }
    if(n<0){
        res.status=R;
        res.block_size = 0;
        rio_writen(connfd, &res, sizeof(response_t));
        free(file_mem);
        Close(fp);
        return R;
    }
    
    res.status=S;
    res.block_size=0;
    rc = rio_writen(connfd, &res, sizeof(response_t));//on envoi 0 pour indiquer la fin du fichier
    if(rc < 0) {
        printf("errno = %d\n", errno);
        printf("message = %s\n", strerror(errno));
        free(file_mem);
        Close(fp);
        return C;
    }
    Close(fp);
    free(file_mem);
    return 0;
}

erreur_t lister_les_fichiers(int connfd,request_t req) {
    response_t res;
    ssize_t rc;
    FILE *fp;
    char buffer[MAXLINE];
    if(req.options[0] != '\0') {
        if(strcmp(req.options, "-l") == 0) {
            fp = popen("ls -l", "r");
        } else if(strcmp(req.options, "-a") == 0) {
            fp = popen("ls -a", "r");
        }else if(strcmp(req.options, "-la") == 0 || strcmp(req.options, "-al") == 0) {
            fp = popen("ls -la", "r");
        }
    } else {
        fp = popen("ls", "r");
    }
    if (fp == NULL) {
        fprintf(stderr, "Erreur lors de l'exécution de la commande ls\n");
        return M;
    }
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        res.status = S;
        res.block_size = strlen(buffer);
        rc = rio_writen(connfd, &res, sizeof(response_t));
            if (rc < 0) {
                fprintf(stderr, "Erreur lors de l'envoi de la réponse pour ls\n");
                pclose(fp);
                return C;
            }
        if ((rc = rio_writen(connfd, buffer, strlen(buffer))) < 0) {
            fprintf(stderr, "Erreur lors de l'envoi de la liste des fichiers\n");
            pclose(fp);
            return C;
        }
    }
    pclose(fp);
        res.status = S;
        res.block_size = 0; //indique la fin de la liste
        rc = rio_writen(connfd, &res, sizeof(response_t));
            if (rc < 0) {
                fprintf(stderr, "Erreur lors de l'envoi de la réponse finale pour ls\n");
                return C;
            }
    return S;
}


int main(int argc, char **argv)
{
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_in clientaddr;
    char client_ip_string[INET_ADDRSTRLEN];
    char client_hostname[MAX_NAME_LEN];
    int err = 0;//pour les erreurs
    //rio_t *rio;
    //struct stat statbuf;
    Signal(SIGCHLD, sigchld_handler);
    Signal(SIGINT, sigint_handler);
    Signal(SIGPIPE, SIG_IGN); //ignorer le signal de pipe pour gerer les coupure du client nous meme 
    
    pid_t pid;
    
    printf("Server is running ...\n");
    clientlen = (socklen_t)sizeof(clientaddr);

    //verifie si le port de l'eclave est ecrit en argument
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port_slave>\n", argv[0]);
        exit(0);
    }
    int port_slave = atoi(argv[1]);
    listenfd = Open_listenfd(port_slave);
   
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
                    if(req.type == GET){
                       
                        err = file_send(connfd,req);
                        if(err==C){
                            printf("Erreur lors de l'envoi du fichier, le client a peut-être coupé la connexion.\n");
                            break;
                        }
                        if(err==R){
                            printf("Erreur de lecture du fichier, le client a peut-être coupé la connexion.\n");
                            continue;
                        }
                        if(err==M){
                            printf("Erreur d'allocation du buffer, le client a peut-être coupé la connexion.\n");
                            continue;
                        }if(err==O){
                            printf("Erreur : le fichier n'existe pas sur le serveur, le client a peut-être coupé la connexion.\n");
                            continue;
                        }
                    }else if(req.type == PUT){
                        printf("la commande put n'est pas encore implementee\n");
                    }else if(req.type == LS){
                        err = lister_les_fichiers(connfd,req);
                    }

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

