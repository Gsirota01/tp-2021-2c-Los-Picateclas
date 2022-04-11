#include "Modulo_Memoria.h"

static void signalHandler(int sig)
{
	if(sig == SIGINT){
		fin = 1;
		//TODO
		//mostrar_paginas_presentes();

		while(numero_carpinchos_activos > 0){
			sem_wait(&carpinchos_activos);
		}
		metricas_SIGINT();
		liberar_memoria();
		exit(0);

	}
	if(sig == SIGUSR1){
		sigusr1 = 1;
	}
	if(sig == SIGUSR2){
		sigusr2 = 1;
	}
}

int main(void) {
	numero_carpinchos_activos = 0;
	fin = 0;
	sigusr1 = 0;
	sigusr2 = 0;
	primero = 0;
    if (signal(SIGINT, signalHandler) == SIG_ERR){
    	 printf("\ncan't catch\n");
    }
    if (signal(SIGUSR1, signalHandler) == SIG_ERR){
    	 printf("\ncan't catch\n");
    }
    if (signal(SIGUSR2, signalHandler) == SIG_ERR){
    	 printf("\ncan't catch\n");
    }
    reloj_posta = 0;
    idProceso = 1;
	printf("getpid %d",getpid());
	sem_init(&sem_fin,0,0);
	sem_init(&sem_fin_swamp,0,0);
	sem_init(&carpinchos_activos,0,0);
	esKernel = 1;


	setup_log_config();
	inicializar_memoria();
    reloj_global = malloc(sizeof(int));
    (*reloj_global) = 0;

	socket_swap = crearConexion(info.ip_swamp, info.puerto_swamp);


    int servidor = iniciarServidor(info.puerto);
	int cliente_fd = esperarCliente(servidor);

	while (!fin) {
		nuevo_hilo(cliente_fd);
		cliente_fd = esperarCliente(servidor);
	}



}

int setup_log_config(){
	//INICIALIZO EL LOG Y EL CONFIG
	logMemoria = log_create("memoria.log", "memoria", 1, LOG_LEVEL_INFO);
	log_info(logMemoria, "INICIANDO LOG memoria...");

	configMemoria = config_create("/home/utnso/tp-2021-2c-Los-Picateclas/EntregaFinal/ModuloMemoria/memoria.config");
	log_info(logMemoria, "CREANDO CONFIG memoria...");

	if(configMemoria == NULL){
		log_error(logMemoria, "NO SE PUDO CREAR EL CONFIG...");
		exit(-3);
	}
	info.ip = config_get_string_value(configMemoria, "IP");
	info.puerto = config_get_string_value(configMemoria, "PUERTO");
	info.ip_swamp = config_get_string_value(configMemoria, "IP_SWAMP");
	info.puerto_swamp = config_get_string_value(configMemoria, "PUERTO_SWAMP");
	info.tamMemoria = config_get_int_value(configMemoria, "TAMANIO");
	info.tamPagina = config_get_int_value(configMemoria, "TAMANIO_PAGINA");
	info.tipoAsignacion = config_get_string_value(configMemoria, "TIPO_ASIGNACION");
  	info.algoReemplazoMMU = config_get_string_value(configMemoria, "ALGORITMO_REEMPLAZO_MMU");
  	info.marcosMaximos = config_get_int_value(configMemoria, "MARCOS_MAXIMOS");
  	info.entradasTLB = config_get_int_value(configMemoria, "CANTIDAD_ENTRADAS_TLB");
	info.algoReemplzoTLB = config_get_string_value(configMemoria, "ALGORITMO_REEMPLAZO_TLB");
	info.retardoHit = config_get_int_value(configMemoria, "RETARDO_ACIERTO_TLB");
	info.retardoMiss = config_get_int_value(configMemoria, "RETARDO_FALLO_TLB");
	info.path = config_get_string_value(configMemoria, "PATH_DUMP_TLB");
	info.mostrarMemoria = config_get_int_value(configMemoria, "MOSTRAR_MEMORIA");
	info.mostrarTLB = config_get_int_value(configMemoria, "MOSTRAR_TLB");

	return 1;
}


void inicializar_memoria(){
    tamanio_memoria = info.tamMemoria;
    tamanio_frame = info.tamPagina;
    memoria = calloc(1,tamanio_memoria);
    int frames_totales = tamanio_memoria/tamanio_frame;
    lista_frames = list_create();
	t_frame *frame;
	int offset = 0;
    for(int i=0; i<frames_totales; i++){
        frame = malloc(sizeof(t_frame));
        frame->ptr_inicio = memoria + offset;
        frame->vacio = 1;
        frame->nro_frame = i;
        list_add(lista_frames, frame);
        offset += tamanio_frame;
    }
    lista_paginas_global = list_create();
    lista_procesos = list_create();
    lista_entradas_TLB = list_create();
    pila_LRU_TLB = list_create();
    cola_pedidos = queue_create();
    if(!strcmp(info.tipoAsignacion,"FIJA")){
    	limite_frames = info.marcosMaximos;
    }
    else{
    	limite_frames = list_size(lista_frames);
    }

    pthread_mutex_init(&mutex_cola_pedidos,NULL);
    pthread_mutex_init(&mutex_lista_procesos,NULL);
    null_alloc = 1;
}

void liberar_memoria(){
	int i = 0;
	int tamanio = list_size(lista_procesos);
	while(tamanio > i){
		t_info_proceso *info_proceso = list_get(lista_procesos,i);
		if(info_proceso->lista_datos_alloc != NULL){
			eliminar_proceso(info_proceso);
		}
		free(info_proceso);
		i++;
	}
	sem_post(&sem_fin_swamp);
    free(reloj_global);
    free(memoria);
    i = 0;
    tamanio = list_size(lista_frames);
	while(tamanio > i){
		free(list_remove(lista_frames,0));
		i++;
	}
	list_destroy(lista_frames);
	list_destroy(lista_paginas_global);
	list_destroy(lista_procesos);
	list_destroy(lista_entradas_TLB);
	list_destroy(pila_LRU_TLB);
	queue_destroy(cola_pedidos);
	log_destroy(logMemoria);
	config_destroy(configMemoria);
    pthread_mutex_destroy(&mutex_cola_pedidos);
    pthread_mutex_destroy(&mutex_lista_procesos);
	close(socket_swap);
}

void cargarNuevoProceso(mate_instanceACambiar* proceso){

	//pthread_mutex_lock(&mutex_lista_procesos);
	if(proceso->id == -1){
		proceso->id = idProceso++;
	}
	t_info_proceso *nuevo_info_proceso = malloc(sizeof(t_info_proceso));
	list_add(lista_procesos,nuevo_info_proceso);
	nuevo_info_proceso->pid = proceso->id;
	nuevo_info_proceso->metricas.tlb_hit = 0;
	nuevo_info_proceso->metricas.tlb_miss = 0;
	nuevo_info_proceso->tabla_de_paginas = list_create();
	nuevo_info_proceso->lista_datos_alloc = list_create();
	if(!strcmp(info.tipoAsignacion,"FIJA")){
		nuevo_info_proceso->reloj = malloc(sizeof(int));
		*(nuevo_info_proceso->reloj) = 0;
		nuevo_info_proceso->lista_paginas_presentes = list_create();
	}
	else{
		nuevo_info_proceso->lista_paginas_presentes = lista_paginas_global;
		nuevo_info_proceso->reloj = reloj_global;
	}
}

