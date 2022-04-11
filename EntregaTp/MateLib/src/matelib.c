#include "matelib.h"



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
		perror("ERROR: ");
	        exit(-1);
	    }


	    freeaddrinfo(server_info);
	    return clientSocket;
}



//int crearConexion(char* ip, char* puerto) {
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
//		//	    	log_error(logLibreria, "ERROR EN GETADDRINFO");
//		exit(-1);
//	}
//
//	clientSocket = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);
//
//	if (connect(clientSocket, server_info->ai_addr, server_info->ai_addrlen)== -1) {
//		//	        log_error(logLibreria, "FALLÓ CONEXIÓN ENTRE LIBRERIA Y MODULO...");
//		exit(-1);
//	}
//
//	//	    log_info(logLibreria, "CONEXIÓN EXITOSA...");
//
//	freeaddrinfo(server_info);
//	return clientSocket;
//}

infoConfig obtenerInfoConfigFinal(t_config* config) {
	infoConfig info;

	info.ipKernel = config_get_string_value(config, "IP_KERNEL");
	info.puertoKernel = config_get_string_value(config, "PUERTO_KERNEL");
	info.ipMemoria = config_get_string_value(config, "IP_MEMORIA");
	info.puertoMemoria = config_get_string_value(config, "PUERTO_MEMORIA");
	info.moduloAConectar = config_get_string_value(config, "MODULO_A_CONECTAR");

	return info;
}

infoConfig obtenerInfoConfig() {
	infoConfig info;

	t_config* config = config_create("/home/utnso/tp-2021-2c-Los-Picateclas/PruebasCarpincho/libreria.config");

	info.ipKernel = config_get_string_value(config, "IP_KERNEL");
	info.puertoKernel = config_get_string_value(config, "PUERTO_KERNEL");
	info.ipMemoria = config_get_string_value(config, "IP_MEMORIA");
	info.puertoMemoria = config_get_string_value(config, "PUERTO_MEMORIA");
	info.moduloAConectar = config_get_string_value(config, "MODULO_A_CONECTAR");

	return info;
}

int sizeConfig(infoConfig info){

	int largoIpKernel = string_length(info.ipKernel);
	int largoPuertoKernel = string_length(info.puertoKernel);
	int largoIpMemoria = string_length(info.ipMemoria);
	int largopuertoMemoria = string_length(info.puertoMemoria);
	int largoModulo = string_length(info.moduloAConectar);

	return largoIpKernel+largoPuertoKernel+largoIpMemoria+largopuertoMemoria+largoModulo;
}

int sizeDeCarpincho(mate_instanceACambiar* carpincho) {
	int a = 2 * sizeof(int) + sizeof(float) * 2 + sizeof(time_t) * 3;
	int c = sizeConfig(carpincho->info);
	int d = sizeof(tamConfig);

	return a+c+d;
}

int sizeDeSemaforo() {

	int a = sizeof(int) + sizeof(sem_t) + sizeof(char*);
	return a;
}

int sizeDeIO() {
	int a = sizeof(char*) + sizeof(sem_t*) + sizeof(int);
	return a;
}


tamConfig valoresConfig(infoConfig info){
	tamConfig valores;
	valores.largoIpKernel = string_length(info.ipKernel);
	valores.largoPuertoKernel = string_length(info.puertoKernel);
	valores.largoIpMemoria = string_length(info.ipMemoria);
	valores.largoPuertoMemoria = string_length(info.puertoMemoria);
	valores.largomoduloAConectar = string_length(info.moduloAConectar);
	return valores;

}


void* serializarSizeConfig(infoConfig info){
	tamConfig asignarValores = valoresConfig(info);
	void* buffer = malloc(sizeof(tamConfig));
	int desplazamiento = 0;


	memcpy(buffer, &(asignarValores.largoIpKernel), sizeof(int));
	desplazamiento += sizeof(int);

	memcpy(buffer+desplazamiento, &(asignarValores.largoPuertoKernel), sizeof(int));
	desplazamiento += sizeof(int);

	memcpy(buffer+desplazamiento, &(asignarValores.largoIpMemoria), sizeof(int));
	desplazamiento += sizeof(int);

	memcpy(buffer+desplazamiento, &(asignarValores.largoPuertoMemoria), sizeof(int));
	desplazamiento += sizeof(int);

	memcpy(buffer+desplazamiento, &(asignarValores.largomoduloAConectar), sizeof(int));
	desplazamiento += sizeof(int);

	return buffer;
}

