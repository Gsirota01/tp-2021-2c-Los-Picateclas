/*
 ============================================================================
 Name        : PruebasDeadlock.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "PruebasDeadlock.h"

int main(void) {

	solicitudesActuales = list_create();
	recursosAsignados = list_create();
	contadoresSemaforos = list_create();

	int p1 = 1;
	int p2 = 2;
	int p3 = 3;
	char* r1 = "R1";
	char* r2 = "R2";
	char* r3 = "R3";


	listaDeadlock* l1 = malloc(sizeof(listaDeadlock));
	l1->carpincho = p1;
	l1->semaforo = malloc(2);
	memcpy(l1->semaforo, r1, 2);
	listaDeadlock* l2 = malloc(sizeof(listaDeadlock));
	l2->carpincho = p2;
	l2->semaforo = malloc(2);
	memcpy(l2->semaforo, r2, 2);
	list_add(solicitudesActuales, l1);
	list_add(solicitudesActuales, l2);


//	listaDeadlock* l1 = malloc(sizeof(listaDeadlock));
//	l1->carpincho = p1;
//	l1->semaforo = malloc(2);
//	memcpy(l1->semaforo, r1, 2);
//	listaDeadlock* l2 = malloc(sizeof(listaDeadlock));
//	l2->carpincho = p3;
//	l2->semaforo = malloc(2);
//	memcpy(l2->semaforo, r2, 2);
//	listaDeadlock* l3 = malloc(sizeof(listaDeadlock));
//	l3->carpincho = p2;
//	l3->semaforo = malloc(2);
//	memcpy(l3->semaforo, r3, 2);
//	list_add(solicitudesActuales, l1);
//	list_add(solicitudesActuales, l2);
//	list_add(solicitudesActuales, l3);




	matrizDeadlock* m1 = malloc(sizeof(matrizDeadlock));
	m1->carpinchos = list_create();
	m1->semaforo = "R1";
	list_add(m1->carpinchos, &p2);
	matrizDeadlock* m2 = malloc(sizeof(matrizDeadlock));
	m2->carpinchos = list_create();
	m2->semaforo = "R2";
//	list_add(m2->carpinchos, &p1);
	list_add(recursosAsignados, m1);
	list_add(recursosAsignados, m2);


//	matrizDeadlock* m1 = malloc(sizeof(matrizDeadlock));
//	m1->carpinchos = list_create();
//	m1->semaforo = "R1";
//	list_add(m1->carpinchos, &p2);
//	list_add(m1->carpinchos, &p3);
//	matrizDeadlock* m2 = malloc(sizeof(matrizDeadlock));
//	m2->carpinchos = list_create();
//	m2->semaforo = "R2";
//	list_add(m2->carpinchos, &p1);
//	list_add(m2->carpinchos, &p3);
//	matrizDeadlock* m3 = malloc(sizeof(matrizDeadlock));
//	m3->carpinchos = list_create();
//	m3->semaforo = "R3";
//	list_add(m3->carpinchos, &p2);
//	list_add(m3->carpinchos, &p3);
//	list_add(recursosAsignados, m1);
//	list_add(recursosAsignados, m2);
//	list_add(recursosAsignados, m3);


	contadorSemaforo* sem1 = malloc(sizeof(contadorSemaforo));
	sem1->semaforo = r1;
	sem1->contador = 0;
	contadorSemaforo* sem2 = malloc(sizeof(contadorSemaforo));
	sem2->semaforo = r2;
	sem2->contador = 0;
//	contadorSemaforo* sem3 = malloc(sizeof(contadorSemaforo));
//	sem3->semaforo = r3;
//	sem3->contador = 0;
	list_add(contadoresSemaforos, sem1);
	list_add(contadoresSemaforos, sem2);
//	list_add(contadoresSemaforos, sem3);


	hayDeadlock();

	return EXIT_SUCCESS;
}


void hayDeadlock() {

	int flagDeadlock = 0;
	int flag;
	int chequeo;
	int hola = 0;

	t_list* carpinchosEnDeadlock;
	for (int i = 0; i < list_size(solicitudesActuales) && flagDeadlock != 1; i++) {
		listaDeadlock* listaSolicitudesActuales = list_get(solicitudesActuales, i);

		flag = 0;
		chequeo = -1;

		carpinchosEnDeadlock = list_create();
		mate_sem_name sema = listaSolicitudesActuales->semaforo;

		int carpinInicial = listaSolicitudesActuales->carpincho;
		list_add(carpinchosEnDeadlock, &carpinInicial);

		resetContadores();
		while(chequeo != carpinInicial && flag != 1){

			if(hola == 0){
				chequeo = carpinInicial;
				hola = 1;
			}

			int idCarpincho2 = estaEnAsignados(sema, chequeo);

			if(idCarpincho2 > 0){

				bool estaElCarpi(listaDeadlock* mm) {
					return mm->carpincho == idCarpincho2;
				}

				listaDeadlock* mm = list_find(solicitudesActuales, (void*) estaElCarpi);
				sema = mm->semaforo;
				chequeo = idCarpincho2;

				if(chequeo != carpinInicial){
					list_add(carpinchosEnDeadlock, &chequeo);
				}


				if (sema == NULL) {
					flag = 1;
				}

			}else{
				//Pasa al siguiente
				flag = 1;
			}

		}


		if (chequeo == carpinInicial) {
			flagDeadlock = 1;
		} else {
			list_destroy(carpinchosEnDeadlock);
		}

		hola = 0;
	}

	if (flagDeadlock == 1) {
		//HAY DEADLOCK
		printf("HAAAYYY DEEEADDDLOOCCKKKKKKKK!!!!!!!!!!!!!!!!!!!");

		//asesinarCarpincho(carpinchosEnDeadlock);//carpinchosEnDeadlock
		hayDeadlock();
	}

}




int estaEnAsignados(mate_sem_name semaforo, int chequeo){
	int a = -1;


	bool estaElRecurso(matrizDeadlock* matriz) {
		return string_equals_ignore_case(matriz->semaforo, semaforo);
	}

	matrizDeadlock* listaRecursosAsignados = list_find(recursosAsignados, (void*) estaElRecurso);

	bool encontrarContador(contadorSemaforo* c){
		return string_equals_ignore_case(c->semaforo, semaforo);
	}


	contadorSemaforo* contadorSem = list_find(contadoresSemaforos, (void*) encontrarContador);
	int aux = contadorSem->contador;

	if(listaRecursosAsignados != NULL){

		int tamLista = list_size(listaRecursosAsignados->carpinchos);
		if(aux < tamLista){

			int* carpi = list_get(listaRecursosAsignados->carpinchos, aux);

			if(*carpi == chequeo){
				aux += 1;
				carpi = list_get(listaRecursosAsignados->carpinchos, aux);
			}

			aux += 1;
			a = *carpi;

		}else{
			return -2;
		}

	}

	contadorSem->contador = aux;

	list_remove_by_condition(contadoresSemaforos, (void*) encontrarContador);
	list_add(contadoresSemaforos, contadorSem);

	return a;
}


void resetContadores(){

	void reset(contadorSemaforo c){
		c.contador = 0;
	}

	list_map(contadoresSemaforos, (void*) reset);
}



























