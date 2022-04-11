#include "Modulo_SWamP.h"

int main(void) {


	//INICIALIZO EL LOG Y EL CONFIG

	logSWamP = log_create("SWamP.log", "SWamP", 1, LOG_LEVEL_INFO);
	log_info(logSWamP, "INICIANDO LOG SWamP...");

	configSWamP = config_create("/home/utnso/tp-2021-2c-Los-Picateclas/Modulo_SWamP/SWamP.config");
	log_info(logSWamP, "CREANDO CONFIG SWamP...");

	if(configSWamP == NULL){
		log_error(logSWamP, "NO SE PUDO CREAR EL CONFIG...");
		exit(-3);
	}
	
	semArchivos = malloc(sizeof(sem_t));
	sem_init(semArchivos, 0, 1);

	infoConfig = obtenerInfoConfig();
	
	listaDeParticiones = list_create();
	pagsEnParticiones  = list_create();
	maxCantDePaginasEnParticion = infoConfig.tamSwap/infoConfig.tamPagina;

	iniciarParticiones();
	cfr = 0;

	pthread_t* hiloSincronizador = malloc(sizeof(pthread_t));
	pthread_create(hiloSincronizador, NULL, (void*)sincronizar, NULL);
	pthread_detach(*hiloSincronizador);
	
	int servidor = iniciarServidor(infoConfig.puerto);
	conexion = esperarCliente(servidor);

	while(1){

		procesarPeticion();

	}

	

	//Liberamos memoria
	liberarParticiones();
	close(conexion);
	
	return EXIT_SUCCESS;
}



//TAMANIO=96
//TAMANIO_PAGINA=32





void iniciarParticiones(){
	int i;
	bool flagCreado = false;

	for(i=0; infoConfig.archivosSwap[i] != NULL; i++){
		archivoParticion* detallesParticion = malloc(sizeof(archivoParticion));
		char* pathParticion = infoConfig.archivosSwap[i];
		int particion = open(pathParticion, O_CREAT|O_RDWR, S_IRUSR|S_IWUSR);
		
		struct stat infoParticion;
		stat(pathParticion, &infoParticion);
		int sizeParticion = infoParticion.st_size;
		
		if(sizeParticion == 0){
			sizeParticion = infoConfig.tamSwap;
			ftruncate(particion, sizeParticion);
			flagCreado = true;
		}
		
		void* particionPointer = mmap(NULL, sizeParticion, PROT_READ|PROT_WRITE, MAP_SHARED, particion, 0);
		
		if(flagCreado == true){
			char* llenadoVacio = string_repeat('\0', sizeParticion);
			memcpy(particionPointer, llenadoVacio, sizeParticion);
			free(llenadoVacio);
		}
		
		detallesParticion->ID = i;
		detallesParticion->punteroAMemoria = particionPointer;
		detallesParticion->cantDePaginasOcupadas = 0;
		detallesParticion->bitmapDeFrames = iniciarBitmapDeParticiones();
//		detallesParticion->infoDePaginas = list_create();

		list_add(listaDeParticiones, detallesParticion);

		close(particion);
	}
}


t_bitarray* iniciarBitmapDeParticiones(){
	t_bitarray* bitmap;

	int cantDeFrames = infoConfig.tamSwap/infoConfig.tamPagina;
	int bitsParaBitmap = cantDeFrames/8+1;
	char* reservar = malloc(bitsParaBitmap);
	bitmap = bitarray_create(reservar, bitsParaBitmap);

	for(int i=0; i<cantDeFrames; i++){
		bitarray_clean_bit(bitmap, i);
	}

	return bitmap;
}

void liberarParticiones(){
	int largoLista = list_size(listaDeParticiones);
	for(int i=0; i<largoLista; i++){
		archivoParticion* detallesParticion = (archivoParticion*) list_get(listaDeParticiones, i);
		munmap(detallesParticion->punteroAMemoria, infoConfig.tamSwap);
		bitarray_destroy(detallesParticion->bitmapDeFrames);
	}
	list_destroy(listaDeParticiones);
}


