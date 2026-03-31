#ifndef FTP_CLIENT_H
#define FTP_CLIENT_H
#include "../protocol_commun.h"

void response(int *clientfd, char *filename,char *host,request_t req);
redirect_t connection_maitre(char *host);
#endif