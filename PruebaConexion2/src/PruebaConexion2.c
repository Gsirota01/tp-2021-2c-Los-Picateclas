/*
 ============================================================================
 Name        : PruebaConexion2.c
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
#include <netdb.h>
#include <commons/log.h>
#include <string.h>
#include <fcntl.h> // for open
#include <unistd.h> // for close



int crearConexion2(char* ip, char* puerto){
	  	struct addrinfo hints;
	    struct addrinfo *server_info;
	    int clientSocket;

	    memset(&hints, 0, sizeof(hints));
	    hints.ai_family = AF_UNSPEC;
	    hints.ai_socktype = SOCK_STREAM;
	    hints.ai_flags = AI_PASSIVE;

	    if(getaddrinfo(ip, puerto, &hints, &server_info)){
	    	exit(-1);
	    }

	    clientSocket = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);

	    if(connect(clientSocket, server_info->ai_addr, server_info->ai_addrlen) == -1){
		perror("ERROR: ");
	        exit(-1);
	    }


	    freeaddrinfo(server_info);
	    return clientSocket;
}


int crearConexion(char* ip){
	struct sockaddr_in direccionServer;
	direccionServer.sin_family = AF_INET;
	direccionServer.sin_addr.s_addr = inet_addr(ip);
	direccionServer.sin_port = htons(13546);

	int cliente = socket(AF_INET, SOCK_STREAM, 0);
	if(connect(cliente, (void*) &direccionServer, sizeof(direccionServer)) != 0){
		perror("No se pudo conectar");
		exit(-1);
	}

	return cliente;
}


int main(void) {

	int conexion = crearConexion("127.0.0.1");

	printf("Me conecte, conexion: %d\n", conexion);

	return EXIT_SUCCESS;
}
