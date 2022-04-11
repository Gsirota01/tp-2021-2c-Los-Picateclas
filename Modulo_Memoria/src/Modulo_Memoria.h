#ifndef MODULO_MEMORIA_H_
#define MODULO_MEMORIA_H_

#include <stdio.h>
#include <stdlib.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <commons/bitarray.h>
#include <string.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stddef.h>
#include <pthread.h>
#include <semaphore.h>
#include <math.h>
#include <errno.h>


t_log* logMemoria;
t_config* configMemoria;
int tamanio_frame;
int socket_swap;

enum mate_errors {
    MATE_FREE_FAULT = -5,
    MATE_READ_FAULT = -6,
    MATE_WRITE_FAULT = -7
};
int crearConexion(char* ip, char* puerto);
//---------------Conexion Swap ----------------------

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
void* serializo(infoPeticion peticion);
void escritura(infoPeticion peticion, void* pag);
void *lectura(infoPeticion peticion);
void eliminado(infoPeticion peticion);

typedef struct{
	int conexion;
}arg_struct;
void procesar_peticion(arg_struct* argumentos);


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
}tipoDePeticionMemoria;


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
	t_list* semaforos;
}mate_instanceACambiar;


typedef struct{
	sem_t* semaforo;
	int instancias;
	char* nombre;
	t_list* carpinchos;
}infoSemaforo;



typedef struct{
  char* ip;
  char* puerto;
  char* ip_swamp;
  char* puerto_swamp;
  int tamMemoria;
  int tamPagina;
  int marcosMaximos;
  int entradasTLB;
  int retardoHit;
  int retardoMiss;
  int mostrarMemoria;
  int mostrarTLB;
  char* tipoAsignacion;
  char* algoReemplzoTLB;
  char* algoReemplazoMMU;
  char* path;
}infConfMemoria;
infConfMemoria info;

typedef struct{
	bool vacio;
	void *ptr_inicio;
	int nro_frame;
}t_frame;

typedef struct{
	bool modificado;
	bool presencia;
	bool uso;
	int nro_pag;
	int nro_frame;
	int pid;
}t_entrada_pagina;


typedef struct{
	int tlb_hit;
	int tlb_miss;
}t_metricas;

typedef struct{
	int tamanio;
	void *leido;
}t_memread;

typedef struct{
	int pid;
	t_list *tabla_de_paginas;
	t_list *lista_paginas_presentes;
	t_list *lista_datos_alloc;
	int *reloj;
	sem_t swap_disponible;
	infoPeticion info_peticion;
	t_metricas metricas;
}t_info_proceso;

typedef struct{
	uint32_t prev_alloc;
	uint32_t next_alloc;
	uint8_t is_free;
}t_alloc;

typedef struct{
	int direccion;
	int tamanio;
}t_dato_alloc;






int tamanio_memoria;
void *memoria;
int limite_frames;
int idProceso;
int numero_carpinchos_activos;
uint32_t null_alloc;
t_list *lista_frames;
t_list *lista_paginas_global;
t_list *lista_entradas_TLB;
t_list *pila_LRU_TLB;
t_list *cola_pedidos_swap;
t_queue *cola_pedidos;
t_list *lista_procesos;
int *reloj_global;
int reloj_posta;
int fin;
int sigusr1;
int sigusr2;
int primero;

int iniciarServidor(char* puerto);
int esperarCliente(int socketServidor);
int crearConexionDiscordiador(char* ip, char* puerto, t_log* logDiscordiador);
int setup_log_config();
void nuevo_hilo(int cliente);
void inicializar_memoria();
void liberar_memoria();
int crear_paginas(int size, t_info_proceso *info_proceso);
t_alloc *obtener_alloc(t_info_proceso *info_proceso, int dir_logica);
void *thread_proceso();
void eliminar_proceso(t_info_proceso *info_proceso);
void suspender_proceso(t_info_proceso *info_proceso);
int memalloc(int tamanio, t_info_proceso *info_proceso);
int memfree(t_info_proceso *info_proceso, int dir_log);
t_memread* memread(t_info_proceso *info_proceso, int dir_logica, int tamanio);
int memwrite(t_info_proceso *info_proceso, int dir_logica, void *data, int tamanio);
t_frame *obtener_frame(t_info_proceso *info_proceso, int nro_pag, bool es_escritura);
void actualizar_acceso_MMU(t_info_proceso *info_proceso,t_entrada_pagina *entrada_pagina, bool es_escritura);
t_frame *buscar_frame_libre();
t_frame *reemplazar_pagina(t_info_proceso *info_proceso, t_entrada_pagina *entrada_pagina_no_presente, t_list *lista_paginas_presentes, void *pagina_a_escribir);
t_entrada_pagina *elegir_victima_MMU(t_info_proceso *info_proceso,t_entrada_pagina *entrada_pagina_no_presente);
void *leer(t_info_proceso *info_proceso, double dir_logica, int tamanio);
void *escribir(t_info_proceso *info_proceso, double dir_logica, int tamanio, void *data);
t_entrada_pagina *buscar_en_TLB(t_info_proceso *info_proceso, int nro_pag);
void actualizar_LRU(t_list *lista, t_entrada_pagina *entrada);
void actualizar_clock(t_list *lista_paginas_presentes, t_entrada_pagina *pagina, int *reloj);
void metricas_SIGINT();
static void signalHandler(int sig);
void dump_SIGURS1();
void mostrar_tlb();
void limpiar_TLB_SIGURS2();
void mostrar_paginas_presentes();


sem_t pedido;
sem_t pedido_completado;
sem_t sem_fin;
sem_t sem_fin_swamp;
sem_t carpinchos_activos;

pthread_mutex_t mutex_cola_pedidos;
pthread_mutex_t mutex_lista_procesos;

void conexion_swap();
void escribir_swap(t_info_proceso *info_proceso,t_entrada_pagina *entrada_pagina, void *pagina);
void *leer_swap(t_info_proceso *info_proceso, t_entrada_pagina *entrada_no_presente);

int sizeConfig(infoConfig info);
int sizeDeCarpincho(mate_instanceACambiar* carpincho);
void liberarConfig(infoConfig info);
void liberarCarpincho(mate_instanceACambiar* carpincho);
tamConfig desserializarSizeConfig(void* buffer, int desplazamiento);
infoConfig limpiarConfig(infoConfig info, tamConfig tam);
infoConfig desserializarConfig(void* buffer, int desplazamiento, tamConfig tam);
mate_instanceACambiar* desserializarCarpincho(void* buffer);
tamConfig valoresConfig(infoConfig info);
void* serializarSizeConfig(infoConfig info);
void* serializarConfig(infoConfig info);
void* serializarCarpincho(mate_instanceACambiar *lib_ref);

#endif /* MODULO_MEMORIA_H_ */
