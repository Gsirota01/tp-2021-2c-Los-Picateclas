/*
 ============================================================================
 Name        : pruebasSwamp.c
 Author      :
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include <commons/string.h>
#include <commons/bitarray.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stddef.h>
#include <pthread.h>

typedef enum{
	LECTURA,
	ESCRITURA,
	ELIMINADO
}tipoDePeticion;

typedef struct{
	tipoDePeticion pedido;
	int nroDePag;
	int proceso;
	void* pagina;
}infoPeticion;

int sizePags;
int conexion;


void* serializo(infoPeticion peticion){
	void* buffer = malloc(sizeof(int)*3 + sizePags);

	memcpy(buffer, &(peticion.pedido), sizeof(int));
	int desplazamiento = sizeof(tipoDePeticion);
	memcpy(buffer+desplazamiento, &(peticion.nroDePag), sizeof(int));
	desplazamiento += sizeof(int);
	memcpy(buffer+desplazamiento, &(peticion.proceso), sizeof(int));
	desplazamiento += sizeof(int);

	return buffer;
}

void lectura(int nroProceso, int nroPag){
	infoPeticion peticion;
	peticion.pedido = LECTURA;
	peticion.proceso = nroProceso;
	peticion.nroDePag = nroPag;

	void* aEnviar = serializo(peticion);
	int bytesAEnviar = sizeof(int)*3;

	send(conexion, &bytesAEnviar, sizeof(int), 0);
	send(conexion, aEnviar, bytesAEnviar, 0);

	void* bufferARecv = malloc(sizePags);
	recv(conexion, bufferARecv, sizePags, MSG_WAITALL);

	char* msj = malloc(sizePags);
	memcpy(msj, bufferARecv, sizePags);

	printf("%s\n", msj);
}


void escritura(int nroProceso, int nroPag, void* pag){
	infoPeticion peticion;
	peticion.pedido = ESCRITURA;
	peticion.proceso = nroProceso;
	peticion.nroDePag = nroPag;

	void* aEnviar = serializo(peticion);

	int desplazamiento = sizeof(int)*3;
	memcpy(aEnviar+desplazamiento, pag, sizePags);

	int bytesAEnviar = desplazamiento + sizePags;
	send(conexion, &bytesAEnviar, sizeof(int), 0);
	send(conexion, aEnviar, bytesAEnviar, 0);
}


void eliminado(int nroProceso, int nroPag){
	infoPeticion peticion;
	peticion.pedido = ELIMINADO;
	peticion.proceso = nroProceso;
	peticion.nroDePag = nroPag;

	void* aEnviar = serializo(peticion);
	int bytesAEnviar = sizeof(int)*3;

	send(conexion, &bytesAEnviar, sizeof(int), 0);
	send(conexion, aEnviar, bytesAEnviar, 0);
}












int crearConexion(char* ip, char* puerto){
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
	        exit(-1);
	    }


	    freeaddrinfo(server_info);
	    return clientSocket;
}





int main(void) {

	conexion = crearConexion("127.0.0.1", "41084");
	sizePags = 2048;

	char* v = string_repeat('a', sizePags);
	void* x = malloc(sizePags);
	memcpy(x, v, sizePags);

	escritura(0, 0,x);
	sleep(2);
	escritura(0, 1,x);
	sleep(2);
	escritura(0, 2,x);
	sleep(2);
	escritura(0, 3,x);
	sleep(2);
	escritura(0, 4,x);
	sleep(2);
	escritura(0, 5,x);
	sleep(2);
	escritura(0, 6,x);
	sleep(2);
	escritura(0, 7,x);
	sleep(2);
	escritura(0, 8,x);
	sleep(2);
	escritura(0, 9,x);

	close(conexion);
	sleep(200);

	return EXIT_SUCCESS;
}












