#ifndef MODULO_SWAMP_H_
#define MODULO_SWAMP_H_

#include "peticiones.h"
#include <errno.h>

void iniciarParticiones();

t_bitarray* iniciarBitmapDeParticiones();

void liberarParticiones();

infConf obtenerInfoConfig();

void* sincronizar();

int iniciarServidor(char* ip, char* puerto);
int esperarCliente(int socketServidor);

#endif /* MODULO_SWAMP_H_ */
