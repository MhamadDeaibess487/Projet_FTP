#include "FTP_master.h"

int main() {
    int listenfd, connfd;
    socklen_t clientlen = sizeof(struct sockaddr_in);
    struct sockaddr_in clientaddr;
    slave_t slaves[NB_SLAVES];
    int next_slave = 0; //incremente pour le balancer

    printf("Serveur Maître Demarre\n");

  

    //connection au esclaves
    for (int i = 0; i < NB_SLAVES; i++) {
        strcpy(slaves[i].ip, "127.0.0.1");
        slaves[i].port = PORT_SLAVE_BASE + i;
        slaves[i].pid = -1; 
        
        printf("Connexion a l'esclave %d sur %s:%d... \n", i, slaves[i].ip, slaves[i].port);
        fflush(stdout);
        
        slaves[i].socket_fd = open_clientfd(slaves[i].ip, slaves[i].port);
        
        if (slaves[i].socket_fd >= 0) {
            slaves[i].fonctionne = 1;
            printf("Esclave %d connecte : Socket FD: %d)\n", i, slaves[i].socket_fd);
        } else {
            slaves[i].fonctionne = 0;
            printf("Erreur de connection : esclave en panne\n");
        }
    }


    listenfd = Open_listenfd(PORT_MASTER);
    printf("Attente des clients sur le port %d...\n", PORT_MASTER);

    //boucle clients
    while (1) {
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        printf("Nouveau client connecté !\n");
        
        int esclave_trouve = 0;
        int slaves_offline = 0; 

        while (slaves_offline < NB_SLAVES) { 
            //si l'esclave est offline on reessaye de se connecte de nouveau
            if (slaves[next_slave].fonctionne == 0) {
                slaves[next_slave].socket_fd = open_clientfd(slaves[next_slave].ip, slaves[next_slave].port);
                
                if (slaves[next_slave].socket_fd >= 0) {
                    slaves[next_slave].fonctionne = 1;
                    printf("L'esclave %d est de retour en ligne ! (Socket: %d)\n", next_slave, slaves[next_slave].socket_fd);
                    esclave_trouve = 1;
                    break; 
                }
            } 

            else {
                esclave_trouve = 1;
                break; 
            }

            //cas ou l'esclave ne s'eat par reallumer
            next_slave = (next_slave + 1) % NB_SLAVES; 
            slaves_offline++; 
        }
        
        //cas ou tous les esclaves sont en panne 
        if (esclave_trouve == 0) {
            printf("Tous les esclaves sont offline. Le Maître reste en attente...\n");
            Close(connfd);
            continue; //maitre reste allume mais client est rejette avoir ete informe
        }

        //redirection
        redirect_t redir;
        strcpy(redir.ip, slaves[next_slave].ip);
        redir.port = slaves[next_slave].port;
        
        printf("Redirection du client vers l'esclave %d (%s:%d)\n", next_slave, redir.ip, redir.port);
        
        
        Rio_writen(connfd, &redir, sizeof(redirect_t));
        //ferme la connection avec le client qui lui se connectera a l'esclave
        Close(connfd);
        
        //pour gerer la repartition des client au esclaves -> balencer
        next_slave = (next_slave + 1) % NB_SLAVES;
    }
    
    return 0;
}