void procesar_peticion(arg_struct* argumentos){

	int pid;
	t_info_proceso *info_proceso;
	bool condicion_mismo_pid(void *arg){
		t_info_proceso *un_info_proceso = (t_info_proceso *)arg;
		return(un_info_proceso->pid == pid);
	}

	int respuesta = 0;
	int desplazamiento = 0;

	int bytesARecibir = 0;
	recv(argumentos->conexion, &bytesARecibir, sizeof(int), MSG_WAITALL);
	void* buffer = malloc(bytesARecibir);
	recv(argumentos->conexion, buffer, bytesARecibir, 0);

	tipoDePeticionMemoria peticion;
	memcpy(&peticion, buffer, sizeof(tipoDePeticionMemoria));
	desplazamiento += sizeof(tipoDePeticionMemoria);

	mate_instanceACambiar* carpincho = desserializarCarpincho(buffer);
	int bytesCarpincho = sizeDeCarpincho(carpincho);
	desplazamiento += bytesCarpincho;

	pid = carpincho->id;

	pthread_mutex_lock(&mutex_cola_pedidos);

	if(peticion == NUEVOPROCESO){

		if(carpincho->id == -1){
			esKernel = 0;
			cargarNuevoProceso(carpincho);
			log_info(logMemoria, "-----INICIO PETICION PROCESO %d-----", carpincho->id);
			log_info(logMemoria, "Proceso %d inicia", carpincho->id);
			void* bufferSend = serializarCarpincho(carpincho);
			int cantBytes = sizeDeCarpincho(carpincho);
			send(argumentos->conexion, &cantBytes, sizeof(int), 0);
			send(argumentos->conexion, bufferSend, cantBytes, 0);
			free(bufferSend);
		}
		else{
			cargarNuevoProceso(carpincho);
			log_info(logMemoria, "-----INICIO PETICION PROCESO %d-----", carpincho->id);
			log_info(logMemoria, "Proceso %d inicia", carpincho->id);
			close(argumentos->conexion);
		}

	}else{
		info_proceso = list_find(lista_procesos,&condicion_mismo_pid);

		log_info(logMemoria, "-----INICIO PETICION PROCESO %d-----", carpincho->id);



		if (peticion == PEDIRMEMORIA) {
			int tamanio_memalloc;
			memcpy(&tamanio_memalloc,buffer+desplazamiento,sizeof(int));

			log_info(logMemoria,"Proceso %d Memalloc de tamanio:%d",info_proceso->pid,tamanio_memalloc);


			respuesta = memalloc(tamanio_memalloc,info_proceso);

			send(argumentos->conexion, &respuesta, sizeof(int), 0);

			}else if (peticion == LEERMEMORIA) {
				int direccion_logica;
				memcpy(&direccion_logica,buffer+desplazamiento,sizeof(int));
				desplazamiento += sizeof(int);

				int tamanio_a_leer;
				memcpy(&tamanio_a_leer,buffer+desplazamiento,sizeof(int));
				log_info(logMemoria,"Proceso %d Memread en direccion:%d y tamanio:%d",info_proceso->pid,direccion_logica,tamanio_a_leer);


				bool condicion_data_alloc(void *arg){
					t_dato_alloc *data_alloc = (t_dato_alloc *)arg;
					return(data_alloc->direccion == direccion_logica && data_alloc->tamanio >= tamanio_a_leer);
				}
				if(list_any_satisfy(info_proceso->lista_datos_alloc,&condicion_data_alloc)){
					respuesta = 1;
					t_memread *paquete_leido = memread(info_proceso,direccion_logica,tamanio_a_leer);
					printf("SE LEYO: %s con TAM: %d \n", paquete_leido->leido, paquete_leido->tamanio);
					send(argumentos->conexion,&respuesta,sizeof(int),0);
					send(argumentos->conexion,paquete_leido->leido,paquete_leido->tamanio,0);
					free(paquete_leido->leido);
					free(paquete_leido);
				}
				else{
					respuesta = MATE_READ_FAULT;
					void* vacio = malloc(tamanio_a_leer);
					send(argumentos->conexion,&respuesta,sizeof(int),0);
					send(argumentos->conexion,vacio,tamanio_a_leer,0);
					free(vacio);
				}


			}else if (peticion == ESCRIBIRMEMORIA) {
				int direccion_logica;
				memcpy(&direccion_logica,buffer+desplazamiento,sizeof(int));
				desplazamiento += sizeof(int);

				int tamanio_a_escribir;
				memcpy(&tamanio_a_escribir,buffer+desplazamiento,sizeof(int));
				desplazamiento += sizeof(int);

				void *a_escribir = malloc(tamanio_a_escribir);
				memcpy(a_escribir,buffer+desplazamiento,tamanio_a_escribir);
				log_info(logMemoria,"Proceso %d Memwrite en direccion:%d y tamanio:%d",info_proceso->pid,direccion_logica,tamanio_a_escribir);
				printf("VOY A ESCRIBIR: %s y TAM: %d \n", a_escribir, tamanio_a_escribir);
				respuesta = memwrite(info_proceso,direccion_logica,a_escribir,tamanio_a_escribir);
				send(argumentos->conexion, &respuesta, sizeof(int), 0);

				free(a_escribir);
			}else if (peticion == LIBERARMEMORIA) {
				int direccion_logica;
				memcpy(&direccion_logica,buffer+desplazamiento,sizeof(int));

				log_info(logMemoria,"Proceso %d Memfree en direccion:%d",info_proceso->pid,direccion_logica);
				respuesta = memfree(info_proceso,direccion_logica);

				send(argumentos->conexion, &respuesta, sizeof(int), 0);
			}else if (peticion == LIBERARPROCESO) {

				log_info(logMemoria, "Proceso %d finaliza", carpincho->id);

				eliminar_proceso(info_proceso);

				if(esKernel == 1){
					close(argumentos->conexion);
				}
			}else if (peticion == PROCESOSUSPENDIDO) {

				log_info(logMemoria, "Proceso %d se suspende", carpincho->id);

				suspender_proceso(info_proceso);

				if(esKernel == 1){
					close(argumentos->conexion);
				}
			}

	}
	if(info.mostrarMemoria && !strcmp(info.tipoAsignacion,"DINAMICA")){
		mostrar_paginas_presentes();
	}


	log_info(logMemoria, "-----FIN PETICION PROCESO %d-----", carpincho->id);


	pthread_mutex_unlock(&mutex_cola_pedidos);

	liberarCarpincho(carpincho);
	if(esKernel == 0){
		close(argumentos->conexion);
	}
	free(argumentos);
	free(buffer);
	pthread_mutex_lock(&mutex_lista_procesos);
	numero_carpinchos_activos--;
	pthread_mutex_unlock(&mutex_lista_procesos);
	sem_post(&carpinchos_activos);
}

