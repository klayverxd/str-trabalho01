#ifndef SOCKET_H
#define SOCKET_H

static int cria_socket_local (void);
static struct sockaddr_in cria_endereco_destino(char *destino, int porta_destino);
static void envia_mensagem(int socket_local, struct sockaddr_in endereco_destino, char *mensagem);
static int recebe_mensagem(int socket_local, char *buffer, int TAM_BUFFER);
void cria_socket (char *destino, int porta_destino);
double msg_socket(char *msg);

#endif
