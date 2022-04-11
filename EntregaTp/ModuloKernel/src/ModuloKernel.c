/*
 ============================================================================
 Name        : ModuloKernel.c
 Author      :
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "ModuloKernel.h"

#include <netinet/in.h>
#include <stddef.h>

int main(void) {

	//INICIALIZO EL LOG Y EL CONFIG
	logKernel = log_create("kernel.log", "kernel", 1, LOG_LEVEL_INFO);
	log_info(logKernel, "INICIANDO LOG kernel...");

	configKernel = config_create("/home/utnso/tp-2021-2c-Los-Picateclas/ModuloKernel/kernel.config");
	log_info(logKernel, "CREANDO CONFIG kernel...");

	if (configKernel == NULL) {
		log_error(logKernel, "NO SE PUDO CREAR EL CONFIG...");
		exit(-3);
	}
	idCarpinchos = 0;

	info = obtenerInfoConfig();

	inicializarSemaforos();
	inicializarListaDeEstados();
	iniciarPrograma();

	int server_fd = iniciarServidor("0.0.0.0", info.puertoEscucha);
	int cliente_fd = esperarCliente(server_fd);

	while (1) {

		nuevoHilo(cliente_fd);

		cliente_fd = esperarCliente(server_fd);
	}

	free(carpinchos_listos_para_ejecutar);
	free(carpinchos_en_ejecucion);

	return EXIT_SUCCESS;
}


void esperarAExec(int id){

	bool encotrar(int* idC){
		return *idC == id;
	}

	bool encontrarCarpi(semReady* semR){
		return semR->id == id;
	}

	sem_wait(modificandoEstados);
	int* idCarp = list_find(exec, (void*) encotrar);
	sem_post(modificandoEstados);

//	printf("Holaa te queria avisar que voy a pedir wait, %d\n", id);

	sem_wait(semaforoDeSemaforosExec);
	semReady* semRe = list_find(listaSemaforosExec, (void*) encontrarCarpi);
	sem_post(semaforoDeSemaforosExec);

	if(idCarp == NULL){// && semRe != NULL){

//		printf("Voy a hacer wait de para ir a EXEC, %d \n", id);
//		printf("El booleano es: %d \n", semRe->estoyBloqueado);

		semRe->estoyBloqueado = true;
		sem_wait(semRe->semaforo);
	}

}


void procesarPeticion(arg_struct* argumentos) {

	int bytesARecibir;
	recv(argumentos->conexion, &bytesARecibir, sizeof(int), MSG_WAITALL);
	void* buffer = malloc(bytesARecibir);
	recv(argumentos->conexion, buffer, bytesARecibir, 0);

	int desplazamiento = 0;
	tipoDePeticion peticion;
	memcpy(&peticion, buffer, sizeof(tipoDePeticion));
	desplazamiento += sizeof(tipoDePeticion);

	mate_instanceACambiar* carpincho = desserializarCarpincho(buffer);
	int bytesCarpincho = sizeDeCarpincho(carpincho);
	desplazamiento += bytesCarpincho;

//	printf("Holaa llegur con peticion, %d y ID: %d \n", peticion, carpincho->id);

	if (peticion == NUEVOPROCESO) {

		cargarNuevoProceso(carpincho);

		actualizarCarpincho(carpincho);

		bool encontrarCarpi(semReady* semRe){
			return carpincho->id == semRe->id;
		}

		sem_wait(semaforoDeSemaforo);
		semReady* semRe = list_find(listaSemaforosReady, (void*) encontrarCarpi);
		sem_post(semaforoDeSemaforo);

		semRe->estoyBloqueado = true;
		sem_wait(semRe->semaforo);

		int cantidadBytesCarpi = sizeDeCarpincho(carpincho);
		void* bufferSendMemoria = malloc(cantidadBytesCarpi+sizeof(tipoDePeticion));
		void* SendMemoria = serializarCarpincho(carpincho);
		memcpy(bufferSendMemoria, &peticion, sizeof(tipoDePeticion));
		memcpy(bufferSendMemoria+sizeof(tipoDePeticion), SendMemoria, cantidadBytesCarpi);

		int conexionMemoria = crearConexion(info.ipMemoria, info.puertoMemoria);

		int b = cantidadBytesCarpi+sizeof(tipoDePeticion);
		send(conexionMemoria, &b, sizeof(int), 0);
		send(conexionMemoria, bufferSendMemoria, b, 0);

		//Devuelve nuevo carpincho
		void* bufferSend = serializarCarpincho(carpincho);
		int cantBytes = sizeDeCarpincho(carpincho);

		send(argumentos->conexion, &cantBytes, sizeof(int), 0);
		send(argumentos->conexion, bufferSend, cantBytes, 0);

		free(bufferSend);
		free(SendMemoria);
		free(bufferSendMemoria);


	} else if (peticion == NUEVOSEMAFORO) {

		int value;
		memcpy(&value, buffer + desplazamiento, sizeof(int));
		desplazamiento += sizeof(int);

		int largoSem;
		memcpy(&largoSem, buffer + desplazamiento, sizeof(int));
		desplazamiento += sizeof(int);

		mate_sem_name sem = malloc(largoSem);
		memcpy(sem, buffer + desplazamiento, largoSem);
		desplazamiento += largoSem;
		//sem = string_substring(sem, 0, largoSem);

		esperarAExec(carpincho->id);

		nuevoSemaforo(sem, value);

		esperarAExec(carpincho->id);

		//Devuelve nuevo carpincho
		void* bufferSend = serializarCarpincho(carpincho);
		int cantBytes = sizeDeCarpincho(carpincho);
		send(argumentos->conexion, &cantBytes, sizeof(int), 0);
		send(argumentos->conexion, bufferSend, cantBytes, 0);

		free(bufferSend);
		free(sem);
	} else if (peticion == SEMWAIT) {

		int largoSem;
		memcpy(&largoSem, buffer + desplazamiento, sizeof(int));
		desplazamiento += sizeof(int);

		mate_sem_name sem = malloc(largoSem);
		memcpy(sem, buffer + desplazamiento, largoSem);
		desplazamiento += largoSem;

		esperarAExec(carpincho->id);

		semWait(sem, carpincho, argumentos->conexion);

		esperarAExec(carpincho->id);

		//Devuelve nuevo carpincho
		void* bufferSend = serializarCarpincho(carpincho);
		int cantBytes = sizeDeCarpincho(carpincho);
		send(argumentos->conexion, &cantBytes, sizeof(int), 0);
		send(argumentos->conexion, bufferSend, cantBytes, 0);

		free(bufferSend);
		free(sem);
	} else if (peticion == SEMPOST) {

		int largoSem;
		memcpy(&largoSem, buffer + desplazamiento, sizeof(int));
		desplazamiento += sizeof(int);

		mate_sem_name sem = malloc(largoSem);
		memcpy(sem, buffer + desplazamiento, largoSem);
		desplazamiento += largoSem;

		esperarAExec(carpincho->id);

		semPost(sem, carpincho);

		esperarAExec(carpincho->id);

		//Devuelve nuevo carpincho
		void* bufferSend = serializarCarpincho(carpincho);
		int cantBytes = sizeDeCarpincho(carpincho);
		send(argumentos->conexion, &cantBytes, sizeof(int), 0);
		send(argumentos->conexion, bufferSend, cantBytes, 0);

		free(bufferSend);
		free(sem);
	} else if (peticion == ELIMINARSEMAFORO) {

		int largoSem;
		memcpy(&largoSem, buffer + desplazamiento, sizeof(int));
		desplazamiento += sizeof(int);

		mate_sem_name sem = malloc(largoSem);
		memcpy(sem, buffer + desplazamiento, largoSem);
		desplazamiento += largoSem;

		esperarAExec(carpincho->id);

		eliminarDeRecursosTotales(sem);

		esperarAExec(carpincho->id);

		//Devuelve nuevo carpincho
		void* bufferSend = serializarCarpincho(carpincho);
		int cantBytes = sizeDeCarpincho(carpincho);
		send(argumentos->conexion, &cantBytes, sizeof(int), 0);
		send(argumentos->conexion, bufferSend, cantBytes, 0);

		free(bufferSend);
		free(sem);
	} else if (peticion == NUEVAIO) {

		int bytesIO;
		memcpy(&bytesIO, buffer + desplazamiento, sizeof(int));
		desplazamiento += sizeof(int);

		mate_io_resource io = malloc(bytesIO);
		memcpy(io, buffer + desplazamiento, bytesIO);
		desplazamiento += bytesIO;

		esperarAExec(carpincho->id);

		carpinchoPideIO(io, carpincho);

		esperarAExec(carpincho->id);

		//Devuelve nuevo carpincho
		void* bufferSend = serializarCarpincho(carpincho);
		int cantBytes = sizeDeCarpincho(carpincho);
		send(argumentos->conexion, &cantBytes, sizeof(int), 0);
		send(argumentos->conexion, bufferSend, cantBytes, 0);

		free(bufferSend);
		free(io);
	} else if (peticion == PEDIRMEMORIA) {

		desplazamiento += sizeof(int);

		esperarAExec(carpincho->id);

		int conexionMemoria = crearConexion(info.ipMemoria, info.puertoMemoria);

		send(conexionMemoria, &bytesARecibir, sizeof(int), 0);
		send(conexionMemoria, buffer, desplazamiento, 0);

		mate_pointer direccion_alloc;
		recv(conexionMemoria, &direccion_alloc, sizeof(mate_pointer), MSG_WAITALL);

		esperarAExec(carpincho->id);

		send(argumentos->conexion, &direccion_alloc, sizeof(mate_pointer), 0);

		close(conexionMemoria);
	} else if (peticion == LEERMEMORIA) {

		int size;
		memcpy(&size, buffer + desplazamiento + sizeof(mate_pointer), sizeof(int));
		desplazamiento += sizeof(mate_pointer) + sizeof(int) + size;

		int conexionMemoria = crearConexion(info.ipMemoria, info.puertoMemoria);

		esperarAExec(carpincho->id);

		send(conexionMemoria, &bytesARecibir, sizeof(int), 0);
		send(conexionMemoria, buffer, desplazamiento, 0);

		void* dest = malloc(size);
		int respuesta;
		recv(conexionMemoria, &respuesta, sizeof(int), MSG_WAITALL);
		recv(conexionMemoria, dest, size, 0);


		esperarAExec(carpincho->id);

		int* mensj = malloc(size);
		memcpy(mensj, dest, size);
		if(dest == NULL){
			printf("DEST ES NULL\n");
		}
		printf("MENSAJE LEIDO: %d CON TAMANIO: %d\n", *mensj, size);
		send(argumentos->conexion, &respuesta, sizeof(int), 0);
		send(argumentos->conexion, dest, size, 0);

		close(conexionMemoria);
		free(dest);
	} else if (peticion == ESCRIBIRMEMORIA) {
		int size;
		memcpy(&size, buffer + desplazamiento + sizeof(mate_pointer),sizeof(int));
		desplazamiento += sizeof(mate_pointer) + sizeof(int) + size;

		int conexionMemoria = crearConexion(info.ipMemoria, info.puertoMemoria);

		esperarAExec(carpincho->id);
		int* mensj = malloc(size);
		int desp = desplazamiento-size;
		memcpy(mensj, buffer+desp, size);
		printf("Recivo un mensaje tam: %d y dice: %d\n", size, *mensj);
		send(conexionMemoria, &bytesARecibir, sizeof(int), 0);
		send(conexionMemoria, buffer, desplazamiento, 0);

		int respuesta;
		recv(conexionMemoria, &respuesta, sizeof(int), MSG_WAITALL);

		esperarAExec(carpincho->id);

		send(argumentos->conexion, &respuesta, sizeof(int), 0);

		close(conexionMemoria);
	} else if (peticion == LIBERARMEMORIA) {

		desplazamiento += sizeof(mate_pointer);

		int conexionMemoria = crearConexion(info.ipMemoria, info.puertoMemoria);

		esperarAExec(carpincho->id);

		send(conexionMemoria, &bytesARecibir, sizeof(int), 0);
		send(conexionMemoria, buffer, desplazamiento, 0);

		int respuesta;
		recv(conexionMemoria, &respuesta, sizeof(int), MSG_WAITALL);

		esperarAExec(carpincho->id);

		send(argumentos->conexion, &respuesta, sizeof(int), 0);

		close(conexionMemoria);
	} else if (peticion == LIBERARPROCESO) {

		esperarAExec(carpincho->id);

		int cantidadBytesCarpi = sizeDeCarpincho(carpincho);
		void* bufferSendMemoria = malloc(cantidadBytesCarpi+sizeof(tipoDePeticion));
		void* sendMemoria = serializarCarpincho(carpincho);
		memcpy(bufferSendMemoria, &peticion, sizeof(tipoDePeticion));
		memcpy(bufferSendMemoria+sizeof(tipoDePeticion), sendMemoria, cantidadBytesCarpi);

		int conexionMemoria = crearConexion(info.ipMemoria, info.puertoMemoria);

		int b = cantidadBytesCarpi+sizeof(tipoDePeticion);
		send(conexionMemoria, &b, sizeof(int), 0);
		send(conexionMemoria, bufferSendMemoria, b, 0);

		//PASAR A EXIT PROCESO Y VACIAR MEMORIA
		finalizarCarpincho(carpincho);

		free(bufferSendMemoria);
		free(sendMemoria);
	}


	free(buffer);
	liberarCarpincho(carpincho);
	close(argumentos->conexion);
	free(argumentos);
}

void nuevoHilo(int cliente) {

	pthread_t newHilo;

	arg_struct* args = malloc(sizeof(arg_struct));
	args->conexion = cliente;

	pthread_create(&newHilo, NULL, (void*) procesarPeticion, (void*) args);
	pthread_detach(newHilo);

}

void liberarConfig(infoConfig info) {
	free(info.ipKernel);
	free(info.puertoKernel);
	free(info.ipMemoria);
	free(info.puertoMemoria);
	free(info.moduloAConectar);
}

infConf obtenerInfoConfig() {
	infConf infoKernel;

	infoKernel.ipMemoria = config_get_string_value(configKernel, "IP_MEMORIA");
	infoKernel.ip = config_get_string_value(configKernel, "IP");
	infoKernel.puertoMemoria = config_get_string_value(configKernel, "PUERTO_MEMORIA");
	infoKernel.puertoEscucha = config_get_string_value(configKernel, "PUERTO_ESCUCHA");
	infoKernel.algoritmo = config_get_string_value(configKernel, "ALGORITMO_PLANIFICACION");
	infoKernel.estimacionInicial = config_get_int_value(configKernel, "ESTIMACION_INICIAL");
	infoKernel.alfa = config_get_double_value(configKernel, "ALFA");

	char** f = config_get_array_value(configKernel, "DISPOSITIVOS_IO");
	infoKernel.dispositivosIO = arrayATlist(f);
	char** l = config_get_array_value(configKernel, "DURACIONES_IO");
	infoKernel.duracionesIO = arrayATlistDeInts(l);

	infoKernel.tiempoDeadlock = config_get_int_value(configKernel,"TIEMPO_DEADLOCK");
	infoKernel.gradoMultiprogramacion = config_get_int_value(configKernel,"GRADO_MULTIPROGRAMACION");
	infoKernel.gradoMultiprocesamiento = config_get_int_value(configKernel,"GRADO_MULTIPROCESAMIENTO");

	frezee(f); //Si ponemos este free rompe la lista de dispositivosIO
	frezee(l);
	
//	mostrarTlistint(infoKernel.duracionesIO);
//	mostrarTlist(infoKernel.dispositivosIO);

	return infoKernel;
}

void mostrarTlistint(t_list* lista){
	int t = list_size(lista);
	int i;
	for(i=0; i<t; i++){
		int* ab = list_get(lista, i);
		printf("Valor: %d\n", *ab);
	}
}


void mostrarTlist(t_list* lista){
	int t = list_size(lista);
	int i;
	for(i=0; i<t; i++){
		char* ab = list_get(lista, i);
		printf("Valor: %s\n", ab);
	}
}


int sizeConfig(infoConfig info) {

	int largoIpKernel = string_length(info.ipKernel);
	int largoPuertoKernel = string_length(info.puertoKernel);
	int largoIpMemoria = string_length(info.ipMemoria);
	int largopuertoMemoria = string_length(info.puertoMemoria);
	int largoModulo = string_length(info.moduloAConectar);

	return largoIpKernel + largoPuertoKernel + largoIpMemoria + largopuertoMemoria + largoModulo;
}

tamConfig valoresConfig(infoConfig info) {
	tamConfig valores;
	valores.largoIpKernel = string_length(info.ipKernel);
	valores.largoPuertoKernel = string_length(info.puertoKernel);
	valores.largoIpMemoria = string_length(info.ipMemoria);
	valores.largoPuertoMemoria = string_length(info.puertoMemoria);
	valores.largomoduloAConectar = string_length(info.moduloAConectar);
	return valores;

}

void* serializarSizeConfig(infoConfig info) {
	tamConfig asignarValores = valoresConfig(info);
	void* buffer = malloc(sizeof(tamConfig));
	int desplazamiento = 0;

	memcpy(buffer, &(asignarValores.largoIpKernel), sizeof(int));
	desplazamiento += sizeof(int);

	memcpy(buffer + desplazamiento, &(asignarValores.largoPuertoKernel),
			sizeof(int));
	desplazamiento += sizeof(int);

	memcpy(buffer + desplazamiento, &(asignarValores.largoIpMemoria),
			sizeof(int));
	desplazamiento += sizeof(int);

	memcpy(buffer + desplazamiento, &(asignarValores.largoPuertoMemoria),
			sizeof(int));
	desplazamiento += sizeof(int);

	memcpy(buffer + desplazamiento, &(asignarValores.largomoduloAConectar),
			sizeof(int));
	desplazamiento += sizeof(int);

	return buffer;
}

tamConfig desserializarSizeConfig(void* buffer, int desplazamiento) {
	tamConfig asignarValores;

	memcpy(&(asignarValores.largoIpKernel), buffer + desplazamiento,
			sizeof(int));
	desplazamiento += sizeof(int);

	memcpy(&(asignarValores.largoPuertoKernel), buffer + desplazamiento,
			sizeof(int));
	desplazamiento += sizeof(int);

	memcpy(&(asignarValores.largoIpMemoria), buffer + desplazamiento,
			sizeof(int));
	desplazamiento += sizeof(int);

	memcpy(&(asignarValores.largoPuertoMemoria), buffer + desplazamiento,
			sizeof(int));
	desplazamiento += sizeof(int);

	memcpy(&(asignarValores.largomoduloAConectar), buffer + desplazamiento,
			sizeof(int));
	desplazamiento += sizeof(int);

	return asignarValores;
}

infoConfig desserializarConfig(void* buffer, int desplazamiento, tamConfig tam) {
	infoConfig info;
	info.ipKernel = malloc(tam.largoIpKernel);
	info.puertoKernel = malloc(tam.largoPuertoKernel);
	info.ipMemoria = malloc(tam.largoIpMemoria);
	info.puertoMemoria = malloc(tam.largoPuertoMemoria);
	info.moduloAConectar = malloc(tam.largomoduloAConectar);

	memcpy(info.ipKernel, buffer + desplazamiento, tam.largoIpKernel);
	desplazamiento += tam.largoIpKernel;

	memcpy(info.puertoKernel, buffer + desplazamiento, tam.largoPuertoKernel);
	desplazamiento += tam.largoPuertoKernel;

	memcpy(info.ipMemoria, buffer + desplazamiento, tam.largoIpMemoria);
	desplazamiento += tam.largoIpMemoria;

	memcpy(info.puertoMemoria, buffer + desplazamiento, tam.largoPuertoMemoria);
	desplazamiento += tam.largoPuertoMemoria;

	memcpy(info.moduloAConectar, buffer + desplazamiento, tam.largomoduloAConectar);
	desplazamiento += tam.largomoduloAConectar;

	infoConfig configLimpio = limpiarConfig(info, tam);

	liberarConfig(info);

	return configLimpio;
}

void* serializarConfig(infoConfig info) {
	void* buffer = malloc(sizeConfig(info));
	int desplazamiento = 0;

	memcpy(buffer, info.ipKernel, string_length(info.ipKernel));
	desplazamiento += string_length(info.ipKernel);

	memcpy(buffer + desplazamiento, info.puertoKernel,
			string_length(info.puertoKernel));
	desplazamiento += string_length(info.puertoKernel);

	memcpy(buffer + desplazamiento, info.ipKernel,
			string_length(info.ipMemoria));
	desplazamiento += string_length(info.ipMemoria);

	memcpy(buffer + desplazamiento, info.puertoKernel,
			string_length(info.puertoMemoria));
	desplazamiento += string_length(info.puertoMemoria);

	memcpy(buffer + desplazamiento, info.moduloAConectar,
			string_length(info.moduloAConectar));
	desplazamiento += string_length(info.moduloAConectar);

	return buffer;
}

void* serializarCarpincho(mate_instanceACambiar *lib_ref) {
	void* buffer = malloc(sizeDeCarpincho(lib_ref));
	int desplazamiento = 0;

	memcpy(buffer, &(lib_ref->id), sizeof(int));
	desplazamiento += sizeof(int);
	memcpy(buffer + desplazamiento, &(lib_ref->estimacionProximaRafaga), sizeof(float));
	desplazamiento += sizeof(float);
	memcpy(buffer + desplazamiento, &(lib_ref->tasaRespuesta), sizeof(float));
	desplazamiento += sizeof(float);
	memcpy(buffer + desplazamiento, &(lib_ref->tiempoInicialEnReady), sizeof(time_t));
	desplazamiento += sizeof(time_t);
	memcpy(buffer + desplazamiento, &(lib_ref->tiempoInicialEnExec), sizeof(time_t));
	desplazamiento += sizeof(time_t);
	memcpy(buffer + desplazamiento, &(lib_ref->tiempoDeEjecucionRafagaAnterior), sizeof(time_t));
	desplazamiento += sizeof(time_t);
	memcpy(buffer + desplazamiento, &(lib_ref->conexionMemoria), sizeof(int));
	desplazamiento += sizeof(int);

	void* tamSerializado = serializarSizeConfig(lib_ref->info);

	memcpy(buffer+desplazamiento, tamSerializado, sizeof(tamConfig));
	desplazamiento += sizeof(tamConfig);

	void* configSerializado = serializarConfig(lib_ref->info);

	memcpy(buffer+desplazamiento, configSerializado, sizeConfig(lib_ref->info));
	desplazamiento += sizeConfig(lib_ref->info);

	free(tamSerializado);
	free(configSerializado);

	return buffer;
}

mate_instanceACambiar* desserializarCarpincho(void* buffer) {
	mate_instanceACambiar* lib_ref = malloc(sizeof(mate_instanceACambiar));
	int desplazamiento = 4;

	memcpy(&(lib_ref->id), buffer+desplazamiento, sizeof(int));
	desplazamiento += sizeof(int);
	memcpy(&(lib_ref->estimacionProximaRafaga), buffer + desplazamiento, sizeof(float));
	desplazamiento += sizeof(float);
	memcpy(&(lib_ref->tasaRespuesta), buffer + desplazamiento, sizeof(float));
	desplazamiento += sizeof(float);
	memcpy(&(lib_ref->tiempoInicialEnReady), buffer + desplazamiento, sizeof(time_t));
	desplazamiento += sizeof(time_t);
	memcpy(&(lib_ref->tiempoInicialEnExec), buffer + desplazamiento, sizeof(time_t));
	desplazamiento += sizeof(time_t);
	memcpy(&(lib_ref->tiempoDeEjecucionRafagaAnterior), buffer + desplazamiento, sizeof(time_t));
	desplazamiento += sizeof(time_t);
	memcpy(&(lib_ref->conexionMemoria), buffer + desplazamiento, sizeof(int));
	desplazamiento += sizeof(int);

	tamConfig tam = desserializarSizeConfig(buffer, desplazamiento);
	desplazamiento += sizeof(tamConfig);
	lib_ref->sizeConfig = tam;

	int tamanioConf = tam.largoIpKernel+tam.largoIpMemoria+tam.largoPuertoKernel+tam.largoPuertoMemoria+tam.largomoduloAConectar;

	infoConfig info = desserializarConfig(buffer, desplazamiento, tam);
	desplazamiento += tamanioConf;
	lib_ref->info = info;

	return lib_ref;
}

infoConfig limpiarConfig(infoConfig info, tamConfig tam) {
	infoConfig infoLimpio;

	infoLimpio.ipKernel = string_substring_until(info.ipKernel, tam.largoIpKernel);
	infoLimpio.puertoKernel = string_substring_until(info.puertoKernel, tam.largoPuertoKernel);
	infoLimpio.ipMemoria = string_substring_until(info.ipMemoria, tam.largoIpMemoria);
	infoLimpio.puertoMemoria = string_substring_until(info.puertoMemoria, tam.largoPuertoMemoria);
	infoLimpio.moduloAConectar = string_substring_until(info.moduloAConectar, tam.largomoduloAConectar);

	return infoLimpio;
}

mate_pointer* desserializarPointer(void* buffer) {
	mate_pointer* pointer = malloc(sizeof(mate_pointer));
	int desplazamiento = sizeof(tipoDePeticion) + sizeof(mate_instanceACambiar);

	memcpy(&(pointer), buffer + desplazamiento, sizeof(int32_t));
	desplazamiento += sizeof(int);

	return pointer;
}

//FUNCIONES PARA LA CONEXION

int iniciarServidor(char* ip, char* puerto) {
		int socketServidor;
	    struct addrinfo hints, *servinfo, *p;

	    memset(&hints, 0, sizeof(hints));
	    hints.ai_family = AF_UNSPEC;
	    hints.ai_socktype = SOCK_STREAM;
	    hints.ai_flags = AI_PASSIVE;

	    getaddrinfo(ip, puerto, &hints, &servinfo);

	    for (p=servinfo; p != NULL; p = p->ai_next)
	    {
	        if ((socketServidor = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
	            continue;

	        if (bind(socketServidor, p->ai_addr, p->ai_addrlen) == -1) {
	        	printf("%s\n", strerror(errno));
	        	close(socketServidor);
	            continue;
	        }
	        break;
	    }


		listen(socketServidor, SOMAXCONN);

	    freeaddrinfo(servinfo);

	    return socketServidor;
}


int esperarCliente(int socketServidor) {
	struct sockaddr_in dirCliente;
	socklen_t tamDireccion = sizeof(struct sockaddr_in);

	int socketCliente = accept(socketServidor, (void*) &dirCliente, &tamDireccion);
//	int socketCliente = accept(socketServidor, NULL, NULL);

	//printf("%s\n", strerror(errno));

	return socketCliente;
}

//int crearConexion(char* ip, char* puerto, t_log* logKernel) {
//	struct addrinfo hints;
//	struct addrinfo *server_info;
//	int clientSocket;
//
//	memset(&hints, 0, sizeof(hints));
//	hints.ai_family = AF_UNSPEC;
//	hints.ai_socktype = SOCK_STREAM;
//	hints.ai_flags = AI_PASSIVE;
//
//	if (getaddrinfo(ip, puerto, &hints, &server_info)) {
//		log_error(logKernel, "ERROR EN GETADDRINFO");
//		exit(-1);
//	}
//
//	clientSocket = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);
//
//	if (connect(clientSocket, server_info->ai_addr, server_info->ai_addrlen) == -1) {
//		printf("IP: %s, PUERTO: %s \n",ip,puerto);
//		printf("SOCKET: %s\n", strerror(errno));
//		log_error(logKernel, "FALLÓ CONEXIÓN ENTRE KERNEL Y LA MEMORIA...");
//		perror("ERROR: ");
//		exit(-1);
//	}
//
//	log_info(logKernel, "CONEXIÓN EXITOSA...");
//
//	freeaddrinfo(server_info);
//	return clientSocket;
//}


// CLIENTE SE INTENTA CONECTAR A SERVER ESCUCHANDO EN IP:PUERTO
//int crearConexion(char* ip, char* puerto, t_log* logger) {
//    struct addrinfo hints, *servinfo;
//
//    // Init de hints
//    memset(&hints, 0, sizeof(hints));
//    hints.ai_family = AF_UNSPEC;
//    hints.ai_socktype = SOCK_STREAM;
//    hints.ai_flags = AI_PASSIVE;
//
//    // Recibe addrinfo
//    getaddrinfo(ip, puerto, &hints, &servinfo);
//
//    // Crea un socket con la informacion recibida (del primero, suficiente)
//    int socket_cliente = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
//
//    // Fallo en crear el socket
//    if(socket_cliente == -1) {
//    	printf("IP: %s, PUERTO: %s \n",ip,puerto);
//    	printf("SOCKET: %s\n", strerror(errno));
//        log_error(logger, "Error creando el socket para %s:%s", ip, puerto);
//        perror("ERROR: ");
//        exit(-1);
//    }
//
//    // Error conectando
//    if(connect(socket_cliente, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
//
//    	printf("IP: %s, PUERTO: %s \n",ip,puerto);
//    	printf("SOCKET: %s\n", strerror(errno));
//    	log_error(logKernel, "FALLÓ CONEXIÓN ENTRE KERNEL Y LA MEMORIA...");
//    	perror("ERROR: ");
//
//    	freeaddrinfo(servinfo);
//        exit(-1);
//    } else
////        log_info(logger, "Cliente conectado en %s:%s (a %s)\n", ip, puerto, server_name);
//
//    freeaddrinfo(servinfo); //free
//
//    return socket_cliente;
//}

int crearConexion(char* ip, char* puerto){
	  	struct addrinfo hints;
	    struct addrinfo *server_info;
	    int clientSocket;

	    memset(&hints, 0, sizeof(hints));
	    hints.ai_family = AF_UNSPEC;
	    hints.ai_socktype = SOCK_STREAM;
	    hints.ai_flags = AI_PASSIVE;

	    if(getaddrinfo(ip, puerto, &hints, &server_info)){
	    	exit(-1);
	    }

	    clientSocket = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);

	    if(connect(clientSocket, server_info->ai_addr, server_info->ai_addrlen) == -1){
	    	printf("FALLO AL CREAR CONEXION ENTRE MEMORIA Y KERNEL\n");
	    	perror("ERROR: ");
	        exit(-1);
	    }


	    freeaddrinfo(server_info);
	    return clientSocket;
}





void inicializarListaDeEstados() {
	listaDeCarpinchos = list_create();
	listaSemaforosReady = list_create();
	listaSemaforosExec = list_create();

	new = queue_create();
	ready = list_create();
	exec = list_create();
	blocked = list_create();
	suspendBlocked = list_create();
	suspendReady = queue_create();
	exitState = list_create();

}

void cargarNuevoProceso(mate_instanceACambiar* proceso) {

	proceso->id = generarId();
	proceso->tiempoDeEjecucionRafagaAnterior = 0;

	int* id = malloc(sizeof(int));
	memcpy(id, &proceso->id, sizeof(int));

	sem_wait(modificandoEstados);
	queue_push(new, id);
	sem_post(modificandoEstados);

	sem_wait(semListaCarpinchos);
	list_add(listaDeCarpinchos, proceso);
	sem_post(semListaCarpinchos);

	log_info(logKernel,"El Carpincho id:%d entra en NEW ",*id);

	semReady* semIdR = malloc(sizeof(semReady));
	semIdR->id = proceso->id;
	semIdR->estoyBloqueado = false;
	semIdR->semaforo = malloc(sizeof(sem_t));
	sem_init(semIdR->semaforo, 0, 0);

	sem_wait(semaforoDeSemaforo);
	list_add(listaSemaforosReady, semIdR);
	sem_post(semaforoDeSemaforo);

	semReady* semIdE = malloc(sizeof(semReady));
	semIdE->id = proceso->id;
	semIdE->estoyBloqueado = false;
	semIdE->semaforo = malloc(sizeof(sem_t));
	sem_init(semIdE->semaforo, 0, 0);

	sem_wait(semaforoDeSemaforosExec);
	list_add(listaSemaforosExec, semIdE);
	sem_post(semaforoDeSemaforosExec);


	sem_wait(modificandoEstados);
	if(queue_size(suspendReady) == 0 && queue_size(new) == 1){
		sem_post(semNewReady);
	}
	sem_post(modificandoEstados);

	sem_wait(modificandoEstados);
	if(queue_size(new) == 1 && !list_is_empty(blocked) && list_is_empty(ready)){
		sem_post(semBlokedSuspend);
	}
	sem_post(modificandoEstados);

}

void transicionNewReady() {



	while (1) {

		sem_wait(modificandoEstados);
		bool estaLlenoSus = !queue_is_empty(suspendReady);
		bool estaLlenoNew = !queue_is_empty(new);
		sem_post(modificandoEstados);

		if (estaLlenoSus) { //La cola de suspendidos tiene mayor prioridad

			sem_wait(carpinchos_listos_para_ejecutar);

			sem_wait(modificandoEstados);
			int* idCarpi = queue_pop(suspendReady);
			sem_post(modificandoEstados);

			int* idCopy = malloc(sizeof(int));
			memcpy(idCopy, idCarpi, sizeof(int));

			sem_wait(modificandoEstados);
			list_add(ready, idCopy);
			sem_post(modificandoEstados);


			tiempoInicialEnReady(*idCopy);
			mate_instanceACambiar* carpincho = obtenerCarpincho(*idCopy);
			estimarRafaga(carpincho);
			log_info(logKernel,"El Carpincho id:%d entra en Ready ",*idCopy);
			//free(idCarpi);

			sem_wait(modificandoEstados);
			if (list_size(ready) == 1) {
				sem_post(semReadyExec);
			}
			sem_post(modificandoEstados);

			bool encontrarCarpi(semReady* sem){
				return *idCarpi == sem->id;
			}

			free(idCarpi);


		} else if (estaLlenoNew) {

			sem_wait(carpinchos_listos_para_ejecutar);

			sem_wait(modificandoEstados);
			int* idCarpi = queue_pop(new);
			int* idCopy = malloc(sizeof(int));
			memcpy(idCopy, idCarpi, sizeof(int));
			list_add(ready, idCopy);
			sem_post(modificandoEstados);

			free(idCarpi);

			tiempoInicialEnReady(*idCopy);
			mate_instanceACambiar* carpincho = obtenerCarpincho(*idCopy);
			estimarRafaga(carpincho);

			log_info(logKernel,"El Carpincho id:%d entra en Ready ",*idCopy);

			sem_wait(modificandoEstados);
			if (list_size(ready) == 1) {
				sem_post(semReadyExec);
			}
			sem_post(modificandoEstados);


			bool encontrarCarpi(semReady* sem){
				return *idCopy == sem->id;
			}

			sem_wait(semaforoDeSemaforo);
			semReady* semRe = list_find(listaSemaforosReady, (void*) encontrarCarpi);
			sem_post(semaforoDeSemaforo);


			sem_post(semRe->semaforo);

//			log_info(logKernel,"El Carpincho id:%d entra en Ready ",*idCopy);

		}else{
			sem_wait(semNewReady);
		}

	}

}

void estimarRafaga(mate_instanceACambiar* proceso) {

	double tiempoDeEjecucion = proceso->tiempoDeEjecucionRafagaAnterior;
	double alfa = info.alfa;

	if (tiempoDeEjecucion == 0) {
		proceso->estimacionProximaRafaga = info.estimacionInicial / 1000;
	} else {
		float resultado =  alfa * tiempoDeEjecucion + (1 - alfa) * proceso->estimacionProximaRafaga;
		proceso->estimacionProximaRafaga = resultado;
	}

}

void calcularTasaRespuesta(mate_instanceACambiar* proceso) {//hrrn

	time_t tiempoInicialEnReady = proceso->tiempoInicialEnReady;
	double estimacionRafaga = proceso->estimacionProximaRafaga;

	double tiempoEnEspera = difftime(time(NULL), tiempoInicialEnReady);

	double resultado = (tiempoEnEspera/ estimacionRafaga  ) + 1;

	proceso->tasaRespuesta = (float)resultado;

}

void calcularRafagaEjecucion(int idProceso) {
	mate_instanceACambiar* proceso = obtenerCarpincho(idProceso);

	time_t tiempoInicialEnExec = proceso->tiempoInicialEnExec;
	time_t tiempoActual = time(NULL);
	double tiempoDeEjecucionActual= difftime(tiempoActual,tiempoInicialEnExec);
	proceso->tiempoDeEjecucionRafagaAnterior = tiempoDeEjecucionActual;
//	printf("tiempo ejecucion Actual: %f tiempo actual: %ld \n",tiempoDeEjecucionActual,tiempoActual);
//	actualizarCarpincho(proceso);

}

void transicionReadyExec() {

	/////////////////////Comparador SJF///////////////////

	mate_instanceACambiar* minimoRafagaSJF(mate_instanceACambiar* procesoA, mate_instanceACambiar* procesoB) {
		if (procesoA->estimacionProximaRafaga <= procesoB->estimacionProximaRafaga) {
			return procesoA;
		}
		return procesoB;
	}
	////////////////////////////////////////////////////////

	////////////////Comparador HRRN///////////////////

	mate_instanceACambiar* mayorTiempoRespuestaHRRN(mate_instanceACambiar* procesoA, mate_instanceACambiar* procesoB) {
		if (procesoA->tasaRespuesta > procesoB->tasaRespuesta) {
			return procesoA;
		}
		return procesoB;
	}
	////////////////////////////////////////////////////////

	while (1) {

		sem_wait(modificandoEstados);
		bool isReadyEmpty = !list_is_empty(ready);
		sem_post(modificandoEstados);

		if (isReadyEmpty) {

			sem_wait(modificandoEstados);
			int sizeExec = list_size(exec);
			sem_post(modificandoEstados);

//			printf("MULTIPROCESAMIENTO ANTES DEL WAIT: %d \n", gradoMProcesamiento);
			if(sizeExec >= info.gradoMultiprocesamiento){
				//printf("No tengo mas lugar: sizeExec=%d \n", sizeExec);
				sem_wait(carpinchos_en_ejecucion);
			}
//			printf("MULTIPROCESAMIENTO DESPUES DEL WAIT: %d \n", gradoMProcesamiento);

			char* algoritmoPlanificador = info.algoritmo;

			sem_wait(modificandoEstados);

//			t_list* listaTasaAntes = carpinchosEnReady();
//			printf("-------------------------ANTES -------------------------------");
//			mostrarTiempos(listaTasaAntes);

//			estimarRafagaDeTodosEnReady();
			calcularTasaRespuestaDeTodosEnReady();
			t_list* listaTasa = carpinchosEnReady();

//			printf("-------------------------DESPUES-------------------------------\n");
//			mostrarTiempos(listaTasa);
//			printf("---------------------------------------------------------------\n");

			sem_post(modificandoEstados);

			/*t_list* listaDeCarpinchosEnReady = carpinchosEnReady();
			printf("-------------------------ANTES -------------------------------");
			mostrarTiempos(listaDeCarpinchosEnReady);

			sem_wait(modificandoEstados);
			t_list* listaRafaga = list_map(listaDeCarpinchosEnReady, (void*) estimarRafaga);
			t_list* listaTasa = list_map(listaRafaga, (void*) calcularTasaRespuesta);
			sem_post(modificandoEstados);
			printf("-------------------------DESPUES-------------------------------\n");
			mostrarTiempos(listaTasa);
			printf("---------------------------------------------------------------\n");*/
			mate_instanceACambiar* procesoAEjecutar;

			if (string_equals_ignore_case(algoritmoPlanificador, "SJF")) {
				sem_wait(modificandoEstados);
				procesoAEjecutar = list_get_minimum(listaTasa, (void*) minimoRafagaSJF);
				printf("Elige al carpincho id: %d \n", procesoAEjecutar->id);
				sem_post(modificandoEstados);
			} else {
				sem_wait(modificandoEstados);
				procesoAEjecutar = list_get_maximum(listaTasa, (void*) mayorTiempoRespuestaHRRN);
				printf("Elige al carpincho id: %d \n", procesoAEjecutar->id);
				sem_post(modificandoEstados);
			}

			int* idCopy = malloc(sizeof(int));
			memcpy(idCopy, &procesoAEjecutar->id, sizeof(int));

			printf("ID: %d, va a pasar de Ready a Exec \n", *idCopy);

			bool encontrateElCarpi(int* id) {
				return procesoAEjecutar->id == *id;
			}

			sem_wait(modificandoEstados);
			list_add(exec, idCopy);
			list_remove_by_condition(ready, (void*) encontrateElCarpi);//and destroy
			sem_post(modificandoEstados);