//Adapta la funcion leer() para leer el HeapMetadata de una direccion logica
//(llamo alloc a los HeapMetadata, lo tendria que cambiar xd)
t_alloc *obtener_alloc(t_info_proceso *info_proceso, int dir_logica){
	void *data = leer(info_proceso,dir_logica - 9,9);
	t_alloc *alloc = malloc(sizeof(t_alloc));
	memcpy(&(alloc->prev_alloc), data, 4);
	memcpy(&(alloc->next_alloc), data + 4, 4);
	memcpy(&(alloc->is_free), data + 8, 1);
	free(data);
	log_info(logMemoria, "Se lee hmd prev:%d next:%d free:%d en direccion %d",alloc->prev_alloc,alloc->next_alloc,alloc->is_free,dir_logica-9);
	return alloc;

}

// Calcula a partir del HeapMetadata el tamanio a leer y lo lee
t_memread* memread(t_info_proceso *info_proceso, int dir_logica, int tamanio){
	/*t_alloc *alloc = obtener_alloc(info_proceso,dir_logica);
	int tamanio;
	tamanio = alloc->next_alloc - dir_logica;
	free(alloc);*/
	bool condicion_data_alloc(void *arg){
		t_dato_alloc *data_alloc = (t_dato_alloc *)arg;
		return(data_alloc->direccion == dir_logica && data_alloc->tamanio >= tamanio);
	}
	t_memread *datos_memread = malloc(sizeof(t_memread));
	datos_memread->leido = leer(info_proceso,dir_logica,tamanio);
	datos_memread->tamanio = tamanio;
	return datos_memread;
}

// Calcula a partir del HeapMetadata el tamanio del alloc y escribe
// Hay que tener en cuenta que la dir. logica siempre referencia a un alloc
//y por lo tanto siempre sabemos el tamanio de lo que vamos a escribir.
int memwrite(t_info_proceso *info_proceso, int dir_logica, void *data, int tamanio){
	bool condicion_data_alloc(void *arg){
		t_dato_alloc *data_alloc = (t_dato_alloc *)arg;
		return(data_alloc->direccion == dir_logica && data_alloc->tamanio >= tamanio);
	}
	if(list_any_satisfy(info_proceso->lista_datos_alloc,&condicion_data_alloc)){
		escribir(info_proceso,dir_logica,tamanio,data);
		return 1;
	}
	else{
		return MATE_WRITE_FAULT;
	}
}

//	Incluye toda la logica de obtener un frame
//	Checkea TLB o busca en tabla, reemplaza pagina si es necesario y
// 	actualiza algoritmos de reemplazo y TLB
t_frame *obtener_frame(t_info_proceso *info_proceso, int nro_pag, bool es_escritura){
	t_frame *frame;
	t_entrada_pagina *entrada_pagina;
	if(info.entradasTLB > 0){
		entrada_pagina = buscar_en_TLB(info_proceso,nro_pag);
	}
	else{
		log_info(logMemoria,"TLB MISS - Proceso:%d Pagina:%d",info_proceso->pid,nro_pag);
		entrada_pagina = NULL;
	}
	if(entrada_pagina == NULL){
		(info_proceso->metricas.tlb_miss)++;
		usleep(info.retardoMiss*1000);
		entrada_pagina = list_get(info_proceso->tabla_de_paginas,nro_pag);
	}
	else{
		log_info(logMemoria,"TLB HIT - Proceso:%d Pagina: %d Marco:%d",info_proceso->pid,nro_pag,entrada_pagina->nro_frame);
		(info_proceso->metricas.tlb_hit)++;
		usleep(info.retardoHit*1000);
	}
	if(!entrada_pagina->presencia){
		frame = reemplazar_pagina(info_proceso, entrada_pagina,info_proceso->lista_paginas_presentes,NULL);
	}
	else{
		int nro_frame = entrada_pagina->nro_frame;
		frame = list_get(lista_frames,nro_frame);
	}
	actualizar_acceso_MMU(info_proceso, entrada_pagina, es_escritura);

	if(info.mostrarTLB){
		mostrar_tlb();
	}
	return frame;
}


void actualizar_acceso_MMU(t_info_proceso *info_proceso,t_entrada_pagina *entrada_pagina, bool es_escritura){
	if(!strcmp(info.algoReemplazoMMU,"CLOCK-M")){
		actualizar_clock(info_proceso->lista_paginas_presentes, entrada_pagina, info_proceso->reloj);
	}
	else{
		actualizar_LRU(info_proceso->lista_paginas_presentes, entrada_pagina);
	}
	entrada_pagina->uso = 1;
	if(es_escritura){
		entrada_pagina->modificado = 1;
	}
}

//Busca en TLB y reemplaza entrada si es necesario
t_entrada_pagina *buscar_en_TLB(t_info_proceso *info_proceso, int nro_pag){
	t_entrada_pagina *entrada_tlb;
	bool condicion_esta_en_tlb(void *arg){
		t_entrada_pagina *una_entrada_tlb = (t_entrada_pagina *)arg;
		return(una_entrada_tlb->pid == info_proceso->pid && una_entrada_tlb->nro_pag == nro_pag);
	}
	entrada_tlb = list_find(lista_entradas_TLB, &condicion_esta_en_tlb);
	if(entrada_tlb == NULL){
		log_info(logMemoria,"TLB MISS - Proceso:%d Pagina:%d",info_proceso->pid,nro_pag);
		t_entrada_pagina *victima;
		if(list_size(lista_entradas_TLB) == info.entradasTLB){
			if(!strcmp(info.algoReemplzoTLB,"FIFO")){
				victima = list_remove(lista_entradas_TLB,0);
			}
			else{
				victima = list_remove(pila_LRU_TLB, 0);
			}

			//free(victima);
			t_entrada_pagina *entrada_pagina = list_get(info_proceso->tabla_de_paginas,nro_pag);
			log_info(logMemoria,"Entrada TLB Victima proceso:%d pagina:%d - Nueva entrada TLB proceso:%d pagina:%d",
					victima->pid,victima->nro_pag,entrada_pagina->pid,entrada_pagina->nro_pag);
		}
		t_entrada_pagina *entrada_pagina = list_get(info_proceso->tabla_de_paginas,nro_pag);
		if(list_size(lista_entradas_TLB) == info.entradasTLB && entrada_tlb == NULL && !strcmp(info.algoReemplzoTLB,"LRU")) {
			int i =0;
			t_entrada_pagina *entrada_actual = list_get(lista_entradas_TLB,0);
			while(victima != entrada_actual){
				i++;
				entrada_actual = list_get(lista_entradas_TLB,i);
			}
			list_replace(lista_entradas_TLB,i,entrada_pagina);
		}
		else{
			list_add(lista_entradas_TLB,entrada_pagina);
		}

		if(!strcmp(info.algoReemplzoTLB,"LRU")){
			actualizar_LRU(pila_LRU_TLB,entrada_pagina);
		}
		return NULL;
	}
	else{
		if(!strcmp(info.algoReemplzoTLB,"LRU")){
			actualizar_LRU(pila_LRU_TLB,entrada_tlb);
		}
		return entrada_tlb;
	}
}