tamConfig desserializarSizeConfig(void* buffer, int desplazamiento){
	tamConfig asignarValores;

	memcpy(&(asignarValores.largoIpKernel), buffer+desplazamiento, sizeof(int));
	desplazamiento += sizeof(int);

	memcpy(&(asignarValores.largoPuertoKernel), buffer+desplazamiento, sizeof(int));
	desplazamiento += sizeof(int);

	memcpy(&(asignarValores.largoIpMemoria), buffer+desplazamiento, sizeof(int));
	desplazamiento += sizeof(int);

	memcpy(&(asignarValores.largoPuertoMemoria), buffer+desplazamiento, sizeof(int));
	desplazamiento += sizeof(int);

	memcpy(&(asignarValores.largomoduloAConectar), buffer+desplazamiento, sizeof(int));
	desplazamiento += sizeof(int);

	return asignarValores;
}


infoConfig limpiarConfig(infoConfig info, tamConfig tam){
	infoConfig infoLimpio;

	infoLimpio.ipKernel = string_substring_until(info.ipKernel, tam.largoIpKernel);
	infoLimpio.puertoKernel = string_substring_until(info.puertoKernel, tam.largoPuertoKernel);
	infoLimpio.ipMemoria = string_substring_until(info.ipMemoria, tam.largoIpMemoria);
	infoLimpio.puertoMemoria = string_substring_until(info.puertoMemoria, tam.largoPuertoMemoria);
	infoLimpio.moduloAConectar = string_substring_until(info.moduloAConectar, tam.largomoduloAConectar);

	return infoLimpio;
}


void liberarConfig(infoConfig info){
	free(info.ipKernel);
	free(info.puertoKernel);
	free(info.ipMemoria);
	free(info.puertoMemoria);
	free(info.moduloAConectar);
}


infoConfig desserializarConfig(void* buffer, int desplazamiento, tamConfig tam){
	infoConfig info;
	info.ipKernel = malloc(tam.largoIpKernel);
	info.puertoKernel = malloc(tam.largoPuertoKernel);
	info.ipMemoria = malloc(tam.largoIpMemoria);
	info.puertoMemoria = malloc(tam.largoPuertoMemoria);
	info.moduloAConectar = malloc(tam.largomoduloAConectar);

	memcpy(info.ipKernel, buffer+desplazamiento, tam.largoIpKernel);
	desplazamiento += tam.largoIpKernel;

	memcpy(info.puertoKernel, buffer+desplazamiento, tam.largoPuertoKernel);
	desplazamiento += tam.largoPuertoKernel;

	memcpy(info.ipMemoria, buffer+desplazamiento, tam.largoIpMemoria);
	desplazamiento += tam.largoIpMemoria;

	memcpy(info.puertoMemoria, buffer+desplazamiento, tam.largoPuertoMemoria);
	desplazamiento += tam.largoPuertoMemoria;

	memcpy(info.moduloAConectar, buffer+desplazamiento, tam.largomoduloAConectar);
	desplazamiento += tam.largomoduloAConectar;

	infoConfig configLimpio = limpiarConfig(info, tam);

	liberarConfig(info);

	return configLimpio;
}