//			procesoAEjecutar->tiempoInicialEnExec = time(NULL);
//			actualizarCarpincho(procesoAEjecutar);

			tiempoInicialEnExec(procesoAEjecutar->id);
			log_info(logKernel,"El Carpincho id:%d entra en EXEC \n",procesoAEjecutar->id);

			bool encontrarCarpi(semReady* semR){
				return semR->id == procesoAEjecutar->id;
			}

			sem_wait(semaforoDeSemaforosExec);
			semReady* semRe = list_find(listaSemaforosExec, (void*) encontrarCarpi);
			sem_post(semaforoDeSemaforosExec);


			if(semRe->estoyBloqueado){
				sem_post(semRe->semaforo);
				semRe->estoyBloqueado = false;
			}


			sem_wait(modificandoEstados);
			if(!queue_is_empty(new) && !list_is_empty(blocked) && list_is_empty(ready) ){
				sem_post(semBlokedSuspend);
			}
			sem_post(modificandoEstados);

//			list_destroy(listaTasaAntes);
			//list_destroy(listaRafaga);
			list_destroy(listaTasa);
		} else {
			//semaforoWait
			sem_wait(semReadyExec);
		}

	}

}

void mostrarTiempos(t_list* listaCarpinchos){
	int i = 0;
	while( i < list_size(listaCarpinchos)){
		mate_instanceACambiar* carpi = list_get(listaCarpinchos,i);
		printf("\nTIEMPOS Id: %d \n Rafaga: %.2f \n TasaRespuesta:%.2f \n\n ",carpi->id,carpi->estimacionProximaRafaga, carpi->tasaRespuesta );

		i++;
	}
}