//Escribe la pagina no presente en el frame
//Para eso puede tener que hacer un reemplazo y guardar a swap
//o escribirla directamente si es espacio.
//En ambos casos las paginas no presentes siempre se sacan de swap
t_frame *reemplazar_pagina(t_info_proceso *info_proceso, t_entrada_pagina *entrada_pagina_no_presente, t_list *lista_paginas_presentes, void *pagina_a_escribir){
	t_entrada_pagina *pagina_victima;
	bool condicion_igual_pagina(void *arg){
		t_entrada_pagina *una_pagina = (t_entrada_pagina *)arg;
		return (una_pagina == pagina_victima);
	}
	void *pagina;
	t_frame *frame_libre = buscar_frame_libre();
	if((frame_libre == NULL && list_size(lista_paginas_presentes) >= limite_frames )||
			(frame_libre != NULL && list_size(lista_paginas_presentes) >= limite_frames)){
		pagina_victima = elegir_victima_MMU(info_proceso,entrada_pagina_no_presente);
		log_info(logMemoria,"Victima proceso:%d pagina:%d - Nueva proceso:%d pagina:%d",pagina_victima->pid,pagina_victima->nro_pag,entrada_pagina_no_presente->pid,entrada_pagina_no_presente->nro_pag);
		t_frame *frame = list_get(lista_frames,pagina_victima->nro_frame);
		frame_libre = frame;
		list_remove_by_condition(lista_paginas_presentes,&condicion_igual_pagina);
		list_remove_by_condition(lista_entradas_TLB,&condicion_igual_pagina);
		pagina_victima->presencia = 0;
		pagina_victima->uso = 1;
		if(pagina_victima->modificado){
			pagina_victima->modificado = 0;
			pagina = calloc(1,tamanio_frame);
			memcpy(pagina,frame->ptr_inicio,tamanio_frame);
			escribir_swap(info_proceso,pagina_victima,pagina);
			free(pagina);
		}
	}
	if(pagina_a_escribir == NULL){
		pagina = leer_swap(info_proceso,entrada_pagina_no_presente);
		memcpy(frame_libre->ptr_inicio,pagina,tamanio_frame);
		free(pagina);
	}
	else{
		pagina = pagina_a_escribir;
		memcpy(frame_libre->ptr_inicio,pagina,tamanio_frame);
	}

	frame_libre->vacio = 0;
	entrada_pagina_no_presente->presencia = 1;
	entrada_pagina_no_presente->nro_frame = frame_libre->nro_frame;
	entrada_pagina_no_presente->modificado = 0;
	//list_add(lista_paginas_presentes,entrada_pagina_no_presente);
	return frame_libre;
}

t_frame *buscar_frame_libre(){
	t_frame *frame;
	bool condicion_frame_libre(void *arg){
		t_frame *frame = (t_frame *)arg;
		return frame->vacio;
	}
	frame = list_find(lista_frames,&condicion_frame_libre);
	return frame;
}


t_entrada_pagina *elegir_victima_MMU(t_info_proceso *info_proceso,t_entrada_pagina *entrada_pagina_no_presente){
	if(!strcmp(info.algoReemplazoMMU,"CLOCK-M")){
		int *reloj = info_proceso->reloj;
		while(1){
			int tamanio_lista_paginas_presente;
			t_list *lista_paginas_presente = info_proceso->lista_paginas_presentes;
			tamanio_lista_paginas_presente = list_size(lista_paginas_presente);
			int i = 0;
			t_entrada_pagina *entrada_pagina;
			do {
				entrada_pagina = list_get(lista_paginas_presente,(*reloj));
				i++;
				(*reloj)++;
				if((*reloj) == tamanio_lista_paginas_presente){
					(*reloj) = 0;
				}
			}
			while(i < tamanio_lista_paginas_presente && (entrada_pagina->uso != 0 || entrada_pagina->modificado != 0));
			if(entrada_pagina->uso == 0 && entrada_pagina->modificado == 0){
				if((*reloj) == 0){
					list_replace(info_proceso->lista_paginas_presentes,tamanio_lista_paginas_presente-1,entrada_pagina_no_presente);
				}
				else{
					list_replace(info_proceso->lista_paginas_presentes,(*reloj) - 1,entrada_pagina_no_presente);
				}
				return entrada_pagina;
			}
			i = 0;
			entrada_pagina = NULL;
			do{
				if(entrada_pagina != NULL){
					entrada_pagina->uso = 0;
				}
				entrada_pagina = list_get(lista_paginas_presente,(*reloj));
				i++;
				(*reloj)++;
				if((*reloj) == tamanio_lista_paginas_presente){
					(*reloj) = 0;
				}
			}
			while(i < tamanio_lista_paginas_presente && (entrada_pagina->uso != 0 || entrada_pagina->modificado != 1));
			if(entrada_pagina->uso == 0 && entrada_pagina->modificado == 1){
				if((*reloj) == 0){
					list_replace(info_proceso->lista_paginas_presentes,tamanio_lista_paginas_presente-1,entrada_pagina_no_presente);
				}
				else{
					list_replace(info_proceso->lista_paginas_presentes,(*reloj) - 1,entrada_pagina_no_presente);
				}
				return entrada_pagina;
			}
			entrada_pagina->uso = 0;
		}
	}
	else{
		return list_get(info_proceso->lista_paginas_presentes,0);
	}
}



void *leer(t_info_proceso *info_proceso, double dir_logica, int tamanio){
	t_frame *frame;
	int tamanio_a_leer;
	int tamanio_restante = tamanio;
	int cantidad_leida = 0;
	double nro_pag = dir_logica/tamanio_frame;
	nro_pag = floor(nro_pag);
	int offset_en_pagina = dir_logica - nro_pag*tamanio_frame;
	int espacio_restante_en_pagina;
	void *data = malloc(tamanio);
	while(cantidad_leida < tamanio){
		espacio_restante_en_pagina = tamanio_frame - offset_en_pagina;
		if(espacio_restante_en_pagina >= tamanio_restante){
			tamanio_a_leer = tamanio_restante;
		}
		else{
			tamanio_a_leer = espacio_restante_en_pagina;
		}
		frame = obtener_frame(info_proceso, nro_pag,0);
		memcpy(data + cantidad_leida,frame->ptr_inicio + offset_en_pagina,tamanio_a_leer);
		offset_en_pagina = 0;
		cantidad_leida += tamanio_a_leer;
		tamanio_restante = tamanio - cantidad_leida;
		nro_pag++;
	}
	return data;
}

