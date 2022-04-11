#include "peticiones.h"

void procesarPeticion(){

	usleep(infoConfig.retardoSwap*1000);

	size_t bytesARecibir = 0;
	int valorRecv1 = recv(conexion, &bytesARecibir, sizeof(size_t), MSG_WAITALL);
	void* buffer = malloc(bytesARecibir);
	recv(conexion, buffer, bytesARecibir, 0);

	cfr++;
//	printf("----------------------------------------------------\n");
//	printf("Peticion numero: %d\n", cfr);
//
//	printf("Valor del recv 1: %d. Con bytes=%d\n", valorRecv1, bytesARecibir);

	infoPeticion* pedido = malloc(sizeof(infoPeticion));

	memcpy(&(pedido->pedido), buffer, sizeof(int));
	int desplazamiento = sizeof(tipoDePeticion);
	memcpy(&(pedido->nroDePag), buffer+desplazamiento, sizeof(int));
	desplazamiento += sizeof(int);
	memcpy(&(pedido->proceso), buffer+desplazamiento, sizeof(int));
	desplazamiento += sizeof(int);

	if(pedido->pedido == LECTURA){
		pedido->pagina = NULL;

		printf("Peticion: LECTURA.\n");


		if(string_equals_ignore_case(infoConfig.tipoAsignacion, "FIJA")){
			envioDePaginaAsigFija(*pedido);
		}else if(string_equals_ignore_case(infoConfig.tipoAsignacion, "DINAMICA")){
			envioDePaginaAsigDinam(*pedido);
		}
	}else if(pedido->pedido == ESCRITURA){
		pedido->pagina = malloc(infoConfig.tamPagina);
		memcpy(pedido->pagina, buffer+desplazamiento, infoConfig.tamPagina);


		//Prueba-------------------------------------------------------
		printf("Peticion: ESCRITURA.\n");

//		char* pagQueLlego = malloc(infoConfig.tamPagina);
//		memcpy(pagQueLlego, pedido->pagina, infoConfig.tamPagina);

//		printf("PAGINA QUE LLEGO: %s\n", pagQueLlego);
//		printf("----------------------------------------------------\n\n");

		//Fin Prueba---------------------------------------------------




		if(string_equals_ignore_case(infoConfig.tipoAsignacion, "FIJA")){
			escribirPaginaAsigFija(*pedido);
		}else if(string_equals_ignore_case(infoConfig.tipoAsignacion, "DINAMICA")){
			escribirPaginaAsigDinam(*pedido);
		}
	}else if(pedido->pedido == ELIMINADO){
		pedido->pagina = NULL;

		printf("Peticion: ELIMINADO.\n");

		if(string_equals_ignore_case(infoConfig.tipoAsignacion, "FIJA")){
			eliminarPaginaAsigFija(*pedido);
		}else if(string_equals_ignore_case(infoConfig.tipoAsignacion, "DINAMICA")){
			eliminarPaginasAsigDinam(*pedido);
		}
	}
	free(buffer);
	free(pedido->pagina);
	free(pedido);
}





//ASIGNACION FIJA

//Leer
void envioDePaginaAsigFija(infoPeticion pedido){
	paginaEscritaFija* infoPag = existeElProcesoSwap(pedido);

	if(infoPag != NULL){
		archivoParticion* particion = obtenerInfoParticion(infoPag->IdDeParticion);

		int pagALeer = infoPag->lugarPrimeraPag+pedido.nroDePag;
		void* lugarDeLectura = particion->punteroAMemoria + pagALeer*infoConfig.tamPagina;

		void* buffer = malloc(infoConfig.tamPagina);
		sem_wait(semArchivos);
		memcpy(buffer, lugarDeLectura, infoConfig.tamPagina);
		sem_post(semArchivos);

		//Preguntar si es necesario enviarle el tam de pag
		send(conexion, buffer, infoConfig.tamPagina, MSG_NOSIGNAL);

		free(buffer);
	}else {
		//ERROR
	}
}