mate_instanceACambiar* copyPaste(mate_instanceACambiar*  carpincho){
	mate_instanceACambiar* carpin = malloc(sizeof(mate_instanceACambiar));
	carpin->id = carpincho->id;
	carpin->estimacionProximaRafaga = carpincho->estimacionProximaRafaga;
	carpin->tasaRespuesta = carpincho->tasaRespuesta;
	carpin->tiempoInicialEnReady = carpincho->tiempoInicialEnReady;
	carpin->tiempoInicialEnExec = carpincho->tiempoInicialEnExec;
	carpin->tiempoDeEjecucionRafagaAnterior = carpincho->tiempoDeEjecucionRafagaAnterior;
	carpin->conexionMemoria = carpincho->conexionMemoria;

	carpin->sizeConfig.largoIpKernel = carpincho->sizeConfig.largoIpKernel;
	carpin->sizeConfig.largoPuertoKernel = carpincho->sizeConfig.largoPuertoKernel;
	carpin->sizeConfig.largoIpMemoria = carpincho->sizeConfig.largoIpMemoria;
	carpin->sizeConfig.largoPuertoMemoria = carpincho->sizeConfig.largoPuertoMemoria;
	carpin->sizeConfig.largomoduloAConectar = carpincho->sizeConfig.largomoduloAConectar;

	carpin->info.ipKernel = malloc(carpin->sizeConfig.largoIpKernel);
	memcpy(carpin->info.ipKernel, carpincho->info.ipKernel, carpin->sizeConfig.largoIpKernel);
	carpin->info.puertoKernel = malloc(carpin->sizeConfig.largoPuertoKernel);
	memcpy(carpin->info.puertoKernel, carpincho->info.puertoKernel, carpin->sizeConfig.largoPuertoKernel);
	carpin->info.ipMemoria = malloc(carpin->sizeConfig.largoIpMemoria);
	memcpy(carpin->info.ipMemoria, carpincho->info.ipMemoria, carpin->sizeConfig.largoIpMemoria);
	carpin->info.puertoMemoria = malloc(carpin->sizeConfig.largoPuertoMemoria);
	memcpy(carpin->info.puertoMemoria, carpincho->info.puertoMemoria, carpin->sizeConfig.largoPuertoMemoria);
	carpin->info.moduloAConectar = malloc(carpin->sizeConfig.largomoduloAConectar);
	memcpy(carpin->info.moduloAConectar, carpincho->info.moduloAConectar, carpin->sizeConfig.largomoduloAConectar);

	carpin->info = limpiarConfig(carpin->info, carpin->sizeConfig);

	return carpin;
}


