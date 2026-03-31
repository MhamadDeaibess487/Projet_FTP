#include "FTP_client.h"

/*
 * FTP_client.c
 */


redirect_t  connection_maitre(char *host) {//on revinet au mettre pour se reconnecter
     printf("Connexion au Serveur Maitre (%s:%d)...\n", host, PORT_MASTER);
    int masterfd = Open_clientfd(host, PORT_MASTER);
    if (masterfd < 0) {
        printf("Erreur : connection avec le serveur maitre impossible.\n");
        exit(1);
    }
    
    //reception de l'information de redirection
    redirect_t redir;

    if (Rio_readn(masterfd, &redir, sizeof(redirect_t)) != sizeof(redirect_t)) {
        printf("\nLe Serveur Maitre a rejete la connexion.\n");
        printf("Raison probable : Tous les esclaves sont offline. Veuillez reessayer plus tard.\n\n");
        Close(masterfd);
        exit(1); 
    }
    
    //fermeture de la connection avec le maitre
    Close(masterfd);
    return redir;
}

//fonction pour se reconnecter a un autre esclave en cas de panne de celui avec lequel on etait connecte
int reconnect_to_slave(int *clientfd, char *host, char *filename, request_t *req, response_t *res) { 
    printf("Erreur de connexion avec le serveur esclave, tentative de reconnexion...\n");

    redirect_t redir = connection_maitre(host);
    printf("Redirection vers l'esclave %s:%d en cours...\n", redir.ip, redir.port);

    if (*clientfd >= 0) {
        Close(*clientfd);
    }

    *clientfd = Open_clientfd(redir.ip, redir.port);
    if (*clientfd < 0) {
        printf("Erreur : connection impossible avec le serveur esclave sur %d.\n", redir.port);
        return -1;
    }

    if (req->type == GET) {
    struct stat st;
    if (stat(filename, &st) == 0) {
        req->offset = st.st_size;
    } else {
        req->offset = 0;
    }
    } else {
        req->offset = 0;
    }

    if (rio_writen(*clientfd, req, sizeof(request_t)) < 0) {
        printf("Erreur lors de l'envoi de la requete au nouvel esclave.\n");
        return -1;
    }

    if (rio_readn(*clientfd, res, sizeof(response_t)) <= 0) {
        printf("Erreur lors de la lecture de la reponse du nouvel esclave.\n");
        return -1;
    }

    return 0;
}



