///*
// * carpincho.c
// *
// *  Created on: 5 dic. 2021
// *      Author: utnso
// */
//
//
//#include <stdio.h>
//#include <stdlib.h>
//#include <unistd.h>
//#include <string.h>
//#include "matelib.h"
//#include <pthread.h>
//
//#define SEMAFORO_SALUDO "SEM_HELLO"
//
//
////void carpinPrueba1(){
////	char* config = "/home/utnso/tp-2021-2c-Los-Picateclas/PruebasKernel/libreria.config";
////	mate_instance instance;
////
////	mate_init(&instance, config);
////
////    char saludo[] = "No, ¡hola humedal!\n";
////
////    mate_pointer saludoRef = mate_memalloc(&instance, strlen(saludo));
////
////    mate_memwrite(&instance, saludo, saludoRef, strlen(saludo));
////
////    mate_sem_init(&instance, SEMAFORO_SALUDO, 0);
////
////    mate_sem_wait(&instance, SEMAFORO_SALUDO);
////
////    mate_memread(&instance, saludoRef, saludo, strlen(saludo));
////
////    printf(saludo);
////
////    mate_close(&instance);
////}
////
////
////void carpinPrueba2(){
////	char* config = "/home/utnso/tp-2021-2c-Los-Picateclas/PruebasKernel/libreria.config";
////
////	mate_instance instance;
////
////	mate_init(&instance, config);
////
////	char saludo[] = "¡Hola mundo!\n";
////
////	mate_pointer saludoRef = mate_memalloc(&instance, strlen(saludo));
////
////	mate_memwrite(&instance, saludo, saludoRef, strlen(saludo));
////
////	mate_memread(&instance, saludoRef, saludo, strlen(saludo));
////
////	printf(saludo);
////
////	mate_sem_post(&instance, SEMAFORO_SALUDO);
////
////	mate_close(&instance);
////}
//
//
//void carpincho1XD(){
//
//	char* config = "/home/utnso/tp-2021-2c-Los-Picateclas/PruebasKernel/libreria.config";
//	mate_instance carpin;
//	printf("Carpincho 1 iniciando...\n");
//	mate_init(&carpin,config);
//
////	mate_pointer mp = mate_memalloc(&carpin, 2050);
////	printf("MATE POINTER ES: %d \n", mp);
////	mate_pointer mp2 = mate_memalloc(&carpin, 1);
////	printf("MATE POINTER ES: %d \n", mp2);
////
////	char* stri = malloc(10);
////	sprintf(stri, "%s \n", "CARPINCHO");
////
////	int n = mate_memwrite(&carpin, stri, mp, 10);
////	printf("TIENE QUE SALIR BIEN %d \n", n);
////
//////	int error = mate_memwrite(&carpin, "HOLA", 2, 5);
//////	printf("ERROR: %d \n", error);
////
////	char* stri2 = malloc(10);
////	int nn = mate_memread(&carpin, mp, stri2, 10);
////	printf("EL MENSAJE LLEGO %d Y ES: %s \n", nn, stri2);
////
////	int resp1 = mate_memfree(&carpin, mp);
////	int resp2 = mate_memfree(&carpin, 1);
////
////	printf("MEMFREE 1 DEVUELVE: %d \n", resp1);
////	printf("MEMFREE 2 DEVUELVE: %d \n", resp2);
//
////	mate_call_io(&carpin, "pelopincho", NULL);
//	mate_sem_init(&carpin,"SEM1ALGOMASGRANDE",0);
////
//	mate_sem_wait(&carpin,"SEM1ALGOMASGRANDE");
////
////	mate_sem_post(&carpin, "SEM1ALGOMASGRANDE");
////
//	mate_sem_destroy(&carpin, "SEM1ALGOMASGRANDE");
//
//	mate_close(&carpin);
//	puts("HOLAA SOY EL CARPINCHO UNO!!!!!!!!!!\n");
//
//}
//
//void carpincho2(){
//
//	char* config = "/home/utnso/tp-2021-2c-Los-Picateclas/PruebasKernel/libreria.config";
//	mate_instance carpin;
//	printf("Carpincho 2 iniciando... \n");
//	mate_init(&carpin,config);
////	mate_call_io(&carpin, "laguna", NULL);
////	sleep(2);
//	puts("SOY EL DOS!\n");
//	mate_sem_post(&carpin,"SEM1ALGOMASGRANDE");
////	sleep(2);
//	mate_close(&carpin);
//
//}
//
//
//
////void pruebaSuspMemoria(){
////
////	char* config = "/home/utnso/tp-2021-2c-Los-Picateclas/PruebasKernel/libreria.config";
////	mate_instance carpin;
////	mate_init(&carpin,config);
////
////
////	sleep(4);
////	mate_instanceACambiar* carpincho = carpin.group_info;
////	int bytesCarpinchos = sizeDeCarpincho(carpincho);
////	void* carpiSerializado = serializarCarpincho(carpincho);
////	tipoDePeticion p = PROCESOSUSPENDIDO;
////	void* bufferAMandar = malloc(sizeof(tipoDePeticion)+bytesCarpinchos);
////	memcpy(bufferAMandar, &p, sizeof(tipoDePeticion));
////	memcpy(bufferAMandar + sizeof(tipoDePeticion), carpiSerializado, bytesCarpinchos);
////
//////	desserializarCarpincho2(bufferAMandar);
////
////
////
////	int conexionMemoria = crearConexion("127.0.0.1", "45943");
////	printf("CONEXION MEMORIA: %d \n", conexionMemoria);
////
////	int bytes = sizeof(tipoDePeticion) + bytesCarpinchos;
////	send(conexionMemoria, &bytes, sizeof(int), 0);
////	send(conexionMemoria, bufferAMandar, bytes, 0);
////
//////	mate_close(&carpin);
////
////}
//
//
//int main(){
//
//	//mate_instance carpin;
////	char* config = "/home/utnso/tp-2021-2c-Los-Picateclas/PruebasKernel/libreria.config";
//
//
//	pthread_t* carpincho1 = malloc(sizeof(pthread_t));
////	pthread_t* carpincho2 = malloc(sizeof(pthread_t));
////	pthread_t carpincho3;
////	pthread_t carpincho4;
////	pthread_t carpincho5;
////	pthread_t carpincho6;
////	pthread_t carpincho7;
////	pthread_t carpincho8;
//
//
//	pthread_create(carpincho1, NULL, (void*) carpincho1XD, NULL);
//	sleep(2);
//	carpincho2();
////	pthread_create(carpincho2, NULL, (void*) carpincho2,NULL);
////	sleep(2);
////	pthread_create(&carpincho3, NULL, (void*) carpincho, NULL);
////	sleep(2);
////	pthread_create(&carpincho4, NULL, (void*) carpincho2, NULL);
////	sleep(5);
////	pthread_create(&carpincho5, NULL, (void*) carpincho2, NULL);
////	sleep(5);
////	pthread_create(&carpincho6, NULL, (void*) carpincho,NULL);
////	sleep(5);
////	pthread_create(&carpincho7, NULL, (void*) carpincho2, NULL);
////	sleep(5);
////	pthread_create(&carpincho8, NULL, (void*) carpincho, NULL);
//
//	pthread_detach(*carpincho1);
////	pthread_detach(*carpincho2);
////	pthread_join(carpincho3, NULL);
////	pthread_join(carpincho4, NULL);
////	pthread_join(carpincho5, NULL);
////	pthread_join(carpincho6, NULL);
////	pthread_join(carpincho7, NULL);
////	pthread_join(carpincho8, NULL);
//
//
//	sleep(200);
//
//
//
//	return 0;
//}
//
//
//
//
