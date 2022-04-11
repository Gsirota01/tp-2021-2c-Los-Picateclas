/*
 * PruebasDeadlock.h
 *
 *  Created on: 5 dic. 2021
 *      Author: utnso
 */

#ifndef PRUEBASDEADLOCK_H_
#define PRUEBASDEADLOCK_H_

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <commons/string.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>

typedef char* mate_sem_name;

typedef struct{
	t_list* carpinchos;
	mate_sem_name semaforo;
}matrizDeadlock;

typedef struct{
	int carpincho;
	mate_sem_name semaforo;
}listaDeadlock;

typedef struct{
	int contador;
	mate_sem_name semaforo;
}contadorSemaforo;

t_list* solicitudesActuales;
t_list* recursosAsignados;
t_list* contadoresSemaforos;


void hayDeadlock();
int estaEnAsignados(mate_sem_name semaforo, int chequeo);
bool estaElRecurso(matrizDeadlock matriz);
void reset(contadorSemaforo c);
void resetContadores();


#endif /* PRUEBASDEADLOCK_H_ */