int validarEstadoSuspension() {
	sem_wait(modificandoEstados);

	bool f = !queue_is_empty(new);
	bool i = !list_is_empty(blocked);
	bool k = list_is_empty(ready);

	sem_post(modificandoEstados);

	return f && i && k;
}

void transicionBlockedSuspendBlocked() {

	while (1) {


		if (validarEstadoSuspension()) {

			sem_post(carpinchos_listos_para_ejecutar); //semaforo A de new a Ready

			sem_wait(modificandoEstados);
			int posicionProceso = list_size(blocked) - 1;
			sem_post(modificandoEstados);

			sem_wait(modificandoEstados);
			int* idCarpin = list_get(blocked, posicionProceso);
			sem_post(modificandoEstados);

			int* idCopy = malloc(sizeof(int));
			memcpy(idCopy, idCarpin, sizeof(int));

			sem_wait(modificandoEstados);
			list_add(suspendBlocked, idCopy);
			list_remove_and_destroy_element(blocked, posicionProceso, free);
			sem_post(modificandoEstados);

			log_info(logKernel,"El Carpincho id:%d entra en SUSPEND BLOCKED ",*idCopy);

			mate_instanceACambiar* carpincho = obtenerCarpincho(*idCopy);
			int bytesCarpinchos = sizeDeCarpincho(carpincho);
			void* carpiSerializado = serializarCarpincho(carpincho);
			tipoDePeticion p = PROCESOSUSPENDIDO;
			void* bufferAMandar = malloc(sizeof(tipoDePeticion)+bytesCarpinchos);
			memcpy(bufferAMandar, &p, sizeof(tipoDePeticion));
			memcpy(bufferAMandar + sizeof(tipoDePeticion), carpiSerializado, bytesCarpinchos);

			int conexionMemoria = crearConexion(info.ipMemoria, info.puertoMemoria);
			int bytes = sizeof(tipoDePeticion) + bytesCarpinchos;
			send(conexionMemoria, &bytes, sizeof(int), 0);
			send(conexionMemoria, bufferAMandar, bytes, 0);

			free(bufferAMandar);
			free(carpiSerializado);
		}else{
			sem_wait(semBlokedSuspend);
		}

	}

}