void *escribir(t_info_proceso *info_proceso, double dir_logica, int tamanio, void *data){
	t_frame *frame;
	int tamanio_a_escribir;
	int cantidad_escrita = 0;
	int tamanio_restante = tamanio;
	double nro_pag = dir_logica/tamanio_frame;
	nro_pag = floor(nro_pag);
	int offset_en_pagina = dir_logica - nro_pag*tamanio_frame;
	int espacio_restante_en_pagina;
	while(cantidad_escrita < tamanio){
		espacio_restante_en_pagina = tamanio_frame - offset_en_pagina;
		if(espacio_restante_en_pagina >= tamanio_restante){
			tamanio_a_escribir = tamanio_restante;
		}
		else{
			tamanio_a_escribir = espacio_restante_en_pagina;
		}
		frame = obtener_frame(info_proceso, nro_pag,1);
		memcpy(frame->ptr_inicio + offset_en_pagina, data + cantidad_escrita, tamanio_a_escribir);
		offset_en_pagina = 0;
		cantidad_escrita += tamanio_a_escribir;
		tamanio_restante = tamanio - cantidad_escrita;
		nro_pag++;
	}
	return data;
}


void actualizar_LRU(t_list *lista, t_entrada_pagina *entrada){
	bool condicion_igual_entrada(void *arg){
		t_entrada_pagina *otra_entrada = (t_entrada_pagina *)arg;
		return (otra_entrada == entrada);
	}
	if(list_find(lista,&condicion_igual_entrada) != NULL){
		list_remove_by_condition(lista,&condicion_igual_entrada);
	}
	list_add(lista,entrada);

}

void actualizar_clock(t_list *lista_paginas_presentes, t_entrada_pagina *pagina, int *reloj){
	int i = 0;
	t_entrada_pagina *pagina_actual = NULL;
	while(i < list_size(lista_paginas_presentes) && pagina_actual != pagina){
		pagina_actual = list_get(lista_paginas_presentes, i);
		i++;
	}
	if(pagina_actual != pagina){
		list_add(lista_paginas_presentes,pagina);
		i++;
	}
}


void *leer_swap(t_info_proceso *info_proceso, t_entrada_pagina *entrada_no_presente){
	info_proceso->info_peticion.pedido = LECTURA;
	info_proceso->info_peticion.nroDePag = entrada_no_presente->nro_pag;
	info_proceso->info_peticion.proceso = info_proceso->pid;
	void* pagina = lectura(info_proceso->info_peticion);
	return pagina;
}
void escribir_swap(t_info_proceso *info_proceso,t_entrada_pagina *entrada_pagina, void *pagina){
	info_proceso->info_peticion.pedido = ESCRITURA;
	info_proceso->info_peticion.pagina = pagina;
	info_proceso->info_peticion.nroDePag = entrada_pagina->nro_pag;
	info_proceso->info_peticion.proceso = entrada_pagina->pid;
	escritura(info_proceso->info_peticion,pagina);
}

void eliminar_de_swap(t_info_proceso *info_proceso){
	info_proceso->info_peticion.pedido = ELIMINADO;
	info_proceso->info_peticion.pagina = NULL;
	info_proceso->info_peticion.nroDePag = 0;
	info_proceso->info_peticion.proceso = info_proceso->pid;
	eliminado(info_proceso->info_peticion);
}