//Escribir
void escribirPaginaAsigFija(infoPeticion pedido){

	paginaEscritaFija* infoPag = existeElProcesoSwap(pedido);

	if(infoPag != NULL){
		archivoParticion* particion = obtenerInfoParticion(infoPag->IdDeParticion);

		int pagAEscribirEnParticion = infoPag->lugarPrimeraPag+pedido.nroDePag;
		void* lugarDeEscritura = particion->punteroAMemoria + pagAEscribirEnParticion*infoConfig.tamPagina;

		sem_wait(semArchivos);
		//Agregar info de la pag a la estructura administrativa infoDePaginas(t_list*)
		memcpy(lugarDeEscritura, pedido.pagina, infoConfig.tamPagina);
		sem_post(semArchivos);

		reemplazarEstructAdminParticion(particion);

	}else if(infoPag == NULL){
		paginaEscritaFija* nuevaPag = malloc(sizeof(paginaEscritaFija));

		//Escribo nueva pagina y reservar lugar para un nuevo proceso
		archivoParticion* particion = particionAEscribir();

		if(particion->cantDePaginasOcupadas+infoConfig.marcosMaximos <= maxCantDePaginasEnParticion){

			//marcar en el bitmap los x "frames" a reservar
			int hayEspacio = chequeoEspacioEnParticion(particion);

			if(hayEspacio != -1){
				particion->cantDePaginasOcupadas += infoConfig.marcosMaximos;
				nuevaPag->lugarPrimeraPag = hayEspacio;

				//Inicializo nuevo grupo de paginas de un proceso y la meto en la lista
				nuevaPag->IdDeParticion = particion->ID;
				nuevaPag->proceso = pedido.proceso;
				list_add(pagsEnParticiones, nuevaPag);

				int pagAEscribirEnParticion = nuevaPag->lugarPrimeraPag+pedido.nroDePag;
				void* lugarDeEscritura = particion->punteroAMemoria + pagAEscribirEnParticion*infoConfig.tamPagina;

				sem_wait(semArchivos);
				//Agregar info de la pag a la estructura administrativa infoDePaginas(t_list*)
				memcpy(lugarDeEscritura, pedido.pagina, infoConfig.tamPagina);
				sem_post(semArchivos);

			}
		}else{
			//ERROR
		}
		reemplazarEstructAdminParticion(particion);
	}
}

//Eliminar
void eliminarPaginaAsigFija(infoPeticion pedido){
	paginaEscritaFija* infoPag = existeElProcesoSwap(pedido);
	archivoParticion* particion = obtenerInfoParticion(infoPag->IdDeParticion);


	int size = infoConfig.tamPagina*infoConfig.marcosMaximos;
	char* llenadoVacio = string_repeat('\0', size);

	void* lugarAEliminar = particion->punteroAMemoria + infoPag->lugarPrimeraPag*infoConfig.tamPagina;

	eliminarProcesoDeEstrAdmin(infoPag, particion);

	sem_wait(semArchivos);
	memcpy(lugarAEliminar,llenadoVacio,size);
	sem_post(semArchivos);

	reemplazarEstructAdminParticion(particion);

}









//ASIGNACION DINAMICA

//Leer
void envioDePaginaAsigDinam(infoPeticion pedido){

	paginaEscritaDinam* infoPag = existeElProcesoSwapDinam(pedido);

	if(infoPag != NULL){

		archivoParticion* particion = obtenerInfoParticion(infoPag->IdDeParticion);
		int paginaALeer = buscarPaginaEnDinam(infoPag, pedido.nroDePag);

		void* aLeer = particion->punteroAMemoria + paginaALeer*infoConfig.tamPagina;

		void* buffer = malloc(infoConfig.tamPagina);
		sem_wait(semArchivos);
		memcpy(buffer, aLeer, infoConfig.tamPagina);
		sem_post(semArchivos);

		send(conexion, buffer, infoConfig.tamPagina, MSG_NOSIGNAL);

//		char* pagLeida = malloc(infoConfig.tamPagina);
//		memcpy(pagLeida, buffer, infoConfig.tamPagina);

//		printf("Pagina leida: %s\n", pagLeida);
//		printf("----------------------------------------------------\n\n");

		free(buffer);

	}else{
		//ERROR
	}

}

