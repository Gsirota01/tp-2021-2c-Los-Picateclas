#include "pruebaHRRN.h"

int main(){

	mate_instanceACambiar* carpincho = malloc(sizeof(mate_instanceACambiar));
	carpincho->id=1;
	carpincho->estimacionProximaRafaga = 1;
	carpincho->tiempoDeEjecucionRafagaAnterior = 0;
	carpincho->tiempoInicialEnReady = time(NULL);


	sleep(2);
	estimarRafaga(carpincho);
	calcularTasaRespuesta(carpincho); // (1 / ~2) + 1 = 1.5// el '~' es de "aproximado"//

	printf("%f \n",carpincho->tasaRespuesta);

	carpincho->tiempoDeEjecucionRafagaAnterior = 3;
	carpincho->tiempoInicialEnReady = time(NULL);

	sleep(5);
	estimarRafaga(carpincho);
	calcularTasaRespuesta(carpincho);// (2 / ~5 ) + 1 = 1.4

	printf("%f \n",carpincho->tasaRespuesta);

	carpincho->tiempoDeEjecucionRafagaAnterior = 4;
	carpincho->tiempoInicialEnReady = time(NULL);

	sleep(3);
	estimarRafaga(carpincho);
	calcularTasaRespuesta(carpincho); // (3 / ~3 ) + 1 = 2
	printf("%f \n",carpincho->tasaRespuesta);

	return 0;
}


void calcularTasaRespuesta(mate_instanceACambiar* proceso){

	time_t tiempoInicialEnReady = proceso->tiempoInicialEnReady;
	time_t estimacionRafaga = proceso->estimacionProximaRafaga;

	time_t tiempoEnEspera = difftime(time(NULL) ,tiempoInicialEnReady) ;

	proceso->tasaRespuesta = (estimacionRafaga / (float)tiempoEnEspera) + 1;

}


void estimarRafaga(mate_instanceACambiar* proceso){
	time_t tiempoDeEjecucion = proceso->tiempoDeEjecucionRafagaAnterior;
	float alfa = 0.5;

	if (tiempoDeEjecucion == 0){
		proceso->estimacionProximaRafaga = 1;
	}else{
	proceso->estimacionProximaRafaga = alfa * tiempoDeEjecucion + (1 - alfa) * proceso->estimacionProximaRafaga;
	return;
	}
}

