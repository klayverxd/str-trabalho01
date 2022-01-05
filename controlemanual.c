/*  Programa de demonstracao de uso de sockets UDP em C no Linux
 *  Funcionamento:
 *  Usuario escolhe opcao no menu e entao envia uma msg para a caldeira.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <netdb.h>

#define FALHA 1


int cria_socket_local(void)
{
	int socket_local;		/* Socket usado na comunicacao */

	socket_local = socket( PF_INET, SOCK_DGRAM, 0);
	if (socket_local < 0) {
		perror("socket");
		return -1;
	}
	return socket_local;
}

struct sockaddr_in cria_endereco_destino(char *destino, int porta_destino)
{
	struct sockaddr_in servidor;	/* Endereco do servidor incluindo ip e porta */
	struct hostent *dest_internet;	/* Endereco destino em formato proprio */
	struct in_addr dest_ip;			/* Endereco destino em formato ip numerico */

	if (inet_aton ( destino, &dest_ip ))
		dest_internet = gethostbyaddr((char *)&dest_ip, sizeof(dest_ip), AF_INET);
	else
		dest_internet = gethostbyname(destino);

	if (dest_internet == NULL) {
		fprintf(stderr,"Endereco de rede invalido\n");
		exit(FALHA);
	}

	memset((char *) &servidor, 0, sizeof(servidor));
	memcpy(&servidor.sin_addr, dest_internet->h_addr_list[0], sizeof(servidor.sin_addr));
	servidor.sin_family = AF_INET;
	servidor.sin_port = htons(porta_destino);

	return servidor;
}


void envia_mensagem(int socket_local, struct sockaddr_in endereco_destino, char *mensagem)
{
	/* Envia msg ao servidor */

	if (sendto(socket_local, mensagem, strlen(mensagem)+1, 0, (struct sockaddr *) &endereco_destino, sizeof(endereco_destino)) < 0 )
	{ 
		perror("sendto");
		return;
	}
}


int recebe_mensagem(int socket_local, char *buffer, int TAM_BUFFER)
{
	int bytes_recebidos;		/* Numero de bytes recebidos */

	/* Espera pela msg de resposta do servidor */
	bytes_recebidos = recvfrom(socket_local, buffer, TAM_BUFFER, 0, NULL, 0);
	if (bytes_recebidos < 0)
	{
		perror("recvfrom");
	}

	return bytes_recebidos;
}

int main(int argc, char *argv[])
{
	if (argc < 3) { 
		fprintf(stderr,"Uso: controlemanual <endereco> <porta>\n");
		fprintf(stderr,"<endereco> eh o endereco IP da caldeira\n");
		fprintf(stderr,"<porta> eh o numero da porta UDP da caldeira\n");
		fprintf(stderr,"Exemplo de uso:\n");
		fprintf(stderr,"   controlemanual localhost 12345\n");
		exit(FALHA);
	}

	int porta_destino = atoi( argv[2]);

	int socket_local = cria_socket_local();

	struct sockaddr_in endereco_destino = cria_endereco_destino(argv[1], porta_destino);


	char opcao;
	do{
		char teclado[1000];
		double valor;
		char msg_enviada[1000];  
		char msg_recebida[1000];
		int nrec;

		printf("\n");   //\\Digite a letra da opcao seguida pelo valor, no caso de atuadores:);
		printf("<x> Termina o programa\n");
		printf("<a> Lê valor de Ta\n");
		printf("<t> Lê valor de T\n");
		printf("<i> Lê valor de Ti\n");
		printf("<o> Lê valor de No\n");
		printf("<h> Lê valor de H\n");
		printf("<I><valor> Define valor de Ni\n");
		printf("<Q><valor> Define valor de Q\n");
		printf("<A><valor> Define valor de Na\n");
		printf("<F><valor> Define valor de Nf\n"); 
		printf("Digite a letra da opcao seguida pelo valor, no caso de atuadores:\n");

		fgets( teclado, 1000, stdin);
		opcao = teclado[0];
		switch( opcao ) {
			case 'x':	exit(0);
			case 'a':	strcpy( msg_enviada, "sta0");
						break;
			case 't':	strcpy( msg_enviada, "st-0");
						break;
			case 'i':	strcpy( msg_enviada, "sti0");
						break;
			case 'o':	strcpy( msg_enviada, "sno0");
						break;
			case 'h':	strcpy( msg_enviada, "sh-0");
						break;
			case 'I':	valor = atof( &teclado[1] );
						sprintf( msg_enviada, "ani%lf", valor);
						break;
			case 'Q':	valor = atof( &teclado[1] );
						sprintf( msg_enviada, "aq-%lf", valor);
						break;
			case 'A':	valor = atof( &teclado[1] );
						sprintf( msg_enviada, "ana%lf", valor);
						break;
			case 'F':	valor = atof( &teclado[1] );
						sprintf( msg_enviada, "anf%lf", valor); 
						break;
			default:	printf("Opcao %c nao existe.\n", opcao);
						continue;
			}

		printf("Enviado: %s\n", msg_enviada);
		envia_mensagem(socket_local, endereco_destino, msg_enviada);

		nrec = recebe_mensagem(socket_local, msg_recebida, 1000);
		msg_recebida[nrec] = '\0';
		printf("Mensagem de resposta com %d bytes >>>%s<<<\n", nrec, msg_recebida);

	} while( opcao != 'x' );

}