void response(int *clientfd, char *filename,char *host,request_t req) {
    response_t res;
    double speed=0.0;
    struct timeval start , end;
    char *contenu_f_lu;
    
    
    int essais = 0;
    ssize_t r;
    size_t total = 0;
    gettimeofday(&start, NULL);//on commence le temps
    r = rio_readn(*clientfd, &res, sizeof(response_t));//on lit la reponse une premiere fois pour gere les erreurs
        
        while (r <= 0 && essais < NB_SLAVES) {
            if (reconnect_to_slave(clientfd, host, filename, &req, &res) == 0) {
                break;
            }
            essais++;
        }

        if (essais == NB_SLAVES) {
            printf("Impossible de trouver un esclave disponible.\n");
            
            return;
        }
        
    
        if(res.status==M){
            printf("erreur : probleme d'allocation du buffer\n");
            return;
        }
      
        if(res.status==R){
            printf("Erreur de lecture du fichier\n");
            return;
        }
        
        if (res.status == O) {
            printf("Erreur : le fichier '%s' n'existe pas sur le serveur\n", filename);
            return;
        }
        //cas ou le client a crash pendant le transfert, on peut continuer a partir de la ou on s'est arreter grace a l'offset
        essais = 0;
        while (res.status==C && essais < NB_SLAVES) {
            if (reconnect_to_slave(clientfd, host, filename, &req, &res) == 0) {
                break;
            }
            essais++;
        }

        if (essais == NB_SLAVES) {
            printf("Impossible de trouver un esclave disponible.\n");
            
            return;
        }
        if(req.type==GET){
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
            
        while (res.block_size > 0) {
            

            r = Rio_readn(*clientfd, contenu_f_lu, res.block_size);//contenue du fichier
            essais = 0;
            while (r <= 0 && essais < NB_SLAVES) {
                if (reconnect_to_slave(clientfd, host, filename, &req, &res) == 0) {
                    r = Rio_readn(*clientfd, contenu_f_lu, res.block_size);
                    if (r > 0) {
                        break;
                    }
                }
                essais++;
            }
            if(essais == NB_SLAVES) {
                printf("Impossible de trouver un esclave disponible.\n");
                free(contenu_f_lu);
                Close(fd);
                return;
            }
            Write(fd, contenu_f_lu, res.block_size);//ecrire dasn le fichier local
            total += res.block_size;//calcul pour la taille
            r = Rio_readn(*clientfd, &res, sizeof(response_t));//continuer a lire la reponse du serveur pour bien gerer
            essais = 0;
            while (r <= 0 && essais < NB_SLAVES) {
                if (reconnect_to_slave(clientfd, host, filename, &req, &res) == 0) {
                    r = 1; // on suppose que la lecture de la reponse a reussi apres la reconnexion, on va lire le contenu du fichier dans la prochaine iteration
                    break;
                }
                essais++;
            }
            if(essais == NB_SLAVES) {
                printf("Impossible de trouver un esclave disponible.\n");
                free(contenu_f_lu);
                Close(fd);
                return;
            }
        }
        
        gettimeofday(&end, NULL);

        if (r > 0) {
            
            
            //on calcule en sec et nse  alors on les converti tous en sec
            double t_ecoule = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
            if(t_ecoule>0){
                
                speed = (total / 1024.0) / t_ecoule; // vitesse en Kbytes/s
            }
            
            printf("Transfer successful\n");
            printf("%ld bytes received in %f seconds (%f Kbytes/s).\n", (long)total, t_ecoule, speed);

        }

        free(contenu_f_lu);
        Close(fd);
    }
    else if(req.type == LS){
        char ls_output[MAXLINE + 1];

        printf("Contenu du répertoire sur le serveur :\n");

        while (res.block_size > 0) {
            r = Rio_readn(*clientfd, ls_output, res.block_size);
            essais = 0;
            while (r <= 0 && essais < NB_SLAVES) {
                if (reconnect_to_slave(clientfd, host, filename, &req, &res) == 0) {
                    r = Rio_readn(*clientfd, ls_output, res.block_size);
                    if (r > 0) {
                        break;
                    }
                }
                essais++;
            }

            if (essais == NB_SLAVES) {
                printf("Impossible de trouver un esclave disponible.\n");
                return;
            }

            ls_output[res.block_size] = '\0';
            printf("%s", ls_output);

            r = Rio_readn(*clientfd, &res, sizeof(response_t));
            essais = 0;
            while (r <= 0 && essais < NB_SLAVES) {
                if (reconnect_to_slave(clientfd, host, filename, &req, &res) == 0) {
                    r = 1;
                    break;
                }
                essais++;
            }

            if (essais == NB_SLAVES) {
                printf("Impossible de trouver un esclave disponible.\n");
                return;
            }
        }
    }else if(req.type == PUT){
        
        if(res.status == S) {
            printf("la commande put a etait execute \n");
        }

    }
    
}