void transicionExecBlocked(mate_instanceACambiar* carpincho) {



	bool _encontrarCarpinchoEnLista(int* id) {
		return carpincho->id == *id;
	}

//	mate_instanceACambiar* carpinchoABloquear = list_find(exec, (void*) _encontrarCarpinchoEnLista);
	sem_wait(modificandoEstados);
	int* idCarpin = list_find(exec, (void*) _encontrarCarpinchoEnLista);
	sem_post(modificandoEstados);

	if(idCarpin !=NULL){

		int* idCopia = malloc(sizeof(int));
		memcpy(idCopia, idCarpin, sizeof(int));

		sem_wait(modificandoEstados);
		int sizeExec = list_size(exec);
		bool readyTieneAlgo = !list_is_empty(ready);
		list_add(blocked, idCopia);
		list_remove_and_destroy_by_condition(exec, (void*) _encontrarCarpinchoEnLista, free);
//		sem_post(modificandoEstados);

		log_info(logKernel,"El Carpincho id:%d esta en estado BLOCKED ",carpincho->id);

//		sem_wait(modificandoEstados);
		if(!queue_is_empty(new) && list_size(blocked) == 1 && list_is_empty(ready) ){
			sem_post(semBlokedSuspend);
		}
//		sem_post(modificandoEstados);

		calcularRafagaEjecucion(*idCopia);

		if(sizeExec == info.gradoMultiprocesamiento && readyTieneAlgo){
			sem_post(carpinchos_en_ejecucion);
		}

		sem_post(modificandoEstados);

	}


}

void transicionBlockedReady(mate_instanceACambiar* carpincho) {

	bool _encontrarCarpinchoEnLista(int* id) {
		return carpincho->id == *id;
	}

	sem_wait(modificandoEstados);
	int* carpinchoADesbloquear = list_find(blocked, (void*) _encontrarCarpinchoEnLista);
	sem_post(modificandoEstados);

	if(carpinchoADesbloquear!=NULL){

		int* idCopy = malloc(sizeof(int));
		memcpy(idCopy, carpinchoADesbloquear, sizeof(int));

		sem_wait(modificandoEstados);
		list_add(ready, idCopy);
		list_remove_and_destroy_by_condition(blocked, (void*) _encontrarCarpinchoEnLista, free);
		sem_post(modificandoEstados);

		log_info(logKernel,"El carpincho:%d sale de BLOCKED y vuelve a READY",*idCopy);
		tiempoInicialEnReady(*idCopy);
		mate_instanceACambiar* carpincho = obtenerCarpincho(*idCopy);
		estimarRafaga(carpincho);
	}

	sem_wait(modificandoEstados);
	if (list_size(ready) == 1) {
		sem_post(semReadyExec);
		//printf("HOLA HICE UN 'semReadyExec' \n");
	}
	sem_post(modificandoEstados);


}

void transicionSuspendBlockedSuspendReady(mate_instanceACambiar* carpincho) {

	bool _encontrarCarpinchoEnLista(int* id) {
		return carpincho->id == *id;
	}

	sem_wait(modificandoEstados);
	int* idCarpincho = list_find(suspendBlocked, (void*) _encontrarCarpinchoEnLista);
	sem_post(modificandoEstados);


	if(idCarpincho != NULL){
		int* idCopy = malloc(sizeof(int));
		memcpy(idCopy, idCarpincho, sizeof(int));

		sem_wait(modificandoEstados);
		queue_push(suspendReady, idCopy);
		list_remove_and_destroy_by_condition(suspendBlocked, (void*) _encontrarCarpinchoEnLista, free);
		sem_post(modificandoEstados);

		log_info(logKernel,"El Carpincho id:%d en estado SUSPEND READY",*idCopy);
	}

	sem_wait(modificandoEstados);
	if(queue_size(suspendReady) == 1 && queue_size(new) == 0){
		sem_post(semNewReady);
	}
	sem_post(modificandoEstados);

}

void finalizarCarpincho(mate_instanceACambiar* carpincho) {

//	printf("Voy a empezar a finalizar... %d\n", carpincho->id);


//	bool _encontrarCarpinchoEnLista(mate_instanceACambiar* elemento) {
//		return carpincho->id == elemento->id;
//	}
	bool _encontrarCarpinchoEnLista(int* id) {
		return carpincho->id == *id;
	}

	sem_wait(modificandoEstados);
	int sizeExec = list_size(exec);
	list_remove_and_destroy_by_condition(exec, (void*) _encontrarCarpinchoEnLista, free);

	sem_post(modificandoEstados);

	if(sizeExec == info.gradoMultiprocesamiento){
		sem_post(carpinchos_en_ejecucion);
	}

	sem_post(carpinchos_listos_para_ejecutar);

	hacerPostDeSemsCarpincho(carpincho);

	bool encontraCarpi(mate_instanceACambiar* carp){
		return carp->id == carpincho->id;
	}
	mate_instanceACambiar* carpin2 = list_find(listaDeCarpinchos, (void*) encontraCarpi);

	if(carpin2 != NULL){
		list_remove_by_condition(listaDeCarpinchos, (void*) encontraCarpi);
		liberarCarpincho(carpin2);
	}

	log_info(logKernel,"Carpincho id:%d finalizando... ",carpincho->id);

}


