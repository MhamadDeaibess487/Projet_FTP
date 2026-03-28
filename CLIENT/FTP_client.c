#include "FTP_client.h"
#include <stdio.h>
#include <errno.h>
/*
 * echoclient.c - An echo client
 */



void response(int clientfd, char *filename) {
    response_t res;
    double speed=0.0;
    struct timeval start , end;
    char *contenu_f_lu;
    
    
    
    
    ssize_t r;
    size_t total = 0;
    gettimeofday(&start, NULL);//on commence le temps
     r = Rio_readn(clientfd, &res, sizeof(response_t));//on lit la reponse une premiere fois pour gere les erreurs
        if(r<0){
            printf("erreur de connexion au debut\n");
            return;
        }
        printf("je lit du server %d\n",res.block_size);

        if(res.status==-3){
            printf("erreur : probleme d'allocation du buffer\n");
            return;
        }
      
        if(res.status==-4){
            printf("Erreur de lecture du fichier\n");
            return;
        }
        
        if (res.status == -2) {
            printf("Erreur : le fichier '%s' n'existe pas sur le serveur\n", filename);
            return;
        }
        if (r <= 0) {
            printf("erreur de connexion\n");
            printf("la\n");
            return;
        }
        // prepa d'un fichier local pour stocker le contenu du fichier qu'on va lire 
        
        int fd = Open(filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (fd<0) {
            perror("Erreur : ouverture du ficheir en local\n");
            return;
        }

        // on prends un block
        contenu_f_lu = malloc(Block);
        if (!contenu_f_lu) {
            Close(fd);
            return;
        }
        
    while (1) {
        if (res.block_size == 0) {//fichier complet d'apres le protocol
            break;
        }

        r = Rio_readn(clientfd, contenu_f_lu, res.block_size);//contenue du fichier
        if (r <= 0) {//probleme
            printf("erreur de connexion\n");
            break;
        }
        Write(fd, contenu_f_lu, res.block_size);//ecrire dasn le fichier local
        total += res.block_size;//calcul pour la taille
        r = Rio_readn(clientfd, &res, sizeof(response_t));//continuer a lire la reponse du serveur pour bien gerer
        if (r <= 0) {
        printf("erreur de connexion\n");
        break;
    }
    }
    
    gettimeofday(&end, NULL);

    if (r > 0) {
        
        
        //on calcule en sec et nse  alors on les converti tous en sec
        double t_ecoule = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
        if(t_ecoule>0){
            
            speed = (total / 1024.0) / t_ecoule;
        }
        
        printf("Transfer successful\n");
        printf("%ld bytes received in %f seconds (%f Kbytes/s).\n", (long)total, t_ecoule, speed);

    }

    free(contenu_f_lu);
    Close(fd);
}




int main(int argc, char **argv)
{
    int clientfd;
    char *host, buf[MAXLINE];
    rio_t rio;

    if(argc!=2){
        printf("pas le bon nb d'arguments\n");
        exit(1);
    }
    host = argv[1];


    /*
     * Note that the 'host' can be a name or an IP address.
     * If necessary, Open_clientfd will perform the name resolution
     * to obtain the IP address.
     */
    clientfd = Open_clientfd(host, PORT);
    
    /*
     * At this stage, the connection is established between the client
     * and the server OS ... but it is possible that the server application
     * has not yet called "Accept" for this connection
     */
    printf("client connected to server OS\n"); 
    
    Rio_readinitb(&rio, clientfd);
    int nb;
    printf("ftp> ");
    while (Fgets(buf, MAXLINE, stdin) != NULL) {
        
        request_t req;
        char type[10];
        char filename[256];
        nb =sscanf(buf,"%s %s", type, filename);

        if (nb<=0){
            printf("ftp> ");
            continue;
        }
        else if(strcmp(type , "bye") == 0) {
            break;
        }
    
        else if (strcmp(type , "get") == 0) {
            if(nb != 2){
                printf("manque le nom des fichiers\n");
                printf("ftp> ");
                continue;
            }
            req.type = GET; 
            strncpy(req.filename, filename, 256);
            req.filename[255] = '\0';

            struct stat st;
            //le cas ou on a deja ce fichier dans le repertoire client
            if (stat(filename, &st) == 0) {
                //on change dans la struct pour mettre la valeur de la size actuel du fichier dans offset
                req.offset = st.st_size; 
                printf("Fichier existant  interronpu (%ld octets) : reprise du transfert...\n", req.offset);
            } else {
                //on met offset a 0 si il y a pas le fichier deja pour faire un transfert normal
                req.offset = 0; 
            }


            Rio_writen(clientfd, &req, sizeof(request_t));
            response(clientfd, filename);
           
        } else {
            printf("commande inconnue\n");
        }
        
        printf("ftp> ");

    }
    Close(clientfd);
    printf("end of connection...\n");
    exit(0);
}