int crear_paginas(int size, t_info_proceso *info_proceso){
	int offset = 0;
	t_entrada_pagina *entrada_pagina;
	while(offset < size){
		entrada_pagina = malloc(sizeof(t_entrada_pagina));
			entrada_pagina->pid = info_proceso->pid;
			entrada_pagina->presencia = 0;
			entrada_pagina->nro_frame = -1;
			entrada_pagina->uso = 1;
			entrada_pagina->modificado = 0;
			list_add(info_proceso->tabla_de_paginas,entrada_pagina);
			entrada_pagina->nro_pag = list_size(info_proceso->tabla_de_paginas) - 1;
			void *pagina = calloc(1,tamanio_frame);
			escribir_swap(info_proceso, entrada_pagina,pagina);
			free(pagina);
			offset += tamanio_frame;
	}
	return 1;

}
int memalloc(int tamanio, t_info_proceso *info_proceso){
	void escribir_hmd(t_alloc *alloc, int dir_log){
		void *data = malloc(9);
		memcpy(data, &(alloc->prev_alloc),4);
		memcpy(data + 4, &(alloc->next_alloc),4);
		memcpy(data + 8,&(alloc->is_free),1);
		escribir(info_proceso,dir_log,9,data);
		log_info(logMemoria, "Se escribe hmd prev:%d next:%d free:%d en direccion %d",alloc->prev_alloc,alloc->next_alloc,alloc->is_free,dir_log);
		free(data);
	}
	void agrega_a_lista_data(int direccion){
		t_dato_alloc *data_alloc = malloc(sizeof(t_dato_alloc));
		data_alloc->direccion = direccion;
		data_alloc->tamanio = tamanio;
		list_add(info_proceso->lista_datos_alloc,data_alloc);
	}

	if(list_size(info_proceso->tabla_de_paginas) == 0){
		crear_paginas(tamanio + 18,info_proceso);
		t_alloc *primer_hmd = malloc(sizeof(t_alloc));
		primer_hmd->prev_alloc = null_alloc;
		primer_hmd->next_alloc = tamanio + 9;
		primer_hmd->is_free = 0;
		escribir_hmd(primer_hmd,0);

		t_alloc *segundo_hmd = malloc(sizeof(t_alloc));
		segundo_hmd->prev_alloc = 0;
		segundo_hmd->next_alloc = null_alloc;
		segundo_hmd->is_free = 1;
		escribir_hmd(segundo_hmd,primer_hmd->next_alloc);
		free(primer_hmd);
		free(segundo_hmd);

		agrega_a_lista_data(9);
		return 9;
	}
	else{
		t_alloc *alloc;
		t_alloc *prev_alloc = NULL;
		int tamanio_alloc;
		int dir_log;
		alloc = obtener_alloc(info_proceso,9);
		while(alloc->next_alloc != null_alloc){
			if(alloc->is_free){
				if(alloc->prev_alloc == null_alloc){
					tamanio_alloc = alloc->next_alloc - 9;
					dir_log = 0;
				}
				else{
					tamanio_alloc = (prev_alloc->next_alloc + 9) - (alloc->next_alloc);
					dir_log = prev_alloc->next_alloc;
				}
				if(tamanio_alloc == tamanio){
					alloc->is_free = 0;
					escribir_hmd(alloc,dir_log);
					agrega_a_lista_data(dir_log + 9);
					if(prev_alloc != NULL){
						free(prev_alloc);
					}
					free(alloc);
					return dir_log + 9;
				}
				else{
					if((tamanio_alloc - tamanio) >= 9 ){
						t_alloc *nuevo_HMD = malloc(sizeof(t_alloc));
						nuevo_HMD->prev_alloc = dir_log;
						nuevo_HMD->next_alloc = alloc->next_alloc;
						nuevo_HMD->is_free = 1;
						escribir_hmd(nuevo_HMD,dir_log + 9 + tamanio);
						alloc->next_alloc = dir_log + 9 + tamanio;
						alloc->is_free = 0;
						escribir_hmd(alloc,dir_log);
						t_alloc *next_alloc = obtener_alloc(info_proceso,nuevo_HMD->next_alloc + 9);
						next_alloc->prev_alloc = dir_log + 9 + tamanio;
						escribir_hmd(next_alloc,nuevo_HMD->next_alloc);
						free(next_alloc);
						free(nuevo_HMD);
						agrega_a_lista_data(dir_log + 9);
						if(prev_alloc != NULL){
							free(prev_alloc);
						}
						free(alloc);
						return dir_log + 9;
					}
					else{
						tamanio_alloc = 0;
					}
				}
			}
			int next_dir_log = alloc->next_alloc;
			if(prev_alloc != NULL){
				free(prev_alloc);
				prev_alloc = NULL;
			}
			prev_alloc = alloc;
			alloc = obtener_alloc(info_proceso,next_dir_log + 9);
		}
		int nro_pag;
		int prev_alloc_next_alloc;
		if(alloc->prev_alloc == null_alloc){
			prev_alloc_next_alloc = 0;
		}
		else{
			//prev_alloc = obtener_alloc(info_proceso,alloc->prev_alloc + 9);
			prev_alloc_next_alloc = prev_alloc->next_alloc;
			if(prev_alloc != NULL){
				free(prev_alloc);
			}
		}
		t_alloc *siguiente_hmd = malloc(sizeof(t_alloc));
		alloc->next_alloc = prev_alloc_next_alloc + 9 + tamanio;
		alloc->is_free = 0;
		escribir_hmd(alloc,prev_alloc_next_alloc);
		siguiente_hmd->prev_alloc = prev_alloc_next_alloc;
		siguiente_hmd->next_alloc = null_alloc;
		siguiente_hmd->is_free = 1;
		nro_pag = ((prev_alloc_next_alloc + 9)/tamanio_frame) + 1;
		tamanio_alloc = nro_pag*tamanio_frame - (prev_alloc_next_alloc + 9);
		if((nro_pag-1)*tamanio_frame == prev_alloc_next_alloc + 9){
			tamanio_alloc = 0;
		}
		crear_paginas(tamanio + 9 - tamanio_alloc,info_proceso);
		escribir_hmd(siguiente_hmd,prev_alloc_next_alloc + 9 + tamanio);
		free(siguiente_hmd);

		free(alloc);
		agrega_a_lista_data(prev_alloc_next_alloc + 9);
		return prev_alloc_next_alloc + 9;

	}
}
int memfree(t_info_proceso *info_proceso, int dir_log){
	bool condicion_data_alloc(void *arg){
		t_dato_alloc *data_alloc = (t_dato_alloc *)arg;
		return(data_alloc->direccion == dir_log);
	}
	if(!list_any_satisfy(info_proceso->lista_datos_alloc,&condicion_data_alloc)){
		return MATE_FREE_FAULT;
	}

	void escribir_hmd(t_alloc *alloc, int dir_log){
		void *data = malloc(9);
		memcpy(data, &(alloc->prev_alloc),4);
		memcpy(data + 4, &(alloc->next_alloc),4);
		memcpy(data + 8,&(alloc->is_free),1);
		escribir(info_proceso,dir_log,9,data);
		log_info(logMemoria, "Se escribe hmd prev:%d next:%d free:%d en direccion %d",alloc->prev_alloc,alloc->next_alloc,alloc->is_free,dir_log);
		free(data);
	}

	t_alloc *alloc_a_borrar = obtener_alloc(info_proceso,dir_log);
	alloc_a_borrar->is_free = 1;
	int dir_logica_a_borrar = dir_log - 9;
	if(alloc_a_borrar->prev_alloc == null_alloc){
		alloc_a_borrar->is_free = 1;
		dir_logica_a_borrar = 0;
	}
	else{
		t_alloc *prev_alloc = obtener_alloc(info_proceso,alloc_a_borrar->prev_alloc + 9);
		if(prev_alloc->is_free){
			dir_logica_a_borrar = alloc_a_borrar->prev_alloc;
			alloc_a_borrar->prev_alloc = prev_alloc->prev_alloc;
		}
		free(prev_alloc);
	}
	t_alloc *next_alloc = obtener_alloc(info_proceso,alloc_a_borrar->next_alloc + 9);
	if(next_alloc->is_free){
		alloc_a_borrar->next_alloc = next_alloc->next_alloc;
		if(next_alloc->next_alloc == null_alloc){
			int nro_pagina = (dir_log/tamanio_frame) + 1;
			int tamanio_tabla_paginas = list_size(info_proceso->tabla_de_paginas);
			t_entrada_pagina *pagina;
			while(tamanio_tabla_paginas > nro_pagina){
				pagina = list_remove(info_proceso->tabla_de_paginas,tamanio_tabla_paginas-1);
				bool condicion_igual_entrada(void *arg){
					t_entrada_pagina *otra_entrada = (t_entrada_pagina *)arg;
					return (otra_entrada == pagina);
				}
				if(list_find(info_proceso->lista_paginas_presentes,&condicion_igual_entrada) != NULL){
					list_remove_by_condition(info_proceso->lista_paginas_presentes,&condicion_igual_entrada);
				}
				if(list_find(lista_entradas_TLB,&condicion_igual_entrada) != NULL){
					list_remove_by_condition(lista_entradas_TLB,&condicion_igual_entrada);
					list_remove_by_condition(pila_LRU_TLB,&condicion_igual_entrada);

				}
				if(pagina->presencia){
					t_frame *frame = list_get(lista_frames,pagina->nro_frame);
					frame->vacio = 1;

				}
				free(pagina);


				tamanio_tabla_paginas--;
			}
		}
	}

	escribir_hmd(alloc_a_borrar,dir_logica_a_borrar);
	free(alloc_a_borrar);
	free(next_alloc);

	return 1;

}

void eliminar_proceso(t_info_proceso *info_proceso){

	if(!list_is_empty(info_proceso->tabla_de_paginas)){
		eliminar_de_swap(info_proceso);
	}


	t_list *paginas_presentes = list_create();
	bool condicion_proceso(void *arg){
		t_entrada_pagina *una_entrada_tlb = (t_entrada_pagina *)arg;
		return(una_entrada_tlb->pid == info_proceso->pid);
	}

	while(list_find(lista_entradas_TLB,&condicion_proceso) != NULL){
		list_remove_by_condition(lista_entradas_TLB,&condicion_proceso);
	}

	if(!strcmp(info.algoReemplzoTLB,"LRU")){
    	while(list_find(pila_LRU_TLB,&condicion_proceso) != NULL){
    		list_remove_by_condition(pila_LRU_TLB,&condicion_proceso);
    	}
	}

    if(!strcmp(info.tipoAsignacion,"FIJA")){
    	int i = 0;
    	int tamanio = list_size(info_proceso->lista_paginas_presentes);
    	while(tamanio > i){
    		list_add(paginas_presentes,list_remove(info_proceso->lista_paginas_presentes,0));
    		i++;
    	}
		list_destroy(info_proceso->lista_paginas_presentes);
    	free(info_proceso->reloj);
    }
    else{
    	while(list_find(info_proceso->lista_paginas_presentes,&condicion_proceso) != NULL){
    		list_add(paginas_presentes,list_remove_by_condition(info_proceso->lista_paginas_presentes,&condicion_proceso));
    	}
    }
    int i = 0;
    int tamanio = list_size(paginas_presentes);
    while(tamanio > i){
    	t_entrada_pagina *pagina_presente = list_get(paginas_presentes,i);
		t_frame *frame = list_get(lista_frames,pagina_presente->nro_frame);
		frame->vacio = 1;
		i++;
    }

    i = 0;
    tamanio = list_size(info_proceso->tabla_de_paginas);
	while(tamanio > i){
		free(list_remove(info_proceso->tabla_de_paginas,0));
		i++;
	}
	list_destroy(info_proceso->tabla_de_paginas);

    i = 0;
    tamanio = list_size(info_proceso->lista_datos_alloc);
	while(tamanio > i){
		free(list_remove(info_proceso->lista_datos_alloc,0));
		i++;
	}
	list_destroy(info_proceso->lista_datos_alloc);

	sem_destroy(&(info_proceso->swap_disponible));
	list_destroy(paginas_presentes);

	//esto es para comprobar que si ya fue eliminado
	info_proceso->lista_datos_alloc = NULL;
}