void printCarpinsEnDeadlock(t_list* carpinchosEnDeadlock){
	printf("CARPINCHOS EN DEADLOCK: ");
	int tamLista = list_size(carpinchosEnDeadlock);
	for(int i=0; i<tamLista; i++){
		int* carpin = list_get(carpinchosEnDeadlock, i);
		printf("%d, ", *carpin);
	}
	printf("\n");
}


void asesinarCarpincho(t_list* carpinchosEnDeadlock) {


	int* maximoCarpin(int* a, int* b) {
		int c = *a;
		int d = *b;
		if (c > d) {
			return a;
		}else{
			return b;
		}
	}

	//printCarpinsEnDeadlock(carpinchosEnDeadlock);
	int* idMaximo = list_get_maximum(carpinchosEnDeadlock,(void*) maximoCarpin);
	printf("Voy a matar al carpincho ID: %d \n", *idMaximo);//SI COMENTO LA LINEA DEL PRINT DE CARPINCHOS EN DEADLOCK TIRA MAL

	bool encontrarCarpin(mate_instanceACambiar* carpin) {
		return carpin->id == *idMaximo;
	}

	sem_wait(semListaCarpinchos);
	mate_instanceACambiar* carpincho = list_find(listaDeCarpinchos, (void*) encontrarCarpin);
	sem_post(semListaCarpinchos);

	bool encontrarCarpinchoEnSemaforos(listaDeadlock* elemento){
		return elemento->carpincho == carpincho->id;
	}

	listaDeadlock* elemento = list_find(solicitudesActuales, (void*)encontrarCarpinchoEnSemaforos);

	if(elemento != NULL){
		//printf("SOCKET: %d \n", elemento->socket);
		mate_instanceACambiar* carpinchoCopia = copyPaste(carpincho);

		carpinchoCopia->id = -4;

		void* bufferSend = serializarCarpincho(carpinchoCopia);
		int cantBytes = sizeDeCarpincho(carpinchoCopia);
		send(elemento->socket, &cantBytes, sizeof(int), 0);
		send(elemento->socket, bufferSend, cantBytes, 0);

		free(bufferSend);
		liberarCarpincho(carpinchoCopia);
	}

	bool _encontrarCarpinchoEnLista(int* elemento) {
		return *idMaximo == *elemento;
	}

	sem_wait(modificandoEstados);
	list_remove_and_destroy_by_condition(blocked, (void*) _encontrarCarpinchoEnLista, free);
	sem_post(modificandoEstados);


	hacerPostDeSemsCarpincho(carpincho);

	sem_post(carpinchos_listos_para_ejecutar);

	list_remove_by_condition(listaDeCarpinchos, (void*) encontrarCarpin);
	liberarCarpincho(carpincho);
}

void hacerPostDeSemsCarpincho(mate_instanceACambiar* carpincho) {

	bool encontrarCarpi(listaDeadlock* l) {
		return l->carpincho == carpincho->id;
	}

	listaDeadlock* carpinConSem = list_find(solicitudesActuales, (void*) encontrarCarpi);

	if (carpinConSem != NULL) {
		semPostEnDeadlock(carpinConSem->semaforo, carpincho);
		list_remove_and_destroy_by_condition(solicitudesActuales, (void*) encontrarCarpi, free);
	}

	bool cumpleCon(int* a) {
		return *a == carpincho->id;
	}
	bool encontrarCarpi2(matrizDeadlock* l) {
		return list_any_satisfy(l->carpinchos, (void*) cumpleCon);
	}

	t_list* listaIncompleta = list_filter(recursosAsignados, (void*) encontrarCarpi2);


	if (listaIncompleta != NULL) {
		int tamLista = list_size(listaIncompleta);
		for (int i = 0; i < tamLista; i++) {
			matrizDeadlock* semaforoConLista = list_get(listaIncompleta, i);
			semPostEnDeadlock(semaforoConLista->semaforo, carpincho);

			bool encontrarCarpi3(int* l) {
				return *l == carpincho->id;
			}

			list_remove_by_condition(semaforoConLista->carpinchos,(void*)encontrarCarpi3);
		}
	}

	list_destroy(listaIncompleta);

}

void liberarCarpincho(mate_instanceACambiar* carpincho) {
	liberarConfig(carpincho->info);
	free(carpincho);
}

void inicializarSemaforos() {

	carpinchos_en_ejecucion = malloc(sizeof(sem_t));
	carpinchos_listos_para_ejecutar = malloc(sizeof(sem_t));
	semDeadlock = malloc(sizeof(sem_t));
	semReadyExec = malloc(sizeof(sem_t));
	semNewReady = malloc(sizeof(sem_t));
	semBlokedSuspend = malloc(sizeof(sem_t));
	modificandoEstados = malloc(sizeof(sem_t));
	semListaCarpinchos = malloc(sizeof(sem_t));
	semaforoDeSemaforo = malloc(sizeof(sem_t));
	semRecursosTotales = malloc(sizeof(sem_t));
	semaforoRecursoIO = malloc(sizeof(sem_t));
	semaforoDeSemaforosExec = malloc(sizeof(sem_t));
	semExecBlocked = malloc(sizeof(sem_t));
	semAsignarId = malloc(sizeof(sem_t));

	sem_init(carpinchos_en_ejecucion, 0, 0);
	sem_init(carpinchos_listos_para_ejecutar, 0, info.gradoMultiprogramacion);
	sem_init(semDeadlock, 0, 1);
	sem_init(semReadyExec, 0, 0);
	sem_init(semNewReady,0,0);
	sem_init(semBlokedSuspend, 0, 0);
	sem_init(modificandoEstados, 0,1);
	sem_init(semListaCarpinchos,0,1);
	sem_init(semaforoDeSemaforo,0,1);
	sem_init(semRecursosTotales, 0, 1);
	sem_init(semaforoRecursoIO, 0, 1);
	sem_init(semaforoDeSemaforosExec, 0, 1);
	sem_init(semExecBlocked, 0, 1);
	sem_init(semAsignarId, 0, 1);


}


void generarRecursosIO() {

	int tamLista = list_size(info.dispositivosIO);
	for (int i = 0; i<tamLista; i++) {
		mate_io_resource recu = list_get(info.dispositivosIO, i);
		int* duracion = list_get(info.duracionesIO, i);

		recursoIO* recurso = nuevoRecursoIO(recu, *duracion);

		sem_wait(semaforoRecursoIO);
		list_add(recursosIO, recurso);
		sem_post(semaforoRecursoIO);

	}

}

recursoIO* nuevoRecursoIO(mate_io_resource nombre, int duracion) {
	recursoIO* recursoIO = malloc(sizeof(recursoIO));

	int tam = string_length(nombre)+1;
	recursoIO->nombre = malloc(tam);
	memcpy(recursoIO->nombre,nombre, tam);
	recursoIO->semaforo = malloc(sizeof(sem_t));
	sem_init(recursoIO->semaforo,0,1);
	recursoIO->duracion = duracion;

	return recursoIO;
}

void agregarARecursosTotales(infoSemaforo* semaforo) {

	bool _econtrarSemaforoEnLista(infoSemaforo* semaforoEnLista) {
		return (string_equals_ignore_case(semaforoEnLista->nombre, semaforo->nombre));
	}

	sem_wait(semDeadlock);
	infoSemaforo* listSem = list_find(recursosTotales, (void*) _econtrarSemaforoEnLista);
	if (listSem == NULL) {
		list_add(recursosTotales, semaforo);
	}

	sem_post(semDeadlock);
}

void eliminarDeRecursosTotales(mate_sem_name semaforo) {

	bool encontraEnRecursos(matrizDeadlock* mat){
		return string_equals_ignore_case(mat->semaforo, semaforo);
	}

	bool _encontrarSemaforoEnLista(infoSemaforo* semaforoEnLista) {
		return (string_equals_ignore_case(semaforoEnLista->nombre, semaforo));
	}
	sem_wait(semDeadlock);
	infoSemaforo* sem = list_find(recursosTotales, (void*) _encontrarSemaforoEnLista);
	free(sem->nombre);
	free(sem->semaforo);
	list_remove_and_destroy_by_condition(recursosTotales, (void*) _encontrarSemaforoEnLista, (void*) free);
	list_remove_and_destroy_by_condition(recursosAsignados, (void*) encontraEnRecursos, free);
	sem_post(semDeadlock);
}

bool procesoEstaBloqueado(int idCarpincho) {

	bool _encontrarCarpinchoEnLista(mate_instanceACambiar* elemento) {
		return idCarpincho == elemento->id;
	}

	return (list_find(blocked, (void*) _encontrarCarpinchoEnLista) != NULL);
}

void algoritmoDeadlock() {
	while (1) {
		usleep(info.tiempoDeadlock * 1000);
		sem_wait(semDeadlock);
		printf("VOY A EMPEZAR DEADLOCK CON: \n");
//		mostrarSolicitudesActualesyRec();
		hayDeadlock();
		sem_post(semDeadlock);
	}
}

void hayDeadlock() {

	int flagDeadlock = 0;
	int flag;
	int chequeo;
	int hola = 0;

	t_list* carpinchosEnDeadlock;
	for (int i = 0; i < list_size(solicitudesActuales) && flagDeadlock != 1; i++) {
		listaDeadlock* listaSolicitudesActuales = list_get(solicitudesActuales, i);
		printf("Entre al for \n");
		flag = 0;
		chequeo = -1;

		carpinchosEnDeadlock = list_create();
		int lengthSem = string_length(listaSolicitudesActuales->semaforo);
		mate_sem_name sema = string_substring(listaSolicitudesActuales->semaforo, 0, lengthSem);

		int carpinInicial = listaSolicitudesActuales->carpincho;
		int* aGuarda = malloc(sizeof(int));
		memcpy(aGuarda, &carpinInicial, sizeof(int));
		list_add(carpinchosEnDeadlock, aGuarda);

		resetContadores();
		while (chequeo != carpinInicial && flag != 1) {

			if (hola == 0) {
				chequeo = carpinInicial;
				hola = 1;
			}

			int idCarpincho2 = estaEnAsignados(sema, chequeo);

			if (idCarpincho2 > 0) {

				bool estaElCarpi(listaDeadlock* mm) {
					return mm->carpincho == idCarpincho2;
				}

				listaDeadlock* mm = list_find(solicitudesActuales,(void*) estaElCarpi);

				if(mm!=NULL){
					int lengthSemm = string_length(mm->semaforo);
					free(sema);
					sema = string_substring(mm->semaforo, 0, lengthSemm);
				}else{
					sema=NULL;
				}


				chequeo = idCarpincho2;

				if (chequeo != carpinInicial) {
					int* aGuarda2 = malloc(sizeof(int));
					memcpy(aGuarda2, &chequeo, sizeof(int));
					list_add(carpinchosEnDeadlock, aGuarda2);
					printCarpinsEnDeadlock(carpinchosEnDeadlock);

				}

				if (sema == NULL) {
					flag = 1;
				}

			} else {
				//Pasa al siguiente
				flag = 1;
				chequeo = -2;
			}

		}

		if (chequeo == carpinInicial) {
			flagDeadlock = 1;
		} else {
			list_destroy_and_destroy_elements(carpinchosEnDeadlock, free);
		}

		hola = 0;
		free(sema);
	}

	if (flagDeadlock == 1) {
		//HAY DEADLOCK
//		mostrarSolicitudesActualesyRec();
		printf("HAAAYYY DEEEADDDLOOCCKKKKKKKK!!!!!!!!!!!!!!!!!!! \n");

		asesinarCarpincho(carpinchosEnDeadlock);
		list_destroy_and_destroy_elements(carpinchosEnDeadlock, free);

//		mostrarSolicitudesActualesyRec();

		hayDeadlock();
	}

}

