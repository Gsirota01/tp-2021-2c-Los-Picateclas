#include "pruebaSJF.h"

int main(){

	mate_instanceACambiar* carpincho = malloc(sizeof(mate_instanceACambiar));
	carpincho->id=1;
	carpincho->estimacionProximaRafaga = 1;
	carpincho->tiempoDeEjecucionRafagaAnterior = 0;


	estimarRafaga(carpincho); // 1

	printf("%f \n",carpincho->tasaRespuesta);

	carpincho->tiempoDeEjecucionRafagaAnterior = 3;

	estimarRafaga(carpincho); // 0.5 * 3 + (1 - 0.5) * 1 = 2

	printf("%f \n",carpincho->tasaRespuesta);

	carpincho->tiempoDeEjecucionRafagaAnterior = 4;

	estimarRafaga(carpincho); // 0.5 * 4 + (1 - 0.5) * 2 = 3
	printf("%f \n",carpincho->tasaRespuesta);

	return 0;
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

