#ifndef MATELIB_H_
#define MATELIB_H_


#include <stdint.h>
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
#include <semaphore.h>


//-------------------Type Definitions----------------------/

typedef char* mate_sem_name;

typedef int32_t mate_pointer;

typedef char* mate_io_resource;


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

enum mate_errors{
	MATE_FREE_FAULT = -5,
	MATE_READ_FAULT = -6,
	MATE_WRITE_FAULT = -7
};

typedef struct{
	void* group_info;
}mate_instance;

typedef struct{
	tipoDePeticion peticion;
	int idCarpincho;
	int idSemaforo;
	unsigned int valorSemaforo;
}mate_inner_structure;


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
	float estimacionProximaRafaga;
	float tasaRespuesta;
	time_t tiempoInicialEnReady;
	time_t tiempoInicialEnExec;
	time_t tiempoDeEjecucionRafagaAnterior;
	int conexionMemoria;
	tamConfig sizeConfig;
	infoConfig info;
}mate_instanceACambiar;


//typedef char *mate_io_resource;

typedef struct{
	char* nombre;
	sem_t* semaforo;
	int duracion;
}infoIOResource;

typedef struct{
	sem_t* semaforo;
	int instancias;
	char* nombre;
}mate_sem_name2;





// TODO: Docstrings
//-----------------Funciones para conectar---------------/

int crearConexion(char* ip, char* puerto);
infoConfig obtenerInfoConfig();
void obtenerInfoConfigFinal(t_config* config, mate_instanceACambiar* carpincho);
void* serializarCarpincho(mate_instanceACambiar *carpincho);

int sizeConfig(infoConfig info);
int sizeDeCarpincho(mate_instanceACambiar* carpincho);
int sizeDeSemaforo();
int sizeDeIO();

void* serializarSemaforo(mate_sem_name semaforo);
void* serializarPointer(mate_pointer addr);
void* serializarConfig(infoConfig info);
infoConfig desserializarConfig(void* buffer, int desplazamiento, tamConfig tam);
void* serializarSizeConfig(infoConfig info);
tamConfig desserializarSizeConfig(void* buffer, int desplazamiento);
mate_instanceACambiar* desserializarCarpincho(void* buffer);
mate_instanceACambiar* desserializarCarpincho2(void* buffer);


tamConfig valoresConfig(infoConfig info);
void liberarCarpincho(mate_instanceACambiar* carpincho);
infoConfig limpiarConfig(infoConfig info, tamConfig tam);
void liberarConfig(infoConfig info);

//------------------General Functions---------------------/
int mate_init(mate_instance *lib_ref, char *config);

int mate_close(mate_instance* lib_ref);

//-----------------Semaphore Functions---------------------/
int mate_sem_init(mate_instance *lib_ref, mate_sem_name sem, unsigned int value);

int mate_sem_wait(mate_instance *lib_ref, mate_sem_name sem);

int mate_sem_post(mate_instance *lib_ref, mate_sem_name sem);

int mate_sem_destroy(mate_instance *lib_ref, mate_sem_name sem);

//--------------------IO Functions------------------------/

int mate_call_io(mate_instance* lib_ref, mate_io_resource io, void *msg);

//--------------Memory Module Functions-------------------/

mate_pointer mate_memalloc(mate_instance* lib_ref, int size);

int mate_memfree(mate_instance* lib_ref, mate_pointer addr);

int mate_memread(mate_instance* lib_ref, mate_pointer origin, void *dest, int size);

int mate_memwrite(mate_instance* lib_ref, void *origin, mate_pointer dest, int size);



#endif /* MATELIB_H_ */
