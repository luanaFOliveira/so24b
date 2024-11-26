#ifndef CONTROLE_PORTAS_H
#define CONTROLE_PORTAS_H

#include <stdbool.h>
#include "es.h"


typedef struct controle_portas_t controle_portas_t;  

controle_portas_t* controle_cria_portas(es_t* es);
void controle_destroi_portas(controle_portas_t* self);

int controle_registra_porta(controle_portas_t* self, dispositivo_id_t leitura, dispositivo_id_t leitura_ok, dispositivo_id_t escrita, dispositivo_id_t escrita_ok);
bool controle_le_porta(controle_portas_t* self, int id, int* pvalor);
bool controle_escreve_porta(controle_portas_t* self, int id, int valor);
void controle_reserva_porta(controle_portas_t* self, int id);
void controle_libera_porta(controle_portas_t* self, int id);
int controle_porta_disponivel(controle_portas_t* self);

#endif 
