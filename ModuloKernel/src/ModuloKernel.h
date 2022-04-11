/*
 * ModuloKernel.h
 *
 *  Created on: 31 oct. 2021
 *      Author: utnso
 */

#ifndef MODULOKERNEL_H_
#define MODULOKERNEL_H_


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
#include <semaphore.h>
#include <pthread.h>
#include <errno.h>



typedef enum{
	NUEVOPROCESO,
	NUEVOSEMAFORO,
	SEMWAIT,
	SEMPOST,
	ELIMINARSEMAFORO,
	NUEVAIO,
	PEDIRMEMORIA,
	LEERMEMORIA,
	ESCRIBIRMEMORIA,
	LIBERARMEMORIA,
	LIBERARPROCESO,
	PROCESOSUSPENDIDO
}tipoDePeticion;


typedef struct{
	int largoIpKernel;
	int largoPuertoKernel;
	int largoIpMemoria;
	int largoPuertoMemoria;
	int largomoduloAConectar;
}tamConfig;


typedef struct{
	char* ipKernel;
	char* puertoKernel;
	char* ipMemoria;
	char* puertoMemoria;
	char* moduloAConectar;
}infoConfig;


typedef struct{
    int id;
	float estimacionProximaRafaga; //SJF/HRRN
	float tasaRespuesta; //HRRN
	time_t tiempoInicialEnReady;
	time_t tiempoInicialEnExec;
	double tiempoDeEjecucionRafagaAnterior;
	int conexionMemoria;
	tamConfig sizeConfig;
	infoConfig info;
}mate_instanceACambiar;

typedef char* mate_sem_name;

typedef struct{
	sem_t* semaforo;
	char* nombre;
	int valorDelSem;
}infoSemaforo;

typedef int32_t mate_pointer;

typedef char* mate_io_resource;


typedef struct{
  char* ipMemoria;
  char* puertoMemoria;
  char* puertoEscucha;
  char* ip;
  char* algoritmo;
  int estimacionInicial;
  double alfa;
  t_list* dispositivosIO;
  t_list* duracionesIO;
  int tiempoDeadlock;
  int gradoMultiprogramacion;
  int gradoMultiprocesamiento;
}infConf;


typedef struct{
	tipoDePeticion pedido;
	mate_instanceACambiar* proceso;
}infoPeticion;

typedef struct{
	int conexion;
}arg_struct;

typedef struct{
	char* nombre;
	sem_t* semaforo;
	int duracion;
} recursoIO;

typedef struct{
	t_list* valores;
	char* nombre;
}tablaRecursos;

typedef struct{
	t_list* carpinchos;
	mate_sem_name semaforo;
}matrizDeadlock;// Recursos asignados

typedef struct{
	int carpincho;
	int socket;
	mate_sem_name semaforo;
}listaDeadlock; //Solicitudes actuales

typedef struct{
	int contador;
	mate_sem_name semaforo;
}contadorSemaforo;

typedef struct{
	int id;
	sem_t* semaforo;
	bool estoyBloqueado;
}semReady;


t_log* logKernel;

t_config* configKernel;
infConf info;

sem_t* carpinchos_en_ejecucion; //GRADO DE MULTIPROCESAMIENTO
sem_t* carpinchos_listos_para_ejecutar;//GRADO DE MULTIPROGRAMACION
sem_t* semDeadlock;
sem_t* semReadyExec;
sem_t* semNewReady;
sem_t* semBlokedSuspend;
sem_t* modificandoEstados;
sem_t* semListaCarpinchos;
sem_t* semaforoDeSemaforo;
sem_t* semaforoDeSemaforosExec;
sem_t* semRecursosTotales;
sem_t* semaforoRecursoIO;
sem_t* semExecBlocked;
sem_t* semAsignarId;


t_list* listaDeCarpinchos;
t_list* listaSemaforosReady;
t_list* listaSemaforosExec;

t_queue* new;
t_list* ready;
t_list* exec;
t_list* blocked;
t_list* suspendBlocked;
t_queue* suspendReady;
t_list* exitState;

t_list* recursosAsignados;//[matrizDeadlock1, matrizDeadlock2, matrizDeadlock3]
t_list* solicitudesActuales;//[listaDeadlock1,listaDeadlock2,listaDeadlock3]
t_list* contadoresSemaforos;
t_list* recursosTotales;
t_list* recursosDisponibles;
t_list* recursosIO;

int idCarpinchos;
int terminarPrograma;


infConf obtenerInfoConfig();
void* serializarCarpincho(mate_instanceACambiar *carpincho);
mate_instanceACambiar* desserializarCarpincho(void* buffer);
void* serializarConfig(infoConfig info);
void esperarAExec(int id);
infoConfig desserializarConfig(void* buffer, int desplazamiento, tamConfig tam);
tamConfig desserializarSizeConfig(void* buffer, int desplazamiento);
void* serializarSizeConfig(infoConfig info);
tamConfig valoresConfig(infoConfig info);
int sizeConfig(infoConfig info);
mate_pointer* desserializarPointer(void* buffer);
int iniciarServidor(char* ip, char* puerto, t_log * log);
int esperar_cliente(int socketServidor, t_log * log);
int crearConexion(char* ip, char* puerto, t_log* log);
void conectarAMemoria(t_log* logKernel, t_log* configKernel);
void inicializarListaDeEstados();
bool encotrar(int* idC);
//int verificarGradoDeMultiprogramacion();
//int verificarGradoMultiprocesamiento();

