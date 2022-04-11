#ifndef PETICIONES_H_
#define PETICIONES_H_

#include <stdio.h>
#include <stdlib.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/string.h>
#include <commons/collections/list.h>
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
#include <semaphore.h>

typedef enum{
	LECTURA,
	ESCRITURA,
	ELIMINADO
}tipoDePeticion;

typedef struct{
  char* ip;
  char* puerto;
  int tamSwap;
  int tamPagina;
  char** archivosSwap;
  int marcosMaximos;
  int retardoSwap;
  char* tipoAsignacion;
}infConf;

typedef struct{
	int ID;
	void* punteroAMemoria;
	int cantDePaginasOcupadas;
	t_bitarray* bitmapDeFrames;
	//t_list* infoDePaginas;
}archivoParticion;

typedef struct{
	int IdDeParticion;
	int lugarPrimeraPag;
	int proceso;
}paginaEscritaFija;

typedef struct{
	int IdDeParticion;
	t_list* listaDePagsOcupadas;
	int proceso;
}paginaEscritaDinam;

typedef struct{
	tipoDePeticion pedido;
	int nroDePag;
	int proceso;
	void* pagina;
}infoPeticion;

typedef struct{
	int nroPag;
	int pagQueOcupaEnParticion;
}pagYLugar;

sem_t* semArchivos;
int cfr;

t_log* logSWamP;
t_config* configSWamP;

infConf infoConfig;
int conexion;

t_list* listaDeParticiones;
t_list* pagsEnParticiones;

int maxCantDePaginasEnParticion;

void procesarPeticion();

void envioDePaginaAsigFija(infoPeticion pedido);
void escribirPaginaAsigFija(infoPeticion pedido);
void eliminarPaginaAsigFija(infoPeticion pedido);

void envioDePaginaAsigDinam(infoPeticion pedido);
void escribirPaginaAsigDinam(infoPeticion pedido);
void eliminarPaginasAsigDinam(infoPeticion pedido);


paginaEscritaFija* existeElProcesoSwap(infoPeticion pedido);
bool existeYa(paginaEscritaFija pag);

paginaEscritaDinam* existeElProcesoSwapDinam(infoPeticion pedido);
bool existeYaDinam(paginaEscritaDinam pag);

archivoParticion* obtenerInfoParticion(int IdPart);
bool informacionDeParticion(archivoParticion info);

archivoParticion* particionAEscribir();
bool ordenarListaSegunOcupado(archivoParticion info, archivoParticion info2);

int chequeoEspacioEnParticion(archivoParticion* particion);
int ocuparEnBitamp(archivoParticion* particion, int pos);

void eliminarProcesoDeEstrAdmin(paginaEscritaFija* infoPag, archivoParticion* particion);
bool pagAElim(paginaEscritaFija infoPag2);

int proxPagLibre(archivoParticion* particion);

int buscarPaginaEnDinam(paginaEscritaDinam* infoPag, int nroDePag);
bool pagsIguales(pagYLugar* conjPags);

void eliminarPagina(archivoParticion* particion, int pagAElim);

void elimEstructAdminDePags(paginaEscritaDinam* infoPag);
bool pagAElimDinam(paginaEscritaDinam* infoPag2);

void reemplazarEstructAdminParticion(archivoParticion* particion);

int yaExisteEstaPagina(paginaEscritaDinam* infoPag, infoPeticion pedido);
bool estaPaginaExiste(pagYLugar* inf);

bool yaNoSirveMas(pagYLugar* inf);

#endif /* PETICIONES_H_ */