//Escribir
void escribirPaginaAsigDinam(infoPeticion pedido){

	paginaEscritaDinam* infoPag = existeElProcesoSwapDinam(pedido);

	if(infoPag != NULL){

		archivoParticion* particion = obtenerInfoParticion(infoPag->IdDeParticion);
		int seVaASobreescribir = yaExisteEstaPagina(infoPag, pedido);

		if(seVaASobreescribir != -1){

			eliminarPagina(particion, seVaASobreescribir);

			pagYLugar* infoDeEscritura = malloc(sizeof(pagYLugar));
			infoDeEscritura->nroPag = pedido.nroDePag;
			infoDeEscritura->pagQueOcupaEnParticion = seVaASobreescribir;

			list_add(infoPag->listaDePagsOcupadas, infoDeEscritura);

			void* lugarAEscribirEnSwap = particion->punteroAMemoria + seVaASobreescribir*infoConfig.tamPagina;

			sem_wait(semArchivos);
			memcpy(lugarAEscribirEnSwap, pedido.pagina, infoConfig.tamPagina);
			sem_post(semArchivos);

			bool yaNoSirveMas(pagYLugar* inf){
				return inf->nroPag == pedido.nroDePag;
			}

			list_remove_and_destroy_by_condition(infoPag->listaDePagsOcupadas, (void*) yaNoSirveMas, free);

		}else{

			int lugarAEscribir = proxPagLibre(particion);

			if(lugarAEscribir != -1){
				pagYLugar* infoDeEscritura = malloc(sizeof(pagYLugar));
				infoDeEscritura->nroPag = pedido.nroDePag;
				infoDeEscritura->pagQueOcupaEnParticion = lugarAEscribir;

				list_add(infoPag->listaDePagsOcupadas, infoDeEscritura);

				particion->cantDePaginasOcupadas++;

				void* lugarAEscribirEnSwap = particion->punteroAMemoria + lugarAEscribir*infoConfig.tamPagina;

				sem_wait(semArchivos);
				memcpy(lugarAEscribirEnSwap, pedido.pagina, infoConfig.tamPagina);
				sem_post(semArchivos);
			}
		}
		reemplazarEstructAdminParticion(particion);


	}else if(infoPag == NULL){

		paginaEscritaDinam* nuevaPag = malloc(sizeof(paginaEscritaDinam));
		nuevaPag->listaDePagsOcupadas = list_create();

		//Escribo nueva pagina y reservar lugar para un nuevo proceso
		archivoParticion* particion = particionAEscribir();

		int nuevoCantOcupadas = particion->cantDePaginasOcupadas+1;
		if( nuevoCantOcupadas <= maxCantDePaginasEnParticion){
			int lugarAEscribir = proxPagLibre(particion);

			if(lugarAEscribir != -1){

				pagYLugar* infoDeEscritura = malloc(sizeof(pagYLugar));
				infoDeEscritura->nroPag = pedido.nroDePag;
				infoDeEscritura->pagQueOcupaEnParticion = lugarAEscribir;

				list_add(nuevaPag->listaDePagsOcupadas, infoDeEscritura);
				nuevaPag->IdDeParticion = particion->ID;
				nuevaPag->proceso = pedido.proceso;

				list_add(pagsEnParticiones, nuevaPag);

				particion->cantDePaginasOcupadas++;

				void* lugarAEscribirEnSwap = particion->punteroAMemoria + lugarAEscribir*infoConfig.tamPagina;

				sem_wait(semArchivos);
				memcpy(lugarAEscribirEnSwap, pedido.pagina, infoConfig.tamPagina);
				sem_post(semArchivos);
			}

		}else{
			//ERROR
		}
		reemplazarEstructAdminParticion(particion);
	}


}

//Eliminar
void eliminarPaginasAsigDinam(infoPeticion pedido){
	paginaEscritaDinam* infoPag = existeElProcesoSwapDinam(pedido);

	if(infoPag != NULL){

		archivoParticion* particion = obtenerInfoParticion(infoPag->IdDeParticion);
		t_list_iterator* iterador = list_iterator_create(infoPag->listaDePagsOcupadas);

		while(list_iterator_has_next(iterador)){

			pagYLugar* pagAElim = list_iterator_next(iterador);
			eliminarPagina(particion, pagAElim->pagQueOcupaEnParticion);

			particion->cantDePaginasOcupadas--;
			bitarray_clean_bit(particion->bitmapDeFrames, pagAElim->pagQueOcupaEnParticion);

		}

		elimEstructAdminDePags(infoPag);
		reemplazarEstructAdminParticion(particion);
		list_iterator_destroy(iterador);

	}else{
		//ERROR
	}
}











paginaEscritaFija* existeElProcesoSwap(infoPeticion pedido){
	paginaEscritaFija* infoPag;

	bool existeYa(paginaEscritaFija* pag){
		return pag->proceso == pedido.proceso;
	}

	infoPag = list_find(pagsEnParticiones, (void*) existeYa);

	return infoPag;
}


paginaEscritaDinam* existeElProcesoSwapDinam(infoPeticion pedido){
	paginaEscritaDinam* infoPag;

	bool existeYaDinam(paginaEscritaDinam* pag){
		return pag->proceso == pedido.proceso;
	}

	infoPag = list_find(pagsEnParticiones, (void*) existeYaDinam);

	return infoPag;
}





archivoParticion* obtenerInfoParticion(int IdPart){
	archivoParticion* particion;

	bool informacionDeParticion(archivoParticion* info){
		return info->ID == IdPart;
	}

	particion = list_find(listaDeParticiones, (void*) informacionDeParticion);

	return particion;
}


archivoParticion* particionAEscribir(){

	bool ordenarListaSegunOcupado(archivoParticion* info, archivoParticion* info2){
		if(info->cantDePaginasOcupadas <= info2->cantDePaginasOcupadas){
			return true;
		}else if(info->cantDePaginasOcupadas > info2->cantDePaginasOcupadas){
			return false;
		}else{
			return true;
		}
	}

	archivoParticion* particion;
	t_list* copiaDeParticiones = list_sorted(listaDeParticiones, (void*) ordenarListaSegunOcupado);

	particion = list_get(copiaDeParticiones, 0);

	list_destroy(copiaDeParticiones);
	return particion;
}



