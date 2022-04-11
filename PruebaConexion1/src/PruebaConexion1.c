/*
 ============================================================================
 Name        : PruebaConexion1.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <commons/log.h>
#include <netdb.h>
#include <string.h>
#include <fcntl.h> // for open
#include <unistd.h> // for close



int iniciarServidor(){

	struct sockaddr_in direccionServer;
	direccionServer.sin_family = AF_INET;
	direccionServer.sin_addr.s_addr = INADDR_ANY;
	direccionServer.sin_port = htons(13546);

	int servidor = socket(AF_INET, SOCK_STREAM, 0);

	if(bind(servidor, (void*) &direccionServer, sizeof(direccionServer)) != 0){
		perror("Fallo el bind");
		exit(-1);
	}

	printf("ESCUCHANDO...\n");
	listen(servidor, SOMAXCONN);

	return servidor;
}

int esperarCliente(int servidor){
	struct sockaddr_in direccionCliente;
	unsigned int tamanioDireccion;
	int cliente = accept(servidor, (void*) &direccionCliente, &tamanioDireccion);
	printf("SE CONECTO UN CLIENTE\n");
	return cliente;
}


int crearConexion(char* ip, char* puerto){
	struct sockaddr_in direccionServer;
	direccionServer.sin_family = AF_INET;
	direccionServer.sin_addr.s_addr = inet_addr(ip);
	direccionServer.sin_port = htons(atoi(puerto));

	int cliente = socket(AF_INET, SOCK_STREAM, 0);
	if(connect(cliente, (void*) &direccionServer, sizeof(direccionServer)) != 0){
		perror("No se pudo conectar");
		exit(-1);
	}

	return cliente;
}





int iniciarServidor2(char* ip, char* puerto) {
		int socketServidor;
	    struct addrinfo hints, *servinfo, *p;

	    memset(&hints, 0, sizeof(hints));
	    hints.ai_family = AF_UNSPEC;
	    hints.ai_socktype = SOCK_STREAM;
	    hints.ai_flags = AI_PASSIVE;

	    getaddrinfo(ip, puerto, &hints, &servinfo);

	    for (p=servinfo; p != NULL; p = p->ai_next)
	    {
	        if ((socketServidor = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
	            continue;

	        if (bind(socketServidor, p->ai_addr, p->ai_addrlen) == -1) {
	        	printf("%s\n", strerror(errno));
	        	close(socketServidor);
	            continue;
	        }
	        break;
	    }


		listen(socketServidor, SOMAXCONN);

	    freeaddrinfo(servinfo);

	    return socketServidor;
}


int esperarCliente2(int socketServidor) {
	struct sockaddr_in dirCliente;
	socklen_t tamDireccion = sizeof(struct sockaddr_in);

	int socketCliente = accept(socketServidor, (void*) &dirCliente, &tamDireccion);
//	int socketCliente = accept(socketServidor, NULL, NULL);

	printf("%s\n", strerror(errno));

	return socketCliente;
}

int main(void) {
	int socket = iniciarServidor();
	int conexion = esperarCliente(socket);

	printf("Se conecto alguien, socket: %d, conexion: %d \n", socket, conexion);

	return EXIT_SUCCESS;
}
