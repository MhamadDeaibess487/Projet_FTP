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
        else{
            fp = popen("ls", "r");
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


erreur_t ecrire_dans_fichier(char *buffer, int taille, int fp) {
    if (write(fp, buffer, taille) < 0) {
        return R;
    }
    return S;
}

erreur_t propager_vers_esclave(request_t req,int port_courant) {
    int connfd;
    response_t res;
    for(int i = 0; i < NB_SLAVES; i++) {
        if(SLAVES[i].port == port_courant) {
            continue; //ne pas propager vers soi meme
        }
        connfd = open_clientfd(SLAVES[i].ip, SLAVES[i].port);
        if (connfd >= 0) {
            if (rio_writen(connfd, &req, sizeof(request_t)) < 0) {//envoi al req
                printf("Erreur lors de l'envoi de la requete au nouvel esclave.\n");
                Close(connfd);
                continue;
            }
            file_send(connfd, req);//envoi le fichier
            if (rio_readn(connfd, &res, sizeof(response_t)) <= 0) {//recoit la reponse
                printf("Erreur lors de la lecture de la reponse du nouvel esclave.\n");
                Close(connfd);
                continue;
            }
            
            Close(connfd);
            return res.status;
        } else {
            printf("Erreur de connection : esclave %d en panne\n", i);
        }
    }
        return C; //si tous les esclaves sont en panne
}

erreur_t effacer_fichier(request_t req,response_t *res) {

    char command[MAXLINE + 10];
    command[0] = '\0';
    strcat(command, "rm ");
    strcat(command, req.filename);
    FILE *fp = popen(command, "r");
    if (fp == NULL) {
        res->status = R;
    } else {
        int ret = pclose(fp);
        if (ret == 0) {
            res->status = S;
        } else {
            res->status = R;
        }
        res->block_size = 0;
        
    }
    return res->status;
}

erreur_t propager_vers_esclave_rm(request_t req,int port_courant) {
    int connfd;
    response_t res;
    for(int i = 0; i < NB_SLAVES; i++) {
        if(SLAVES[i].port == port_courant) {
            continue; //ne pas propager vers soi meme
        }
        connfd = open_clientfd(SLAVES[i].ip, SLAVES[i].port);
        if (connfd >= 0) {
            if (rio_writen(connfd, &req, sizeof(request_t)) < 0) {//envoi al req
                printf("Erreur lors de l'envoi de la requete au nouvel esclave.\n");
                Close(connfd);
                continue;
            }
            
            if (rio_readn(connfd, &res, sizeof(response_t)) <= 0) {//recoit la reponse
                printf("Erreur lors de la lecture de la reponse du nouvel esclave.\n");
                Close(connfd);
                continue;
            }
            
            Close(connfd);
            return res.status;
        } else {
            printf("Erreur de connection : esclave %d en panne\n", i);
        }
    }
        return C; //si tous les esclaves sont en panne
}


int main(int argc, char **argv){
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
    for (int i = 0; i < NB_SLAVES; i++) {
        strcpy(SLAVES[i].ip, "127.0.0.1");
        SLAVES[i].port = PORT_SLAVE_BASE + i;
    }
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
                    err = 0;
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
                        
                       
                        response_t res;
                        char file_mem[Block];

                        int fp = open(req.filename, O_WRONLY | O_CREAT| O_TRUNC, 0644);
                        if (fp < 0) {
                            printf("Erreur lors de l'ouverture du fichier pour PUT\n");
                            continue;
                        }

                        while (Rio_readn(connfd, &res, sizeof(response_t)) > 0) {
                            if (res.block_size == 0) {
                                break;
                            }

                            if (Rio_readn(connfd, file_mem, res.block_size) <= 0) {
                                printf("Erreur lors de la réception du contenu du fichier\n");
                                err = C;
                                break;
                            }

                            if (write(fp, file_mem, res.block_size) < 0) {
                                printf("Erreur lors de l'écriture locale du fichier\n");
                                err = R;
                                break;
                            }
                        }

                        close(fp);

                        if (err == C) {
                            printf("Erreur lors de la réception du fichier, le client a peut-être coupé la connexion.\n");
                            continue;
                        }
                        res.status = S;
                        res.block_size = 0;
                        rio_writen(connfd, &res, sizeof(response_t));
                        
                        req.type = INT_PUT; //on change le type de la requete pour que les autres esclaves ne fassent pas de put dans le fichier mais juste une ecriture locale pour gerer la reprise d'un put apres un crash du client
                        err = propager_vers_esclave(req,port_slave);
                        if(err==C){
                            printf("Erreur lors de la propagation du fichier vers les esclaves, tous les esclaves sont peut-être en panne.\n");
                            continue;;
                        }
                    }
                    else if(req.type == INT_PUT){
                        response_t res;
                        char file_mem[Block];

                        int fp = open(req.filename, O_WRONLY | O_CREAT| O_TRUNC, 0644);
                        if (fp < 0) {
                            printf("Erreur lors de l'ouverture du fichier pour PUT\n");
                            continue;
                        }

                        while (Rio_readn(connfd, &res, sizeof(response_t)) > 0) {
                            if (res.block_size == 0) {
                                break;
                            }

                            if (Rio_readn(connfd, file_mem, res.block_size) <= 0) {
                                printf("Erreur lors de la réception du contenu du fichier\n");
                                err = C;
                                break;
                            }

                            if (write(fp, file_mem, res.block_size) < 0) {
                                printf("Erreur lors de l'écriture locale du fichier\n");
                                err = R;
                                break;
                            }
                        }

                        close(fp);

                        if (err == C) {
                            printf("Erreur lors de la réception du fichier, le client a peut-être coupé la connexion.\n");
                            continue;
                        }
                        res.status = S;
                        res.block_size = 0;
                        rio_writen(connfd, &res, sizeof(response_t));
                        continue;; //on ne propage pas vers les esclaves car c'est une requete interne pour gerer la reprise d'un put apres un crash du client
                    }
                    else if(req.type == LS){
                        err = lister_les_fichiers(connfd,req);
                    }
                    else if(req.type == RM){
                    response_t res;
                    err = effacer_fichier(req, &res);
                    rio_writen(connfd, &res, sizeof(response_t)); // toujours envoyer
                    if (err == S) {
                        req.type = INT_RM;
                        propager_vers_esclave_rm(req, port_slave);
                    }
                }
                    else if(req.type == INT_RM){
                        response_t res;
                    
                        err = effacer_fichier(req, &res);
                        
                        rio_writen(connfd, &res, sizeof(response_t));
                        
                        continue;; //on ne propage pas vers les esclaves car c'est une requete
                    }
                    else{
                        printf("Type de requete inconnu\n");
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