infConf obtenerInfoConfig(){
	infConf info;
	
	info.ip = config_get_string_value(configSWamP, "IP");
	info.puerto = config_get_string_value(configSWamP, "PUERTO");
	info.tamSwap = config_get_int_value(configSWamP, "TAMANIO_SWAP");
	info.tamPagina = config_get_int_value(configSWamP, "TAMANIO_PAGINA");
  	info.archivosSwap = config_get_array_value(configSWamP, "ARCHIVOS_SWAP");
  	info.marcosMaximos = config_get_int_value(configSWamP, "MARCOS_MAXIMOS");
  	info.retardoSwap = config_get_int_value(configSWamP, "RETARDO_SWAP");
	info.tipoAsignacion = config_get_string_value(configSWamP, "TIPO_ASIGNACION");
	
	return info;
}


void* sincronizar(){
	while(1){
		sleep(5);
		sem_wait(semArchivos);
		int largoLista = list_size(listaDeParticiones);
		for(int i=0; i<largoLista; i++){
			archivoParticion* detallesParticion = (archivoParticion*) list_get(listaDeParticiones, i);
			msync(detallesParticion->punteroAMemoria, infoConfig.tamSwap ,MS_SYNC);
		}
		sem_post(semArchivos);
	}
}

//FUNCIONES PARA LA CONEXION




int iniciarServidor(char* puerto) {
	struct addrinfo hints, *servinfo;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(NULL, puerto, &hints, &servinfo);

	int socketServidor = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
	if (socketServidor == -1){
		perror("Error");
		exit(-1);
	}

	int bindReturn = bind(socketServidor, servinfo->ai_addr, servinfo->ai_addrlen);
	if (bindReturn == -1) {
		perror("Error");
		close(socketServidor);
	    exit(-1);
	}

	int escucha = listen(socketServidor, SOMAXCONN);
	if(escucha == -1){
		perror("Error");
	}
	freeaddrinfo(servinfo);
	return socketServidor;
}


int esperarCliente(int socketServidor) {
	struct sockaddr_in dirCliente;
	socklen_t tamDireccion = sizeof(struct sockaddr_in);

	int socketCliente = accept(socketServidor, (void*) &dirCliente, &tamDireccion);

	if(socketCliente == -1){
		perror("Error");
	}

	return socketCliente;
}







//int iniciarServidor(char* ip, char* puerto, t_log * log) {
//		int socketServidor;
//	    struct addrinfo hints, *servinfo, *p;
//
//	    memset(&hints, 0, sizeof(hints));
//	    hints.ai_family = AF_UNSPEC;
//	    hints.ai_socktype = SOCK_STREAM;
//	    hints.ai_flags = AI_PASSIVE;
//
//	    getaddrinfo(ip, puerto, &hints, &servinfo);
//
//	    for (p=servinfo; p != NULL; p = p->ai_next)
//	    {
//	        if ((socketServidor = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
//	            continue;
//
//	        if (bind(socketServidor, p->ai_addr, p->ai_addrlen) == -1) {
//	        	printf("%s\n", strerror(errno));
//	        	close(socketServidor);
//	            continue;
//	        }
//	        break;
//	    }
//
//
//		listen(socketServidor, SOMAXCONN);
//
//	    freeaddrinfo(servinfo);
//
//	    log_info(log, "SERVIDOR LISTO PARA RECIBIR CLIENTES...");
//
//	    return socketServidor;
//}


//int esperarCliente(int socketServidor, t_log * log) {
//	struct sockaddr_in dirCliente;
//	socklen_t tamDireccion = sizeof(struct sockaddr_in);
//
//	int socketCliente = accept(socketServidor, (void*) &dirCliente, &tamDireccion);
////	int socketCliente = accept(socketServidor, NULL, NULL);
//
//	printf("%s\n", strerror(errno));
//
//	log_info(log, "SE CONECTO UN CLIENTE!!!!");
//
//	return socketCliente;
//}