void* serializarConfig(infoConfig info){
	void* buffer = malloc(sizeConfig(info));
	int desplazamiento = 0;

	memcpy(buffer, info.ipKernel, string_length(info.ipKernel));
	desplazamiento += string_length(info.ipKernel);

	memcpy(buffer+desplazamiento, info.puertoKernel, string_length(info.puertoKernel));
	desplazamiento += string_length(info.puertoKernel);

	memcpy(buffer+desplazamiento, info.ipMemoria, string_length(info.ipMemoria));
	desplazamiento += string_length(info.ipMemoria);

	memcpy(buffer+desplazamiento, info.puertoMemoria, string_length(info.puertoMemoria));
	desplazamiento += string_length(info.puertoMemoria);

	memcpy(buffer+desplazamiento, info.moduloAConectar, string_length(info.moduloAConectar));
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
	int desplazamiento = 0;

	memcpy(&(lib_ref->id), buffer, sizeof(int));
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






mate_instanceACambiar* desserializarCarpincho2(void* buffer) {
	mate_instanceACambiar* lib_ref = malloc(sizeof(mate_instanceACambiar));
	int desplazamiento = 4;

	memcpy(&(lib_ref->id), buffer, sizeof(int));
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











void liberarCarpincho(mate_instanceACambiar* carpincho){
	liberarConfig(carpincho->info);
	//list_destroy(carpincho->semaforos);
	free(carpincho);
}


void* serializarSemaforo(mate_sem_name semaforo) {
	int largoNombreSem = string_length(semaforo);
	void* buffer = malloc(largoNombreSem);

	memcpy(buffer, semaforo, largoNombreSem);

	return buffer;
}

void* serializarPointer(mate_pointer addr) {

	void* buffer = malloc(sizeof(int32_t));

	memcpy(buffer, &(addr), sizeof(int32_t));
	return buffer;

}


//------------------General Functions---------------------/

int mate_init(mate_instance *lib_ref, char *config) {

	int desplazamiento = 0;
	mate_instanceACambiar* carpincho = malloc(sizeof(mate_instanceACambiar));
	t_config* conf = config_create(config);
	carpincho->info = obtenerInfoConfigFinal(conf);

	tipoDePeticion peticion = NUEVOPROCESO;

	int tamCarpi = sizeDeCarpincho(carpincho);
	void* buffer = malloc(sizeof(tipoDePeticion) + tamCarpi);

	memcpy(buffer, &peticion, sizeof(tipoDePeticion));
	desplazamiento += sizeof(tipoDePeticion);

	void* carpin = serializarCarpincho(carpincho);

	memcpy(buffer + desplazamiento, carpin, sizeDeCarpincho(carpincho));
	desplazamiento += sizeDeCarpincho(carpincho);

	bool es = string_equals_ignore_case(carpincho->info.moduloAConectar, "KERNEL");
	if (es) {
		int conexionKernel = crearConexion(carpincho->info.ipKernel, carpincho->info.puertoKernel);

		send(conexionKernel, &desplazamiento, sizeof(int), 0);
		send(conexionKernel, buffer, desplazamiento, 0);


		liberarCarpincho(carpincho);


		//Recivo carpincho modificado
		int sizeCarpiNuevo;
		recv(conexionKernel, &sizeCarpiNuevo, sizeof(int), MSG_WAITALL);
		void* bufferRecv = malloc(sizeCarpiNuevo);
		recv(conexionKernel, bufferRecv, sizeCarpiNuevo, 0);

		mate_instanceACambiar* nuevoCarpi = desserializarCarpincho(bufferRecv);
		lib_ref->group_info = nuevoCarpi;


		close(conexionKernel);

		free(bufferRecv);
		free(carpin);

	} else {

		int conexionMemoria = crearConexion(carpincho->info.ipMemoria, carpincho->info.puertoMemoria);

		free(carpin);
		carpincho->conexionMemoria = conexionMemoria;
		carpincho->id = -1;
		void* carpin2 = serializarCarpincho(carpincho);
		memcpy(buffer + sizeof(tipoDePeticion), carpin2, sizeDeCarpincho(carpincho));

		send(carpincho->conexionMemoria, &desplazamiento, sizeof(int), 0);
		send(carpincho->conexionMemoria, buffer, desplazamiento, 0);

		liberarCarpincho(carpincho);


		//Recivo carpincho modificado
		int sizeCarpiNuevo;
		recv(conexionMemoria, &sizeCarpiNuevo, sizeof(int), MSG_WAITALL);
		void* bufferRecv = malloc(sizeCarpiNuevo);
		recv(conexionMemoria, bufferRecv, sizeCarpiNuevo, 0);

		mate_instanceACambiar* nuevoCarpi = desserializarCarpincho(bufferRecv);
		lib_ref->group_info = nuevoCarpi;

		close(conexionMemoria);

		free(bufferRecv);
		free(carpin2);
	}


	free(buffer);
	//config_destroy(conf);//Me tira invalid free() hay que ver bien PQ


	return 0;
}

int mate_close(mate_instance* lib_ref) {

	mate_instanceACambiar* carpincho = lib_ref->group_info;

	int desplazamiento = 0;

	tipoDePeticion peticion = LIBERARPROCESO;

	int tamCarpi = sizeDeCarpincho(carpincho);
	void* buffer = malloc(sizeof(tipoDePeticion) + tamCarpi);

	memcpy(buffer, &peticion, sizeof(tipoDePeticion));
	desplazamiento += sizeof(tipoDePeticion);

	void* carpin = serializarCarpincho(carpincho);

	memcpy(buffer + desplazamiento, carpin, tamCarpi);
	desplazamiento += tamCarpi;


	if (string_equals_ignore_case(carpincho->info.moduloAConectar, "KERNEL") && carpincho->id > 0) {

		int conexionKernel = crearConexion(carpincho->info.ipKernel, carpincho->info.puertoKernel);

		send(conexionKernel, &desplazamiento, sizeof(int), 0);
		send(conexionKernel, buffer, desplazamiento, 0);


		liberarCarpincho(carpincho);
		free(buffer);
		free(carpin);
		close(conexionKernel);

	}else{
		int conexionMemoria= crearConexion(carpincho->info.ipMemoria, carpincho->info.puertoMemoria);

		send(conexionMemoria, &desplazamiento, sizeof(int), 0);
		send(conexionMemoria, buffer, desplazamiento, 0);


		liberarCarpincho(carpincho);
		free(buffer);
		free(carpin);
		close(conexionMemoria);
	}

	return 0;
}



//-----------------Semaphore Functions---------------------/

int mate_sem_init(mate_instance* lib_ref, mate_sem_name sem, unsigned int value) {

	mate_instanceACambiar* carpincho = lib_ref->group_info;
	
	if (string_equals_ignore_case(carpincho->info.moduloAConectar, "KERNEL") && carpincho->id > 0) {

		int conexionKernel = crearConexion(carpincho->info.ipKernel, carpincho->info.puertoKernel);

		int desplazamiento = 0;
		int bytesCarpincho = sizeDeCarpincho(carpincho);
		int bytesSem = string_length(sem) +1;

		tipoDePeticion peticion = NUEVOSEMAFORO;

		void* buffer = malloc(sizeof(tipoDePeticion)+bytesCarpincho+bytesSem+sizeof(int)*2);

		memcpy(buffer, &peticion, sizeof(tipoDePeticion));
		desplazamiento += sizeof(tipoDePeticion);

		void* carpinSerializado = serializarCarpincho(carpincho);
		memcpy(buffer + desplazamiento, carpinSerializado, bytesCarpincho);
		desplazamiento += bytesCarpincho;

		memcpy(buffer + desplazamiento, &value, sizeof(int));
		desplazamiento += sizeof(int);
		
		memcpy(buffer + desplazamiento, &bytesSem, sizeof(int));
		desplazamiento += sizeof(int);
		
		memcpy(buffer + desplazamiento, sem, bytesSem);
		desplazamiento += bytesSem;

		//Envio las cosas necesarias
		send(conexionKernel, &desplazamiento, sizeof(int), 0);
		send(conexionKernel, buffer, desplazamiento, 0);


		//Recivo carpincho modificado
		int sizeCarpiNuevo;
		recv(conexionKernel, &sizeCarpiNuevo, sizeof(int), MSG_WAITALL);
		void* bufferRecv = malloc(sizeCarpiNuevo);
		recv(conexionKernel, bufferRecv, sizeCarpiNuevo, 0);

		mate_instanceACambiar* nuevoCarpi = desserializarCarpincho(bufferRecv);
		lib_ref->group_info = nuevoCarpi;

		close(conexionKernel);
	} else {
		return -1;
	}

	return 0;
}

int mate_sem_wait(mate_instance *lib_ref, mate_sem_name sem) {

	mate_instanceACambiar* carpincho = lib_ref->group_info;

	if (string_equals_ignore_case(carpincho->info.moduloAConectar, "KERNEL") && carpincho->id > 0) {
		
		int conexionKernel = crearConexion(carpincho->info.ipKernel, carpincho->info.puertoKernel);

		tipoDePeticion peticion = SEMWAIT;
		int bytesCarpincho = sizeDeCarpincho(carpincho);
		int bytesSem = string_length(sem) + 1;

		void* buffer = malloc(sizeof(tipoDePeticion)+bytesCarpincho+bytesSem+sizeof(int));
		int desplazamiento = 0;

		memcpy(buffer, &(peticion), sizeof(tipoDePeticion));
		desplazamiento += sizeof(tipoDePeticion);

		void* carpinSerializado = serializarCarpincho(carpincho);
		memcpy(buffer + desplazamiento, carpinSerializado, bytesCarpincho);
		desplazamiento += bytesCarpincho;

		memcpy(buffer + desplazamiento, &bytesSem, sizeof(int));
		desplazamiento += sizeof(int);
		
		memcpy(buffer + desplazamiento, sem, bytesSem);
		desplazamiento += bytesSem;


		send(conexionKernel, &desplazamiento, sizeof(int), 0);
		send(conexionKernel, buffer, desplazamiento, 0);

		//Recivo carpincho modificado
		int sizeCarpiNuevo;
		recv(conexionKernel, &sizeCarpiNuevo, sizeof(int), MSG_WAITALL);
		void* bufferRecv = malloc(sizeCarpiNuevo);
		recv(conexionKernel, bufferRecv, sizeCarpiNuevo, 0);

		mate_instanceACambiar* nuevoCarpi = desserializarCarpincho(bufferRecv);
		lib_ref->group_info = nuevoCarpi;

		close(conexionKernel);

	} else {
		return -1;
	}

	return 0;

}

int mate_sem_post(mate_instance *lib_ref, mate_sem_name sem) {

	mate_instanceACambiar* carpincho = lib_ref->group_info;

	if (string_equals_ignore_case(carpincho->info.moduloAConectar, "KERNEL") && carpincho->id > 0) {
		
		int conexionKernel = crearConexion(carpincho->info.ipKernel, carpincho->info.puertoKernel);

		tipoDePeticion peticion = SEMPOST;
		int bytesCarpincho = sizeDeCarpincho(carpincho);
		int bytesSem = string_length(sem) + 1 ;

		void* buffer = malloc(sizeof(tipoDePeticion)+bytesCarpincho+bytesSem+sizeof(int));
		int desplazamiento = 0;

		memcpy(buffer, &(peticion), sizeof(tipoDePeticion));
		desplazamiento += sizeof(tipoDePeticion);

		void* carpinSerializado = serializarCarpincho(carpincho);
		memcpy(buffer + desplazamiento, carpinSerializado, bytesCarpincho);
		desplazamiento += bytesCarpincho;

		memcpy(buffer + desplazamiento, &bytesSem, sizeof(int));
		desplazamiento += sizeof(int);
		
		memcpy(buffer + desplazamiento, sem, bytesSem);
		desplazamiento += bytesSem;

		send(conexionKernel, &desplazamiento, sizeof(int), 0);
		send(conexionKernel, buffer, desplazamiento, 0);

		//Recivo carpincho modificado
		int sizeCarpiNuevo;
		recv(conexionKernel, &sizeCarpiNuevo, sizeof(int), MSG_WAITALL);
		void* bufferRecv = malloc(sizeCarpiNuevo);
		recv(conexionKernel, bufferRecv, sizeCarpiNuevo, 0);

		mate_instanceACambiar* nuevoCarpi = desserializarCarpincho(bufferRecv);
		lib_ref->group_info = nuevoCarpi;

		close(conexionKernel);
	} else {
		return -1;
	}

	return 0;
}

int mate_sem_destroy(mate_instance *lib_ref, mate_sem_name sem) {

	
	mate_instanceACambiar* carpincho = lib_ref->group_info;

	if (string_equals_ignore_case(carpincho->info.moduloAConectar, "KERNEL") && carpincho->id > 0) {
		
		int conexionKernel = crearConexion(carpincho->info.ipKernel, carpincho->info.puertoKernel);

		tipoDePeticion peticion = ELIMINARSEMAFORO;
		int bytesCarpincho = sizeDeCarpincho(carpincho);
		int bytesSem = string_length(sem) + 1;

		void* buffer = malloc(sizeof(tipoDePeticion)+bytesCarpincho+bytesSem+sizeof(int));
		int desplazamiento = 0;

		memcpy(buffer, &(peticion), sizeof(tipoDePeticion));
		desplazamiento += sizeof(tipoDePeticion);

		void* carpinSerializado = serializarCarpincho(carpincho);
		memcpy(buffer + desplazamiento, carpinSerializado, bytesCarpincho);
		desplazamiento += bytesCarpincho;

		memcpy(buffer + desplazamiento, &bytesSem, sizeof(int));
		desplazamiento += sizeof(int);
		
		memcpy(buffer + desplazamiento, sem, bytesSem);
		desplazamiento += bytesSem;

		send(conexionKernel, &desplazamiento, sizeof(int), 0);
		send(conexionKernel, buffer, desplazamiento, 0);

		//Recivo carpincho modificado
		int sizeCarpiNuevo;
		recv(conexionKernel, &sizeCarpiNuevo, sizeof(int), MSG_WAITALL);
		void* bufferRecv = malloc(sizeCarpiNuevo);
		recv(conexionKernel, bufferRecv, sizeCarpiNuevo, 0);

		mate_instanceACambiar* nuevoCarpi = desserializarCarpincho(bufferRecv);
		lib_ref->group_info = nuevoCarpi;

		close(conexionKernel);
	} else {
		return -1;
	}

	return 0;
}

//--------------------IO Functions------------------------/

int mate_call_io(mate_instance* lib_ref, mate_io_resource io, void *msg) {

	mate_instanceACambiar* carpincho = lib_ref->group_info;
	int bytesCarpincho = sizeDeCarpincho(carpincho);
	int bytesIO = string_length(io)+1;

	tipoDePeticion peticion = NUEVAIO;
	void* buffer = malloc(sizeof(tipoDePeticion)+bytesCarpincho+bytesIO+sizeof(int));
	int desplazamiento = 0;

	memcpy(buffer, &(peticion), sizeof(tipoDePeticion));
	desplazamiento += sizeof(tipoDePeticion);

	void* carpinSerializado = serializarCarpincho(carpincho);
	memcpy(buffer + desplazamiento, carpinSerializado, bytesCarpincho);
	desplazamiento += bytesCarpincho;

	memcpy(buffer + desplazamiento, &bytesIO, sizeof(int));
	desplazamiento += sizeof(int);

	memcpy(buffer + desplazamiento, io, bytesIO);
	desplazamiento += bytesIO;


	if (string_equals_ignore_case(carpincho->info.moduloAConectar, "KERNEL")) {

		int conexionKernel = crearConexion(carpincho->info.ipKernel, carpincho->info.puertoKernel);
//		printf("Conexion: %d y ID: %d\n", conexionKernel, carpincho->id);

		send(conexionKernel, &desplazamiento, sizeof(int), 0);
		send(conexionKernel, buffer, desplazamiento, 0);

		//Recivo carpincho modificado
		int sizeCarpiNuevo;
		recv(conexionKernel, &sizeCarpiNuevo, sizeof(int), MSG_WAITALL);
		void* bufferRecv = malloc(sizeCarpiNuevo);
		recv(conexionKernel, bufferRecv, sizeCarpiNuevo, 0);

		mate_instanceACambiar* nuevoCarpi = desserializarCarpincho(bufferRecv);
		lib_ref->group_info = nuevoCarpi;

		close(conexionKernel);
	}else{

		send(carpincho->conexionMemoria, &desplazamiento, sizeof(int), 0);
		send(carpincho->conexionMemoria, buffer, desplazamiento, 0);
	}

	return 0;
}



//--------------Memory Module Functions-------------------/

mate_pointer mate_memalloc(mate_instance* lib_ref, int size) {
	mate_instanceACambiar* carpincho = lib_ref->group_info;

	int bytesCarpincho = sizeDeCarpincho(carpincho);

	tipoDePeticion peticion = PEDIRMEMORIA;
	void* buffer = malloc(sizeof(int) + bytesCarpincho + sizeof(tipoDePeticion));
	int desplazamiento = 0;

	memcpy(buffer, &(peticion), sizeof(tipoDePeticion));
	desplazamiento += sizeof(tipoDePeticion);

	void* carpinSerializado = serializarCarpincho(carpincho);
	memcpy(buffer + desplazamiento, carpinSerializado, bytesCarpincho);
	desplazamiento += bytesCarpincho;

	memcpy(buffer + desplazamiento, &(size), sizeof(int));
	desplazamiento += sizeof(int);

	mate_pointer direccion_alloc;
	if (string_equals_ignore_case(carpincho->info.moduloAConectar, "KERNEL")) {

		int conexion = crearConexion(carpincho->info.ipKernel, carpincho->info.puertoKernel);
		send(conexion, &desplazamiento, sizeof(int), 0);
		send(conexion, buffer, desplazamiento, 0);


		recv(conexion, &direccion_alloc, sizeof(mate_pointer),MSG_WAITALL);

		close(conexion);

	}else{

		int conexion = crearConexion(carpincho->info.ipMemoria, carpincho->info.puertoMemoria);
		send(conexion, &desplazamiento, sizeof(int), 0);
		send(conexion, buffer, desplazamiento, 0);


		recv(conexion, &direccion_alloc, sizeof(mate_pointer), MSG_WAITALL);
		close(conexion);
	}



	free(buffer);
	free(carpinSerializado);

	return direccion_alloc;
}

int mate_memfree(mate_instance *lib_ref, mate_pointer addr) {
	mate_instanceACambiar* carpincho = lib_ref->group_info;

	int bytesCarpincho = sizeDeCarpincho(carpincho);
	tipoDePeticion peticion = LIBERARMEMORIA;
	void* buffer = malloc(sizeof(mate_pointer) + bytesCarpincho+ sizeof(tipoDePeticion));
	int desplazamiento = 0;

	memcpy(buffer, &(peticion), sizeof(tipoDePeticion));
	desplazamiento += sizeof(tipoDePeticion);

	void* carpinchoSerializado = serializarCarpincho(carpincho);
	memcpy(buffer + desplazamiento, carpinchoSerializado, bytesCarpincho);
	desplazamiento += bytesCarpincho;

	memcpy(buffer + desplazamiento, &addr, sizeof(mate_pointer));
	desplazamiento += sizeof(mate_pointer);

	int respuesta;

	if (string_equals_ignore_case(carpincho->info.moduloAConectar, "KERNEL")) {

		int conexion = crearConexion(carpincho->info.ipKernel, carpincho->info.puertoKernel);

		send(conexion, &desplazamiento, sizeof(int), 0);
		send(conexion, buffer, desplazamiento, 0);

		recv(conexion, &respuesta, sizeof(int), MSG_WAITALL);

		close(conexion);

	}else{

		int conexion = crearConexion(carpincho->info.ipMemoria, carpincho->info.puertoMemoria);

		send(conexion, &desplazamiento, sizeof(int), 0);
		send(conexion, buffer, desplazamiento, 0);

		recv(conexion, &respuesta, sizeof(int), MSG_WAITALL);
		close(conexion);
	}


	free(buffer);
	free(carpinchoSerializado);

	return respuesta;

}

int mate_memread(mate_instance* lib_ref, mate_pointer origin, void* dest, int size){ //size es el tamanio de dest
	mate_instanceACambiar* carpincho = lib_ref->group_info;

	int bytesCarpincho = sizeDeCarpincho(carpincho);

	tipoDePeticion peticion = LEERMEMORIA;
	void* buffer = malloc(sizeof(tipoDePeticion) + bytesCarpincho + sizeof(mate_pointer) +size + sizeof(int));
	int desplazamiento = 0;

	memcpy(buffer, &(peticion), sizeof(tipoDePeticion));
	desplazamiento += sizeof(tipoDePeticion);

	void* carpi = serializarCarpincho(carpincho);
	memcpy(buffer + desplazamiento, carpi, bytesCarpincho);
	desplazamiento += bytesCarpincho;

	memcpy(buffer + desplazamiento, &origin, sizeof(mate_pointer));
	desplazamiento += sizeof(mate_pointer);

	memcpy(buffer + desplazamiento, &size, sizeof(int));
	desplazamiento += sizeof(int);

	int respuesta;
	if (string_equals_ignore_case(carpincho->info.moduloAConectar, "KERNEL")) {

		int conexion = crearConexion(carpincho->info.ipKernel, carpincho->info.puertoKernel);

		send(conexion, &desplazamiento, sizeof(int), 0);
		send(conexion, buffer, desplazamiento, 0);

		recv(conexion, &respuesta, sizeof(int), MSG_WAITALL);
		recv(conexion, dest, size, 0);

		close(conexion);

	}else{

		int conexion = crearConexion(carpincho->info.ipMemoria, carpincho->info.puertoMemoria);

		send(conexion, &desplazamiento, sizeof(int), 0);
		send(conexion, buffer, desplazamiento, 0);

		recv(conexion, &respuesta, sizeof(int), MSG_WAITALL);
		if(respuesta == 1){
			recv(conexion, dest, size, MSG_WAITALL);
		}
		close(conexion);
	}

	free(carpi);
	free(buffer);

	return respuesta;
}

int mate_memwrite(mate_instance* lib_ref, void *origin, mate_pointer dest, int size){
	mate_instanceACambiar* carpincho = lib_ref->group_info;

	int bytesCarpincho = sizeDeCarpincho(carpincho);
	tipoDePeticion peticion = ESCRIBIRMEMORIA;
	void* buffer = malloc(sizeof(tipoDePeticion) + bytesCarpincho + size + sizeof(mate_pointer) + size + sizeof(int));
	int desplazamiento = 0;

	memcpy(buffer, &(peticion), sizeof(tipoDePeticion));
	desplazamiento += sizeof(tipoDePeticion);

	void* carpinchoSerializado = serializarCarpincho(carpincho);
	memcpy(buffer + desplazamiento, carpinchoSerializado, bytesCarpincho);
	desplazamiento += bytesCarpincho;

	memcpy(buffer + desplazamiento, &dest, sizeof(mate_pointer));
	desplazamiento += sizeof(mate_pointer);

	memcpy(buffer + desplazamiento, &size, sizeof(int));
	desplazamiento += sizeof(int);

	memcpy(buffer + desplazamiento, origin, size);
	desplazamiento += size;

	int respuesta;

	if (string_equals_ignore_case(carpincho->info.moduloAConectar, "KERNEL")) {

		int conexion = crearConexion(carpincho->info.ipKernel, carpincho->info.puertoKernel);

		send(conexion, &desplazamiento, sizeof(int), 0);
		send(conexion, buffer, desplazamiento, 0);

		recv(conexion, &respuesta, sizeof(int), MSG_WAITALL);

		close(conexion);


	}else{

		int conexion = crearConexion(carpincho->info.ipMemoria, carpincho->info.puertoMemoria);

		send(conexion, &desplazamiento, sizeof(int), 0);
		send(conexion, buffer, desplazamiento, 0);


		recv(conexion, &respuesta, sizeof(int), MSG_WAITALL);
		close(conexion);
	}


	free(buffer);
	free(carpinchoSerializado);

	return respuesta;
}
