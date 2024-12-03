#ifndef CONTROLE_ES_H
#define CONTROLE_ES_H

#include <stdbool.h>
#include "es.h"


typedef struct controle_es_t controle_es_t;  

controle_es_t* cria_controle_es(es_t* es);
void destroi_controle_es(controle_es_t* self);

int controle_registra_dispositivo(controle_es_t* self, dispositivo_id_t leitura, dispositivo_id_t leitura_ok, dispositivo_id_t escrita, dispositivo_id_t escrita_ok);
bool controle_le_dispositivo(controle_es_t* self, int id, int* pvalor);
bool controle_escreve_dispositivo(controle_es_t* self, int id, int valor);
void controle_reserva_dispositivo(controle_es_t* self, int id);
void controle_libera_dispositivo(controle_es_t* self, int id);
int controle_dispositivo_disponivel(controle_es_t* self);

#endif 