void metricas_SIGINT(){
	int i = 0;
	int tlb_hit_totales = 0;
	int tlb_miss_totales = 0;
	t_info_proceso *info_proceso;
	int cantidad_de_procesos = list_size(lista_procesos);
	log_info(logMemoria,"--------------------------------------------------------------");
	while(cantidad_de_procesos > i){
		info_proceso = list_get(lista_procesos,i);
		log_info(logMemoria,"Carpincho %d cantidad TLB Hit: %d",info_proceso->pid,info_proceso->metricas.tlb_hit);
		tlb_hit_totales += info_proceso->metricas.tlb_hit;
		log_info(logMemoria,"Carpincho %d cantidad TLB Miss: %d",info_proceso->pid,info_proceso->metricas.tlb_miss);
		tlb_miss_totales += info_proceso->metricas.tlb_miss;
		i++;
	}
	log_info(logMemoria,"Cantidad de TLB Hit totales: %d",tlb_hit_totales);
	log_info(logMemoria,"Cantidad de TLB Miss totales: %d",tlb_miss_totales);
	log_info(logMemoria,"--------------------------------------------------------------");

}

void dump_SIGURS1(){
    char filepath[256];
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    snprintf(filepath, sizeof(filepath),"%s/Dump_%d-%02d-%02d-%02d:%02d:%02d.tlb",info.path, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    FILE *fptr = fopen(filepath, "w");
    if (fptr == NULL)
    {
        log_info(logMemoria,"Could not open file");
    }

    fprintf(fptr,"Dump: %d-%02d-%02d %02d:%02d:%02d\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
	int i = 0;
	int cantidad_entradas_TLB = list_size(lista_entradas_TLB);
	while(cantidad_entradas_TLB > i){
		t_entrada_pagina *entrada_tlb = list_get(lista_entradas_TLB,i);
		fprintf(fptr,"Entrada:%d Estado:Ocupado Carpincho: %d Pagina: %d Marco: %d\n",i,entrada_tlb->pid,entrada_tlb->nro_pag,entrada_tlb->nro_frame);
		i++;
	}
	while(info.entradasTLB > i){
		fprintf(fptr,"Entrada:%d Estado:Libre Carpincho: - Pagina: - Marco: - \n",i);
		i++;
	}
    fclose(fptr);

}

void limpiar_TLB_SIGURS2(){
	int i = 0;
	int cantidad_entradas_TLB = list_size(lista_entradas_TLB);
	while(cantidad_entradas_TLB > i){
		list_remove(lista_entradas_TLB,0);
		i++;
	}
	i = 0;
	if(!strcmp(info.algoReemplzoTLB,"LRU")){
		int cantidad_entradas_pila_TLB = list_size(pila_LRU_TLB);
		while(cantidad_entradas_pila_TLB > i){
			list_remove(pila_LRU_TLB,0);
			i++;
    	}
	}
	log_info(logMemoria,"Se limpia la TLB");

}

void mostrar_paginas_presentes(){
	int tamanio_lista = list_size(lista_paginas_global);
	int i = 0;
	while(tamanio_lista > i){
		t_entrada_pagina *entrada = list_get(lista_paginas_global,i);
		log_info(logMemoria,"Frame %d Proceso %d Pagina %d",entrada->nro_frame,entrada->pid,entrada->nro_pag);
		i++;
	}
}

void mostrar_tlb(){
	int i = 0;
	int cantidad_entradas_TLB = list_size(lista_entradas_TLB);
	while(cantidad_entradas_TLB > i){
		t_entrada_pagina *entrada_tlb = list_get(lista_entradas_TLB,i);
		log_info(logMemoria,"Entrada:%d Estado:Ocupado Carpincho: %d Pagina: %d Marco: %d\n",i,entrada_tlb->pid,entrada_tlb->nro_pag,entrada_tlb->nro_frame);
		i++;
	}
	while(info.entradasTLB > i){
		log_info(logMemoria,"Entrada:%d Estado:Libre Carpincho: - Pagina: - Marco: - \n",i);
		i++;
	}
}

void suspender_proceso(t_info_proceso *info_proceso){
	t_list *paginas_presentes = list_create();
	bool condicion_proceso(void *arg){
		t_entrada_pagina *una_entrada_tlb = (t_entrada_pagina *)arg;
		return(una_entrada_tlb->pid == info_proceso->pid);
	}

	while(list_find(lista_entradas_TLB,&condicion_proceso)){
		list_remove_by_condition(lista_entradas_TLB,&condicion_proceso);
	}

	if(!strcmp(info.algoReemplzoTLB,"LRU")){
    	while(list_find(pila_LRU_TLB,&condicion_proceso)){
    		list_remove_by_condition(pila_LRU_TLB,&condicion_proceso);
    	}
	}

    if(!strcmp(info.tipoAsignacion,"FIJA")){
    	int i = 0;
    	int tamanio = list_size(info_proceso->lista_paginas_presentes);
    	while(tamanio > i){
    		list_add(paginas_presentes,list_remove(info_proceso->lista_paginas_presentes,0));
    		i++;
    	}
    }
    else{
    	while(list_find(info_proceso->lista_paginas_presentes,&condicion_proceso)){
    		list_add(paginas_presentes,list_remove_by_condition(info_proceso->lista_paginas_presentes,&condicion_proceso));
    	}
    }
    int i = 0;
    int tamanio = list_size(paginas_presentes);
    while(tamanio > i){
    	t_entrada_pagina *pagina_presente = list_get(paginas_presentes,i);
		t_frame *frame = list_get(lista_frames,pagina_presente->nro_frame);
		frame->vacio = 1;
		pagina_presente->presencia = 0;
		pagina_presente->uso = 1;
		if(pagina_presente->modificado){
			pagina_presente->modificado = 0;
			void *pagina = malloc(tamanio_frame);
			memcpy(pagina,frame->ptr_inicio,tamanio_frame);
			escribir_swap(info_proceso,pagina_presente,pagina);
			free(pagina);
		}
		i++;
    }
    list_destroy(paginas_presentes);
}





//FUNCIONES PARA LA CONEXION

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
////		    int activado = 1;
////		    setsockopt(socketServidor, SOL_SOCKET, SO_REUSEADDR, &activado, sizeof(activado));
//
//	        if (bind(socketServidor, p->ai_addr, p->ai_addrlen) == -1) {
//	            close(socketServidor);
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
//
//
//int esperarCliente(int socketServidor, t_log * log) {
//	struct sockaddr_in dirCliente;
//	socklen_t tamDireccion = sizeof(struct sockaddr_in);
//
//	int socketCliente = accept(socketServidor, (void*) &dirCliente, &tamDireccion);
//
//	return socketCliente;
//}







// INICIA SERVER ESCUCHANDO EN IP:PUERTO




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






//int iniciarServidor(char* puerto){
//
//	struct sockaddr_in direccionServer;
//	direccionServer.sin_family = AF_INET;
//	direccionServer.sin_addr.s_addr = INADDR_ANY;
//	direccionServer.sin_port = htons(atoi(puerto));
//
//	int servidor = socket(AF_INET, SOCK_STREAM, 0);
//
//	if(bind(servidor, (void*) &direccionServer, sizeof(direccionServer)) != 0){
//		perror("Fallo el bind");
//		exit(-1);
//	}
//
//	printf("ESCUCHANDO...\n");
//	listen(servidor, SOMAXCONN);
//
//	return servidor;
//}
//
//int esperarCliente(int servidor){
//	struct sockaddr_in direccionCliente;
//	unsigned int tamanioDireccion;
//	int cliente = accept(servidor, (void*) &direccionCliente, &tamanioDireccion);
//	printf("SE CONECTO UN CLIENTE\n");
//	return cliente;
//}









//------------------------------------------------------------

void* serializo(infoPeticion peticion){
	void* buffer = malloc(sizeof(int)*3 + tamanio_frame);

	memcpy(buffer, &(peticion.pedido), sizeof(int));
	int desplazamiento = sizeof(tipoDePeticion);
	memcpy(buffer+desplazamiento, &(peticion.nroDePag), sizeof(int));
	desplazamiento += sizeof(int);
	memcpy(buffer+desplazamiento, &(peticion.proceso), sizeof(int));
	desplazamiento += sizeof(int);

	return buffer;
}

void *lectura(infoPeticion peticion){

	void* aEnviar = serializo(peticion);
	int bytesAEnviar = sizeof(int)*3;
	log_info(logMemoria,"Se lee proceso:%d pagina:%d de SWAmP",peticion.proceso,peticion.nroDePag);

	send(socket_swap, &bytesAEnviar, sizeof(int), 0);
	send(socket_swap, aEnviar, bytesAEnviar, 0);

	free(aEnviar);

	void* bufferARecv = malloc(tamanio_frame);
	recv(socket_swap, bufferARecv, tamanio_frame, MSG_WAITALL);


	return bufferARecv;
}

void escritura(infoPeticion peticion, void* pag){
	void* aEnviar = serializo(peticion);

	int desplazamiento = sizeof(int)*3;
	memcpy(aEnviar+desplazamiento, pag, tamanio_frame);

	int bytesAEnviar = desplazamiento + tamanio_frame;


	log_info(logMemoria,"Se envia proceso:%d pagina:%d a SWAmP",peticion.proceso,peticion.nroDePag);

	send(socket_swap, &bytesAEnviar, sizeof(int), 0);
	send(socket_swap, aEnviar, bytesAEnviar, 0);
	free(aEnviar);
}

void eliminado(infoPeticion peticion){
	void* aEnviar = serializo(peticion);
	int bytesAEnviar = sizeof(int)*3;

	int pedido;
	int numeropag;
	int proceso;
	memcpy(&pedido,aEnviar,sizeof(int));
	memcpy(&numeropag,aEnviar + sizeof(int),sizeof(int));
	memcpy(&proceso,aEnviar + sizeof(int)*2,sizeof(int));
	log_info(logMemoria,"Se elimina proceso:%d de SWAmP",peticion.proceso);


	send(socket_swap, &bytesAEnviar, sizeof(int), 0);
	send(socket_swap, aEnviar, bytesAEnviar, 0);
	free(aEnviar);
}




//int crearConexion(char* ip, char* puerto){
//	struct sockaddr_in direccionServer;
//	direccionServer.sin_family = AF_INET;
//	direccionServer.sin_addr.s_addr = inet_addr(ip);
//	direccionServer.sin_port = htons(atoi(puerto));
//
//	int cliente = socket(AF_INET, SOCK_STREAM, 0);
//	if(connect(cliente, (void*) &direccionServer, sizeof(direccionServer)) != 0){
//		perror("No se pudo conectar");
//		exit(-1);
//	}
//
//	return cliente;
//}


int crearConexion(char* ip, char* puerto){
	struct addrinfo hints;
	struct addrinfo *server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	int inf = getaddrinfo(ip, puerto, &hints, &server_info);
    if(inf == 1){
    	perror("Error");
    	exit(-1);
    }

	int clientSocket = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);

	if(connect(clientSocket, server_info->ai_addr, server_info->ai_addrlen) == -1){
		perror("Error");
	    exit(-1);
	}

	freeaddrinfo(server_info);
	return clientSocket;
}

void nuevo_hilo(int cliente){

	pthread_t newHilo;

	arg_struct* args = malloc(sizeof(arg_struct));
	args->conexion = cliente;

	pthread_create(&newHilo, NULL, (void*)procesar_peticion, (void*)args);
	pthread_detach(newHilo);

}







int sizeDeCarpincho(mate_instanceACambiar* carpincho) {
	int a = 2 * sizeof(int) + sizeof(float) * 2 + sizeof(time_t) * 3;
	int c = sizeConfig(carpincho->info);
	int d = sizeof(tamConfig);
	return a+c+d;
}

void liberarConfig(infoConfig info) {
	free(info.ipKernel);
	free(info.puertoKernel);
	free(info.ipMemoria);
	free(info.puertoMemoria);
	free(info.moduloAConectar);
}

void liberarCarpincho(mate_instanceACambiar* carpincho) {
	liberarConfig(carpincho->info);
	free(carpincho);
}



infoConfig limpiarConfig(infoConfig info, tamConfig tam) {
	infoConfig infoLimpio;

	infoLimpio.ipKernel = string_substring_until(info.ipKernel,
			tam.largoIpKernel);
	infoLimpio.puertoKernel = string_substring_until(info.puertoKernel,
			tam.largoPuertoKernel);
	infoLimpio.ipMemoria = string_substring_until(info.ipMemoria,
			tam.largoIpMemoria);
	infoLimpio.puertoMemoria = string_substring_until(info.puertoMemoria,
			tam.largoPuertoMemoria);
	infoLimpio.moduloAConectar = string_substring_until(info.moduloAConectar,
			tam.largomoduloAConectar);

	return infoLimpio;
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

	memcpy(buffer + desplazamiento, info.ipMemoria,
			string_length(info.ipMemoria));
	desplazamiento += string_length(info.ipMemoria);

	memcpy(buffer + desplazamiento, info.puertoMemoria,
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