int main(int argc, char **argv){
    int clientfd;
    char *host, buf[MAXLINE];
    rio_t rio;
    
    if(argc!=2){
        printf("pas le bon nb d'arguments\n");
        exit(1);
    }
    host = argv[1];
    ssize_t r;//pour lw rio_writen 

    redirect_t redir = connection_maitre(host);
    printf("Redirection vers l'esclave %s:%d en cours...\n", redir.ip, redir.port);

    //connection avec l'esclave
    clientfd = Open_clientfd(redir.ip, redir.port);
    if (clientfd < 0) {
        printf("Erreur : connection impossible avec le serveur esclave sur %d.\n", redir.port);
        exit(1);
    }

    printf("client connected to slave server OS\n");    
    
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


            r=rio_writen(clientfd, &req, sizeof(request_t));//envoi de la requete au serveur esclave
            int essais = 0;
            while(r<0 && essais < NB_SLAVES) {
                printf("Erreur lors de l'envoi de la requete au serveur esclave, tentative de reconnexion...\n");
                redir=connection_maitre(host);
                printf("Redirection vers l'esclave %s:%d en cours...\n", redir.ip, redir.port);
                Close(clientfd);
                clientfd = Open_clientfd(redir.ip, redir.port);
                if (clientfd < 0) {
                    printf("Erreur : connection impossible avec le serveur esclave sur %d.\n", redir.port);
                }else{
                    r = rio_writen(clientfd, &req, sizeof(request_t));
                }
                essais++;
            }
            if (r < 0 ) {
                printf("Impossible de contacter un esclave disponible.\n");
                continue;
            }
             response(&clientfd, filename,host,req);//traitement de la reponse du serveur esclave
            
           
        }else if(strcmp(type , "ls") == 0) {
            // Gestion de la commande ls
            req.type = LS; 
            nb =sscanf(buf,"%s %s", type, filename);
            strncpy(req.options, filename, 256);
            req.options[255] = '\0';
            req.filename[0] = '\0';
            req.offset = 0;
            r = rio_writen(clientfd, &req, sizeof(request_t));
            int essais = 0;
            while (r < 0 && essais < NB_SLAVES) {
                printf("Erreur lors de l'envoi de la requete au serveur esclave, tentative de reconnexion...\n");
                redir = connection_maitre(host);
                printf("Redirection vers l'esclave %s:%d en cours...\n", redir.ip, redir.port);
                Close(clientfd);
                clientfd = Open_clientfd(redir.ip, redir.port);
                if (clientfd < 0) {
                    printf("Erreur : connection impossible avec le serveur esclave sur %d.\n", redir.port);
                } else {
                    r = rio_writen(clientfd, &req, sizeof(request_t));
                }
                essais++;
            }
            if (r < 0) {
                printf("Impossible de contacter un esclave disponible.\n");
                continue;
            }
            req.filename[0] = '\0';
            req.offset = 0;
            filename[0] = '\0';
            req.options[0] = '\0';
            response(&clientfd, filename,host,req);//traitement de la reponse du serveur esclave
        }
        else if (strcmp(type , "put") == 0) {
            if(nb != 2){
                printf("manque le nom des fichiers\n");
                printf("ftp> ");
                continue;
            }
            req.type = PUT; 
            strncpy(req.filename, filename, 256);
            int fp = open(filename, O_RDONLY, 0644);
            if (fp < 0) {
                printf("Erreur lors de l'ouverture du fichier pour PUT\n");
                continue;
            }
            req.filename[255] = '\0';
            req.offset = 0; 
            req.options[0] = '\0';
            rio_writen(clientfd, &req, sizeof(request_t));
            
            char contenue_fichier[Block];
            response_t res;
            ssize_t n;
            
            while((n= read(fp, contenue_fichier, Block)) > 0) {
                
                res.status = S;
                res.block_size = n;
                if (rio_writen(clientfd, &res, sizeof(response_t)) < 0) {
                    printf("Erreur lors de l'envoi de la reponse au serveur esclave.\n");
                    break;
                }
                if (rio_writen(clientfd, contenue_fichier, res.block_size) < 0) {
                    printf("Erreur lors de l'envoi du contenu du fichier au serveur esclave.\n");
                    break;
                }
            }
            Close(fp);
            res.status = S;
            res.block_size = 0;
            rio_writen(clientfd, &res, sizeof(response_t)); // Indique la fin du fichier
            response(&clientfd, filename,host,req);//traitement de la reponse du serveur esclave
        }
        else if(strcmp(type , "rm") == 0) {
             if(nb != 2){
                printf("manque le nom des fichiers\n");
                printf("ftp> ");
                continue;
            }
            req.type = RM; 
            strncpy(req.filename, filename, 256);
            response_t res;
          
            req.filename[255] = '\0';
            req.offset = 0; 
            req.options[0] = '\0';

              res.status = S;
             if (rio_writen(clientfd, &req, sizeof(request_t)) < 0) {
                printf("Erreur lors de l'envoi de la requete au serveur esclave.\n");
                continue;
            }
             if (rio_readn(clientfd, &res, sizeof(response_t)) <= 0) {
                printf("Erreur lors de la lecture de la reponse du serveur esclave.\n");
                continue;
            }
             if(res.status == S) {
                printf("la commande rm a etait execute \n");
             }
        }
        else {
            printf("commande inconnue\n");
        }
       
        
        printf("ftp> ");

    }
    Close(clientfd);
    printf("end of connection...\n");
    exit(0);
}

