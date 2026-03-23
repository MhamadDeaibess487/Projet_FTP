#include "FTP_client.h"
#include <stdio.h>

/*
 * echoclient.c - An echo client
 */


int main(int argc, char **argv)
{
    int clientfd;
    char *host, buf[MAXLINE];
    rio_t rio;

    
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

    printf("ftp> ");
    while (Fgets(buf, MAXLINE, stdin) != NULL) {
        request_t req;
        char type[10];
        char filename[256];

        if (sscanf(buf,"%s %s", type, filename) != 2) 
            continue;

        if (strcmp(type , "bye") == 0) {
            printf("end of connection...\n"); // Ajout du ; et \n
            exit(0);
        }

        if (strcmp(type , "get") == 0) {
            req.type = GET; 
            strncpy(req.filename, filename, 256);
            
            Rio_writen(clientfd, &req, sizeof(request_t));
            response(clientfd, filename);
        } else {
            printf("commande inconnue");
        }
                
        
           

    }
    Close(clientfd);
    printf("end of connection...");
    exit(0);
}


void response(int clientfd, char *filename) {
    response_t res;
    time_t start, end;
    
    // on recoit la reponse 
    if (Rio_readn(clientfd, &res, sizeof(response_t)) <= 0) {
        printf("Error: Connection lost\n");
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

    start = time(NULL);

    // on lit le contenu du fichier envoye par le serveur
    ssize_t n = Rio_readn(clientfd, contenu_f_lu, res.file_size);
    
    end = time (NULL);

    if (n > 0) {
        // on ecrit dans le fichier qu'on a deja preparer le contuenu du fichier lu
        fwrite(contenu_f_lu, 1, n, fp);
        
        double t_ecoule = end - start;
        if (t_ecoule == 0)
            t_ecoule = 1.0; 
        double speed = (n / 1024.0) / t_ecoule;

        printf("Transfer successful\n");
        printf("%ld bytes received in %.4f seconds (%.2f Kbytes/s).\n", (long)n, t_ecoule, speed);
    }

    free(contenu_f_lu);
    fclose(fp);
}