int estaEnAsignados(mate_sem_name semaforo, int chequeo) {
	printf("Esta en asignados? \n");
	int a = -1;
	int aux=0;
	bool estaElRecurso(matrizDeadlock* matriz) {
		return string_equals_ignore_case(matriz->semaforo, semaforo);
	}

	matrizDeadlock* listaRecursosAsignados = list_find(recursosAsignados, (void*) estaElRecurso);

	bool encontrarContador(contadorSemaforo* c) {
		return string_equals_ignore_case(c->semaforo, semaforo);
	}

	contadorSemaforo* contadorSem = list_find(contadoresSemaforos, (void*) encontrarContador);
	if(contadorSem!=NULL){
		aux = contadorSem->contador;
	}else{
		return -2;
	}


	if (listaRecursosAsignados != NULL) {

		int tamLista = list_size(listaRecursosAsignados->carpinchos);
		if (aux < tamLista) {

			int* carpi = list_get(listaRecursosAsignados->carpinchos, aux);

			if (*carpi == chequeo) {
				aux += 1;
				carpi = list_get(listaRecursosAsignados->carpinchos, aux);
			}

			aux += 1;
			a = *carpi;

		}else {
			return -2;
		}

	}

	contadorSem->contador = aux;

	list_remove_by_condition(contadoresSemaforos, (void*) encontrarContador);
	list_add(contadoresSemaforos, contadorSem);

	printf("Se encontro al carpin %d, usando el semaforo %s \n", a, semaforo);
	return a;
}

void resetContadores() {
	printf("reseteando contadores \n");

//	void reset(contadorSemaforo *c) {
//		c->contador = 0;
//	}
//
//	t_list* listaAux = list_map(contadoresSemaforos, (void*) reset);
//	contadoresSemaforos = listaAux;

	int tamLista = list_size(contadoresSemaforos);
	for(int i=0; i<tamLista; i++){
		contadorSemaforo* contador = list_get(contadoresSemaforos, i);
		contador->contador =0;
	}
}


void iniciarPrograma() {


	pthread_t planificadorLargoPlazo;
	pthread_create(&planificadorLargoPlazo, NULL, (void*) transicionNewReady,NULL);
	pthread_detach(planificadorLargoPlazo);

	pthread_t hiloReadyExec;
	pthread_create(&hiloReadyExec, NULL, (void*) transicionReadyExec, NULL);
	pthread_detach(hiloReadyExec);

	pthread_t ejecucionDeadlock;
	pthread_create(&ejecucionDeadlock, NULL, (void*) algoritmoDeadlock, NULL);
	pthread_detach(ejecucionDeadlock);

	pthread_t hiloBlokeante;
	pthread_create(&hiloBlokeante, NULL, (void*) transicionBlockedSuspendBlocked, NULL);
	pthread_detach(hiloBlokeante);

//	pthread_t hiloMostrador;
//	pthread_create(&hiloMostrador, NULL, (void*) mostrarEstados, NULL);
//	pthread_detach(hiloMostrador);

	recursosTotales = list_create();
	recursosIO = list_create();
	contadoresSemaforos = list_create();
	solicitudesActuales = list_create();
	recursosAsignados = list_create();

	generarRecursosIO();
}


void nuevoSemaforo(mate_sem_name semName, int valorSem) {

	infoSemaforo* sem = malloc(sizeof(infoSemaforo));
	sem->semaforo = malloc(sizeof(sem_t));
	sem->nombre = malloc(string_length(semName) + 1);

	sem_init(sem->semaforo,0,valorSem);

	memcpy(sem->nombre,semName,string_length(semName) + 1);
	sem->valorDelSem = valorSem;

	agregarARecursosTotales(sem);

	contadorSemaforo* contador = malloc(sizeof(contadorSemaforo));
	contador->semaforo = malloc(string_length(semName) + 1);
	memcpy(contador->semaforo,semName,string_length(semName) + 1 );
	contador->contador = 0;
	list_add(contadoresSemaforos,contador);


}

void semWait(mate_sem_name semaforoLib, mate_instanceACambiar* carpincho, int socket){
	infoSemaforo* semaforo = encontrarSemaforo(semaforoLib);

	if(semaforo == NULL){
		printf("SEMAFORO NO ENCONTRADO");
		return;
	}

	if (semaforo->valorDelSem < 1) {

		transicionExecBlocked(carpincho);

		agregarASolicitudesActuales(semaforoLib,carpincho, socket);//agregar al carpincho a solicitudes actuales
		sem_wait(semDeadlock);
		printf("ME VOY A BLOQUEAR (ID:%d, SEM:%s) ASI QUEDO SOLICITUDES ACTUALES: \n", carpincho->id, semaforoLib);
//		mostrarSolicitudesActualesyRec();
		sem_post(semDeadlock);

		semaforo->valorDelSem--;
		sem_wait(semaforo->semaforo);

		sacarDeSolicitudesActuales(semaforoLib,carpincho);

		transicionSuspendBlockedSuspendReady(carpincho);
		transicionBlockedReady(carpincho);

	}else{
		semaforo->valorDelSem--;
		sem_wait(semaforo->semaforo);
	}

	sem_wait(semDeadlock);
	agregarARecursosAsignados(semaforoLib,carpincho);
	sem_post(semDeadlock);

	sem_wait(semDeadlock);
//	printf("-----------------------------DESPUES DEL WAIT-------------------------------- \n");
//	mostrarSolicitudesActualesyRec();
	sem_post(semDeadlock);
}

void mostrarSolicitudesActualesyRec(){
	int tam = list_size(solicitudesActuales);
	int j = 0;

	for (int i = 0; i < tam; i++) {

		listaDeadlock* elementoLista = list_get(solicitudesActuales,i);
		printf("El semaforo %s tiene una solicitud de carpincho Id: %d \n",elementoLista->semaforo,elementoLista->carpincho);


		matrizDeadlock* elementoLista2 = list_get(recursosAsignados,i);
		printf("El semaforo %s tiene asignados los sig carpinchos: \n", elementoLista2->semaforo);

		for (j = 0; j < list_size(elementoLista2->carpinchos); j++) {
			int* id = list_get(elementoLista2->carpinchos,j);
			printf("ID:%d \n", *id);
		}


	}

}
void semPost(mate_sem_name semaforoLib, mate_instanceACambiar* carpincho) {

	//encontrar al semaforoLib en la lista de RecursosTotales
	infoSemaforo* semAUsar = encontrarSemaforo(semaforoLib);

	//Hacer sem_post de ese sem
	sem_post(semAUsar->semaforo);

	//Sumarle uno a la veriable de valoresActuales
	semAUsar->valorDelSem++;

	sem_wait(semDeadlock);
	sacarDeRecursosAsignados(semaforoLib,carpincho);
	sem_post(semDeadlock);

}


void semPostEnDeadlock(mate_sem_name semaforoLib, mate_instanceACambiar* carpincho) {

	//encontrar al semaforoLib en la lista de RecursosTotales
	infoSemaforo* semAUsar = encontrarSemaforo(semaforoLib);

	//Hacer sem_post de ese sem
	sem_post(semAUsar->semaforo);

	//Sumarle uno a la veriable de valoresActuales
	semAUsar->valorDelSem++;

	sacarDeRecursosAsignados(semaforoLib,carpincho);

}


void agregarASolicitudesActuales(mate_sem_name semaforoSolicitado ,mate_instanceACambiar* carpinchoSolicitante, int socket){

	sem_wait(semDeadlock);
	listaDeadlock* solicitud = malloc(sizeof(listaDeadlock));
	solicitud->semaforo = semaforoSolicitado;
	solicitud->carpincho = carpinchoSolicitante->id;
	solicitud->socket = socket;

	list_add(solicitudesActuales, solicitud);

	sem_post(semDeadlock);



}

void sacarDeSolicitudesActuales(mate_sem_name semaforoSolicitado, mate_instanceACambiar* carpinchoSolicitante){

	bool encontrarProcesoEnSolicitudes(listaDeadlock* solicitud){
		return string_equals_ignore_case(solicitud->semaforo,semaforoSolicitado);
	}
	sem_wait(semDeadlock);
	listaDeadlock* procesoEnLista = list_find(solicitudesActuales, (void*) encontrarProcesoEnSolicitudes);

	if(procesoEnLista != NULL){
		list_remove_and_destroy_by_condition(solicitudesActuales, (void*) encontrarProcesoEnSolicitudes, free);
	}

	sem_post(semDeadlock);

}

void agregarARecursosAsignados(mate_sem_name semaforo, mate_instanceACambiar* carpincho){

	bool encontrarSemaforo(matrizDeadlock* recursoLista){
		return string_equals_ignore_case(recursoLista->semaforo,semaforo);
	}

	bool encontrarCarpinchoEnLista(int* id) {
			return carpincho->id == *id;
	}

	matrizDeadlock* recursoAEncontrar = list_find(recursosAsignados,(void*)encontrarSemaforo);

	if(recursoAEncontrar == NULL){

		matrizDeadlock* recurso = malloc(sizeof(matrizDeadlock));
		recurso->carpinchos = list_create();
		int* idC = malloc(sizeof(int));
		memcpy(idC, &carpincho->id, sizeof(int));
		list_add(recurso->carpinchos, idC);
		recurso->semaforo = malloc(string_length(semaforo) + 1);
		memcpy(recurso->semaforo, semaforo, string_length(semaforo)+1);


		list_add(recursosAsignados, recurso);

	}else{

		int* carpinchoLista = list_find(recursoAEncontrar->carpinchos, (void*) encontrarCarpinchoEnLista);

		if(carpinchoLista != NULL){
			int* idC = malloc(sizeof(int));
			memcpy(idC, &carpincho->id, sizeof(int));
			list_add(recursoAEncontrar->carpinchos,idC);
		}

	}



}

