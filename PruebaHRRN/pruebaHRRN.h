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


typedef struct{
    int id;
	float estimacionProximaRafaga;
	float tasaRespuesta;
	time_t tiempoInicialEnReady;
	time_t tiempoInicialEnExec;
	time_t tiempoDeEjecucionRafagaAnterior;
}mate_instanceACambiar;


void calcularTasaRespuesta(mate_instanceACambiar* proceso);
void estimarRafaga(mate_instanceACambiar* proceso);
