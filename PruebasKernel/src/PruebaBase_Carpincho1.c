/*
 ============================================================================
 Name        : PruebaSuspension.c
 Description : Prueba de suspension de carpinchos del TP CarpinchOS
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "matelib.h"
#include <string.h>
#include <pthread.h>

#define SEMAFORO_SALUDO "SEM_HELLO"

void carpin1() {

    char* config = "/home/utnso/tp-2021-2c-Los-Picateclas/PruebasKernel/libreria.config";


	printf("MAIN - Utilizando el archivo de config: %s\n", config);

	mate_instance instance;

	mate_init(&instance, config);

    char saludo[] = "No, ¡hola humedal!\n";

    mate_pointer saludoRef = mate_memalloc(&instance, strlen(saludo));

    mate_memwrite(&instance, saludo, saludoRef, strlen(saludo));

    mate_sem_init(&instance, SEMAFORO_SALUDO, 0);

    mate_sem_wait(&instance, SEMAFORO_SALUDO);

    mate_memread(&instance, saludoRef, saludo, strlen(saludo));

    printf(saludo);

    mate_close(&instance);

}

void carpin2(){

	char* config = "/home/utnso/tp-2021-2c-Los-Picateclas/PruebasKernel/libreria.config";

	printf("MAIN - Utilizando el archivo de config: %s\n", config);

	mate_instance instance;

	mate_init(&instance, config);

	char saludo[] = "¡Hola mundo!\n";

	mate_pointer saludoRef = mate_memalloc(&instance, strlen(saludo));

	mate_memwrite(&instance, saludo, saludoRef, strlen(saludo));

	mate_memread(&instance, saludoRef, saludo, strlen(saludo));

	printf(saludo);

	mate_sem_post(&instance, SEMAFORO_SALUDO);

	mate_close(&instance);


}

int main (){

	pthread_t carpi1;
	pthread_t carpi2;

	pthread_create(&carpi1, NULL, (void*)carpin1, NULL);
	sleep(2);
	pthread_create(&carpi2, NULL, (void*)carpin2, NULL);

	pthread_join(carpi1, NULL);
	pthread_join(carpi2, NULL);
}