void sacarDeRecursosAsignados(mate_sem_name semaforo, mate_instanceACambiar* carpincho){

	bool encontrarSemaforo(matrizDeadlock* recursoLista){
		return string_equals_ignore_case(recursoLista->semaforo,semaforo);
	}

	bool encontrarCarpinchoEnLista(mate_instanceACambiar* elemento) {
			return carpincho->id == elemento->id;
	}

	matrizDeadlock* recursoAEncontrar = list_find(recursosAsignados,(void*)encontrarSemaforo);

	if(recursoAEncontrar == NULL){
		printf("ERROR: El semaforo no se encuentra en la lista de recursos asignados");
		return;
	}

	mate_instanceACambiar* carpinchoLista = list_find(recursoAEncontrar->carpinchos, (void*)encontrarCarpinchoEnLista);

	if(carpinchoLista == NULL){
		//printf("ERROR: El carpincho no tenia asociado este semaforo");
		return;
	}

	list_remove_by_condition(recursoAEncontrar->carpinchos,(void*) encontrarCarpinchoEnLista );



}



void carpinchoPideIO(mate_io_resource recurso, mate_instanceACambiar* carpincho) {

	recursoIO* recursoIO = encontrarRecursoIO(recurso);

	//sem_wait(semExecBlocked);
	transicionExecBlocked(carpincho);
	//sleep(1.5); //Con este sleep HRRN ejecuta bien
	printf("Voy a hacer IO ID: %d \n", carpincho->id);
	//sem_post(semExecBlocked);

	sem_wait(recursoIO->semaforo);
	usleep(recursoIO->duracion * 1000);
	sem_post(recursoIO->semaforo);

	printf("Termine de hacer IO ID: %d\n", carpincho->id);

	transicionSuspendBlockedSuspendReady(carpincho);
	transicionBlockedReady(carpincho);

}


infoSemaforo* encontrarSemaforo(mate_sem_name semaforoLib) {
	bool _encontrarSemaforoEnLista(infoSemaforo* elementoLista) {
		return string_equals_ignore_case(elementoLista->nombre, semaforoLib);
	}

	sem_wait(semRecursosTotales);
	infoSemaforo* semAUsar = list_find(recursosTotales, (void*) _encontrarSemaforoEnLista);
	sem_post(semRecursosTotales);
	return semAUsar;
}

recursoIO* encontrarRecursoIO(mate_io_resource recurso) {

	bool _encontrarRecursoIOEnLista(recursoIO* recursoIOLista) {
		return string_equals_ignore_case(recurso, recursoIOLista->nombre);
	}
//	mostrarTlistint(info.duracionesIO);
//	mostrarTlist(info.dispositivosIO);
	sem_wait(semaforoRecursoIO);
	recursoIO* recu = list_find(recursosIO, (void*) _encontrarRecursoIOEnLista);
	sem_post(semaforoRecursoIO);

	return recu;
}

int generarId() {
	sem_wait(semAsignarId);
	static int idProceso = 0;
	idProceso++;
	sem_post(semAsignarId);
	return idProceso;
}

int sizeDeCarpincho(mate_instanceACambiar* carpincho) {
	int a = 2 * sizeof(int) + sizeof(float) * 2 + sizeof(time_t) * 3;
	int c = sizeConfig(carpincho->info);
	int d = sizeof(tamConfig);

	return a+c+d;
}

t_list* arrayATlist(char** array) {
	t_list* listaDeBloques = list_create();
	
	int largo;
	for (largo = 0; array[largo] != NULL; largo++) {
		char* bloque = array[largo];
		int tamBloque = string_length(bloque);

		char* bloque2 = string_substring(bloque, 0, tamBloque);
		list_add(listaDeBloques, bloque2);
	}
	return listaDeBloques;
}


t_list* arrayATlistDeInts(char** array){
	t_list* listaDeBloques = list_create();

	int largo;	
	for (largo = 0; array[largo] != NULL; largo++) {
		char* bloque = array[largo];

		int* bloqueInt = malloc(sizeof(int));		 //Reservas memoria en el heap
		int tempVal = atoi(bloque);			//Obtener el valor en int del string que tenias en el config
		memcpy(bloqueInt, &tempVal, sizeof(int));	//Copias el valor a la variable que reservaste antes
		list_add(listaDeBloques, bloqueInt);		//Guardas la variable de heap en la lista en lugar la variable de stack
	}
	return listaDeBloques;
}


void frezee(char** array) {

	for (int i = 0; array[i] != NULL; i++) {
		free(array[i]);
	}
	free(array);
}



void mostrarEstados(){
	while(1){
		sleep(2);
		sem_wait(modificandoEstados);
	
		printf("--------------------------------------------------------------------\n");
		printf("ESTADOS:\n");
//		t_list* listaNew = deQueueALista(new);
//		t_list* listaSuspendReady = deQueueALista(suspendReady);
//		printearEstado("NEW", listaNew);
		printearEstado("READY", ready);
		printearEstado("EXEC", exec);
		printearEstado("BLOCKED", blocked);
		printearEstado("SUSPENDBLOCKED", suspendBlocked);
//		printearEstado("SUSPENDREADY", listaSuspendReady);
		printf("--------------------------------------------------------------------\n\n");
		
//		list_destroy(listaNew);
//		list_destroy(listaSuspendReady);
		sem_post(modificandoEstados);
	}
}

void printearEstado(char* estado, t_list* listaEstado){
	int tamLista = list_size(listaEstado);
	
	printf("->%s: ", estado);
	for(int i=0; i<tamLista; i++){
		int* carpincho = list_get(listaEstado, i);
		printf("%d, ", *carpincho);
	}
	printf("\n");
}

void printearEstado2(char* estado, t_list* listaEstado){
	int tamLista = list_size(listaEstado);

	printf("->%s: ", estado);
	for(int i=0; i<tamLista; i++){
		int* carpincho = list_get(listaEstado, i);
		printf("%d, ", *carpincho);
	}
	printf("\n");
}

t_list* deQueueALista(t_queue* colaEstado){
	t_list* listaEstado = list_create();
	int tamCola = queue_size(colaEstado);
	
	for(int i=0; i<tamCola; i++){
		int* carpincho = queue_pop(colaEstado);
		list_add(listaEstado, carpincho);
	}
	
	int tamLista = list_size(listaEstado);

	for(int i=tamLista-1; i>=0; i--){
		int* carpincho = list_get(listaEstado, i);
		queue_push(colaEstado, carpincho);
	}
	
	return listaEstado;
}


mate_instanceACambiar* obtenerCarpincho(int idCarpi){

	bool encontrarCarpi(mate_instanceACambiar* carpinch){
		return carpinch->id == idCarpi;
	}

	sem_wait(semListaCarpinchos);
	mate_instanceACambiar* carpincho = list_find(listaDeCarpinchos, (void*) encontrarCarpi);
	sem_post(semListaCarpinchos);

	return carpincho;
}

t_list* carpinchosEnReady(){
	t_list* lista = list_create();
	int tamReady = list_size(ready);

	for(int i=0; i<tamReady; i++){
		int* idCarp = list_get(ready, i);
		mate_instanceACambiar* carpincho = obtenerCarpincho(*idCarp);
		list_add(lista, carpincho);
	}

	return lista;
}


void actualizarCarpincho(mate_instanceACambiar* carpinchoViejo){

	mate_instanceACambiar* carpinchoNuevo = copyPaste(carpinchoViejo);

	bool encontrarCarpi(mate_instanceACambiar* carpinch){
		return carpinch->id == carpinchoViejo->id;
	}

	sem_wait(semListaCarpinchos);
	list_remove_by_condition(listaDeCarpinchos, (void*)encontrarCarpi);//Capaz se puede meter and_destroy
	list_add(listaDeCarpinchos, carpinchoNuevo);
	sem_post(semListaCarpinchos);

}


void estimarRafagaDeTodosEnReady(){
	int sizeReady = list_size(ready);

	for(int i=0; i<sizeReady; i++){

		int* idCarpincho = list_get(ready, i);
		mate_instanceACambiar* carpincho = obtenerCarpincho(*idCarpincho);

		double tiempoDeEjecucion = carpincho->tiempoDeEjecucionRafagaAnterior;
		double alfa = info.alfa; //0.3

		if (tiempoDeEjecucion == 0) {
			carpincho->estimacionProximaRafaga = info.estimacionInicial / 1000;
		} else {
			float resultado =  alfa * tiempoDeEjecucion + (1 - alfa) * carpincho->estimacionProximaRafaga;
			carpincho->estimacionProximaRafaga = resultado;
		}

		printf("Estimar Rafaga ID: %d, tiempoDeEjecucion: %f, ProxRafaga: %f\n",carpincho->id, tiempoDeEjecucion, carpincho->estimacionProximaRafaga);
		//actualizarCarpincho(carpincho);

	}

}

void calcularTasaRespuestaDeTodosEnReady() {//hrrn

	int sizeReady = list_size(ready);

	for(int i=0; i<sizeReady; i++){

		int* idCarpincho = list_get(ready, i);
		mate_instanceACambiar* carpincho = obtenerCarpincho(*idCarpincho);

		time_t tiempoInicialEnReady = carpincho->tiempoInicialEnReady;
		double estimacionRafaga = carpincho->estimacionProximaRafaga;

		double tiempoEnEspera = difftime(time(NULL),tiempoInicialEnReady);
		double resultado = (tiempoEnEspera/ estimacionRafaga  ) + 1;
		carpincho->tasaRespuesta = (float) resultado;
//		printf(" tasaCalculada: %.2f tiempoEnEspera: %f \n",carpincho->tasaRespuesta , tiempoEnEspera);

//		actualizarCarpincho(carpincho);
	}
}


void tiempoInicialEnReady(int idCarpi){

	mate_instanceACambiar* carpincho = obtenerCarpincho(idCarpi);
	carpincho->tiempoInicialEnReady = time(NULL);
//	printf("HOLA ESTOY ACTUALIZANDO TIEMPO INICIAL EN READY, %f \n", (double)carpincho->tiempoInicialEnReady);
//	actualizarCarpincho(carpincho);

}

void tiempoInicialEnExec(int idCarpi){

	mate_instanceACambiar* carpincho = obtenerCarpincho(idCarpi);
	carpincho->tiempoInicialEnExec = time(NULL);
//	actualizarCarpincho(carpincho);
}











