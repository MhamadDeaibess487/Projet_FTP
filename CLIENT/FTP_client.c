#include "FTP_client.h"
#include <stdio.h>

/*
 * echoclient.c - An echo client
 */



void response(int clientfd, char *filename) {
    response_t res;
    double speed;
    struct timespec start, end;

    // on recoit la reponse 
    if (Rio_readn(clientfd, &res, sizeof(response_t)) <= 0) {
        printf("Error: Connection lost\n");
        return;
    }

    if(res.status==-1){
        printf("erreur : probleme dans le repere des stat du fichiers\n");
        return;
    }

    if(res.status==-3){
        printf("erreur : probleme d'allocation du buffer\n");
        return;
    }
    if(res.status==-5){
        printf("Erreur d'ouverture du fichier\n");
        return;
    }
     if(res.status==-4){
        printf("Erreur de lecture du fichier\n");
        return;
    }


    // verifie le status
    if (res.status < 0) {
        printf("Erreur : le fichier '%s' n'existe pas sur le serveur\n", filename);
        return;
    }

    
    // prepa d'un fichier local pour stocker le contenu du fichier qu'on va lire 
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        perror("Error opening local file");
        return;
    }

    // on prends en une fois l'integralite du contenu du fichier -> on l emet dans un buffer
    char *contenu_f_lu = malloc(res.file_size);
    if (!contenu_f_lu) {
        fclose(fp);
        return;
    }
    

    clock_gettime(CLOCK_MONOTONIC, &start);
    // on lit le contenu du fichier envoye par le serveur
    ssize_t n = Rio_readn(clientfd, contenu_f_lu, res.file_size);
    
    clock_gettime(CLOCK_MONOTONIC, &end);

    if (n > 0) {
        // on ecrit dans le fichier qu'on a deja preparer le contuenu du fichier lu
        fwrite(contenu_f_lu, 1, n, fp);
        //on calcule en sec et nse  alors on les converti tous en sec
        double t_ecoule = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1000000000.0;
        if(t_ecoule>0){
            
            speed = (n / 1024.0) / t_ecoule;
        }
        
        printf("Transfer successful\n");
        printf("%ld bytes received in %f seconds (%f Kbytes/s).\n", (long)n, t_ecoule, speed);
    }

    free(contenu_f_lu);
    fclose(fp);
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
            
            Rio_writen(clientfd, &req, sizeof(request_t));
            response(clientfd, filename);
        } else {
            printf("commande inconnue");
        }
        
        printf("ftp> ");

    }
    Close(clientfd);
    printf("end of connection...\n");
    exit(0);
}