int chequeoEspacioEnParticion(archivoParticion* particion){
	int pagsTotales = infoConfig.tamSwap/infoConfig.tamPagina;
	for(int i=0; i<=pagsTotales; i++){
		bool estaOcuopado = bitarray_test_bit(particion->bitmapDeFrames, i);

		if(estaOcuopado == 0){
			int contador = 1;
			bool hayLugar = 0;
			for(int j=i+1; (j<=pagsTotales && contador < infoConfig.marcosMaximos && hayLugar != 1); j++){
				hayLugar = bitarray_test_bit(particion->bitmapDeFrames, j);
				if(hayLugar == 0){
					contador++;
				}
			}
			if(contador == infoConfig.marcosMaximos){
				int inicioAOcupar = ocuparEnBitamp(particion, i+contador);
				return inicioAOcupar;
			}
		}

	}
	return -1;
}



int ocuparEnBitamp(archivoParticion* particion, int pos){
	int lugarAOcupar = pos - infoConfig.marcosMaximos;
	int retorno = lugarAOcupar;
	for(int i=0; i<infoConfig.marcosMaximos; i++){
		bitarray_set_bit(particion->bitmapDeFrames, lugarAOcupar);
		lugarAOcupar++;
	}

	return retorno;
}


void eliminarProcesoDeEstrAdmin(paginaEscritaFija* infoPag, archivoParticion* particion){

	particion->cantDePaginasOcupadas -= infoConfig.marcosMaximos;

	int max = infoPag->lugarPrimeraPag + infoConfig.marcosMaximos;
	for(int i=infoPag->lugarPrimeraPag; i<max; i++){
		bitarray_clean_bit(particion->bitmapDeFrames, i);
	}


	bool pagAElim(paginaEscritaFija* infoPag2){
		return infoPag2->proceso == infoPag->proceso;
	}

	list_remove_and_destroy_by_condition(pagsEnParticiones, (void*) pagAElim, free);

}


int proxPagLibre(archivoParticion* particion){

	int pagLibre = -1;
	bool test = 1;

	for(int i=0; i<maxCantDePaginasEnParticion && test == 1; i++){
		test = bitarray_test_bit(particion->bitmapDeFrames, i);
		if(test == 0){
			pagLibre = i;
			bitarray_set_bit(particion->bitmapDeFrames, i);
		}
	}

	return pagLibre;
}



int buscarPaginaEnDinam(paginaEscritaDinam* infoPag, int nroDePag){

	bool pagsIguales(pagYLugar* conjPags){
		return conjPags->nroPag == nroDePag;
	}

	pagYLugar* pagEncontrada = list_find(infoPag->listaDePagsOcupadas, (void*) pagsIguales);

	return pagEncontrada->pagQueOcupaEnParticion;
}



void eliminarPagina(archivoParticion* particion, int pagAElim){
	void* elim = particion->punteroAMemoria + pagAElim*infoConfig.tamPagina;
	char* reemplazo = string_repeat('\0', infoConfig.tamPagina);

	sem_wait(semArchivos);
	memcpy(elim, reemplazo, infoConfig.tamPagina);
	sem_post(semArchivos);

	free(reemplazo);
}

void elimEstructAdminDePags(paginaEscritaDinam* infoPag){

	list_destroy_and_destroy_elements(infoPag->listaDePagsOcupadas, free);

	bool pagAElimDinam(paginaEscritaDinam* infoPag2){
		return infoPag2->proceso == infoPag->proceso;
	}

	list_remove_and_destroy_by_condition(pagsEnParticiones, (void*) pagAElimDinam, free);


}


void reemplazarEstructAdminParticion(archivoParticion* particion){
	archivoParticion* partic = malloc(sizeof(archivoParticion));
	partic->ID = particion->ID;
	partic->bitmapDeFrames = particion->bitmapDeFrames;
	partic->cantDePaginasOcupadas = particion->cantDePaginasOcupadas;
	partic->punteroAMemoria = particion->punteroAMemoria;

	bool elimParticion (archivoParticion* part){
		return particion->ID == part->ID;
	}

	list_remove_and_destroy_by_condition(listaDeParticiones, (void*) elimParticion, free);

	list_add(listaDeParticiones, partic);
}



int yaExisteEstaPagina(paginaEscritaDinam* infoPag, infoPeticion pedido){

	bool estaPaginaExiste(pagYLugar* inf){
		return inf->nroPag == pedido.nroDePag;
	}

	pagYLugar* pagYLugarAReemplazar = list_find(infoPag->listaDePagsOcupadas, (void*) estaPaginaExiste);

	if(pagYLugarAReemplazar == NULL){
		return -1;
	}else {
		return pagYLugarAReemplazar->pagQueOcupaEnParticion;
	}

}