void liberarCarpincho(mate_instanceACambiar* carpincho);
void liberarConfig(infoConfig info);
infoConfig limpiarConfig(infoConfig info, tamConfig tam);

void estimarRafaga(mate_instanceACambiar* proceso);
void calcularTasaRespuesta(mate_instanceACambiar* proceso);
void calcularRafagaEjecucion(int idProceso);

mate_instanceACambiar* minimoRafagaSJF(mate_instanceACambiar* procesoA, mate_instanceACambiar* procesoB);
mate_instanceACambiar* mayorTiempoRespuestaHRRN(mate_instanceACambiar* procesoA, mate_instanceACambiar* procesoB);
int validarEstadoSuspension();

void cargarNuevoProceso(mate_instanceACambiar* proceso);
void transicionNewReady();
void transicionReadyExec();
void transicionExecBlocked(mate_instanceACambiar* carpincho);
void transicionBlockedReady(mate_instanceACambiar* carpincho);
void transicionBlockedSuspendBlocked();
void transicionSuspendBlockedSuspendReady(mate_instanceACambiar* carpincho);
void finalizarCarpincho(mate_instanceACambiar* carpincho);

void* recibir_buffer(int* size, int socket_cliente);
int recibir_operacion(int socket_cliente);
void nuevoHilo(int cliente);

void inicializarSemaforos();
void agregarRecursoInicial(infoSemaforo* semaforo);
void generarRecursosIO();
recursoIO* nuevoRecursoIO(char* nombre,int duracion);
void agregarARecursosTotales(infoSemaforo* semaforo);
void eliminarDeRecursosTotales(mate_sem_name semaforo);
bool procesoEstaBloqueado(int idCarpincho);
void transicionBlockedFinish(int idCarpincho);
void algoritmoDeadlock();
void hayDeadlock();
int estaEnAsignados(mate_sem_name semaforo, int chequeo);
bool estaElRecurso(matrizDeadlock matriz);
void reset(contadorSemaforo c);
void resetContadores();
void hacerPostDeSemsCarpincho(mate_instanceACambiar* carpincho);

void iniciarPrograma();

void mostrarTlistint(t_list* lista);
void mostrarTlist(t_list* lista);
void printCarpinsEnDeadlock(t_list* carpinchosEnDeadlock);

void nuevoSemaforo(mate_sem_name sem, int valorSem);
void semWait(mate_sem_name semaforoLib, mate_instanceACambiar* carpincho, int socket);
void semPost(mate_sem_name semaforoLib, mate_instanceACambiar* carpincho);
void semPostEnDeadlock(mate_sem_name semaforoLib, mate_instanceACambiar* carpincho);
void agregarASolicitudesActuales(mate_sem_name semaforoSolicitado ,mate_instanceACambiar* carpinchoSolicitante, int socket);
void sacarDeSolicitudesActuales(mate_sem_name semaforoSolicitado, mate_instanceACambiar* carpinchoSolicitante);
void sacarDeRecursosAsignados(mate_sem_name semaforo, mate_instanceACambiar* carpincho);
void agregarARecursosAsignados(mate_sem_name semaforo, mate_instanceACambiar* carpincho);
bool _encontrarSemaforoEnLista(infoSemaforo* semaforoEnLista);
void carpinchoPideIO(mate_io_resource recurso, mate_instanceACambiar* carpincho);
infoSemaforo* encontrarSemaforo(mate_sem_name semaforoLib);
recursoIO* encontrarRecursoIO(mate_io_resource recursoIO);
bool _encontrarRecursoIOEnLista(recursoIO* recursoIOLista);
void mostrarTiempos(t_list* listaCarpinchos);

int generarId();
int sizeDeCarpincho(mate_instanceACambiar* carpincho);

t_list* arrayATlist(char** array);
t_list* arrayATlistDeInts(char** array);

void frezee(char** array);

void mostrarEstados();
void printearEstado(char* estado, t_list* listaEstado);
void printearEstado2(char* estado, t_list* listaEstado);
t_list* deQueueALista(t_queue* colaEstado);
t_queue* deListaAQueue(t_list* listaEstado);

mate_instanceACambiar* copyPaste(mate_instanceACambiar*  carpincho);
mate_instanceACambiar* obtenerCarpincho(int idCarpi);
t_list* carpinchosEnReady();
void actualizarCarpincho(mate_instanceACambiar* carpinchoViejo);
void actualizarCarpincho2(mate_instanceACambiar* carpinchoViejo);

void estimarRafagaDeTodosEnReady();
void calcularTasaRespuestaDeTodosEnReady();
void tiempoInicialEnReady(int idCarpi);
void tiempoInicialEnExec(int idCarpi);
void mostrarSolicitudesActualesyRec();

#endif /* MODULOKERNEL_H_ */
