#include "controle_es.h"
#include <stdlib.h>
#include <assert.h>

typedef struct dispositivo_es_t {
    dispositivo_id_t leitura;
    dispositivo_id_t leitura_ok;
    dispositivo_id_t escrita;
    dispositivo_id_t escrita_ok;
    bool reservada;
    int id;  
} dispositivo_es_t;

struct controle_es_t {
    es_t* es;
    dispositivo_es_t* dispositivos; 
    int num_dispositivos;   
};

controle_es_t* cria_controle_es(es_t* es) {
    controle_es_t* self = (controle_es_t*)malloc(sizeof(controle_es_t));
    assert(self != NULL);
    self->es = es;
    self->dispositivos = NULL;   
    self->num_dispositivos = 0;  
    return self;
}

void destroi_controle_es(controle_es_t* self) {
    if (self->dispositivos) {
        free(self->dispositivos);
    }
    free(self);
}

int controle_registra_dispositivo(controle_es_t* self, dispositivo_id_t leitura, dispositivo_id_t leitura_ok, dispositivo_id_t escrita, dispositivo_id_t escrita_ok) {
    self->dispositivos = realloc(self->dispositivos, (self->num_dispositivos + 1) * sizeof(dispositivo_es_t));
    assert(self->dispositivos != NULL);

    dispositivo_es_t* novo_dispositivo = &self->dispositivos[self->num_dispositivos];

    novo_dispositivo->leitura = leitura;
    novo_dispositivo->leitura_ok = leitura_ok;
    novo_dispositivo->escrita = escrita;
    novo_dispositivo->escrita_ok = escrita_ok;
    novo_dispositivo->reservada = false;
    novo_dispositivo->id = self->num_dispositivos;  

    self->num_dispositivos++;

    return novo_dispositivo->id;
}

bool controle_le_dispositivo(controle_es_t *self, int id, int *pvalor)
{
  int ok;

  if (id < self->num_dispositivos){

    if (es_le(self->es, self->dispositivos[id].leitura_ok, &ok) != ERR_OK) {
        return false;
    }

    if (!ok) {
        return false;
    }

  }

  return es_le(self->es, self->dispositivos[id].leitura, pvalor) == ERR_OK;
}

bool controle_escreve_dispositivo(controle_es_t* self, int id, int valor) {
    for (int i = 0; i < self->num_dispositivos; i++) {
        dispositivo_es_t* es_atual = &self->dispositivos[i];
        if (es_atual->id == id) {
            int ok;
            if (es_le(self->es, es_atual->escrita_ok, &ok) != ERR_OK || !ok) {
                return false;
            }
            return es_escreve(self->es, es_atual->escrita, valor) == ERR_OK;
        }
    }
    return false;
}

void controle_reserva_dispositivo(controle_es_t* self, int id) {
    for (int i = 0; i < self->num_dispositivos; i++) {
        dispositivo_es_t* es_atual = &self->dispositivos[i];
        if (es_atual->id == id && !es_atual->reservada) {
            es_atual->reservada = true;
            return;
        }
    }
}

void controle_libera_dispositivo(controle_es_t* self, int id) {
    for (int i = 0; i < self->num_dispositivos; i++) {
        dispositivo_es_t* es_atual = &self->dispositivos[i];
        if (es_atual->id == id && es_atual->reservada) {
            es_atual->reservada = false;
            return;
        }
    }
}

int controle_dispositivo_disponivel(controle_es_t* self) {
    for (int i = 0; i < self->num_dispositivos; i++) {
        if (!self->dispositivos[i].reservada) {
            controle_reserva_dispositivo(self, self->dispositivos[i].id);
            return self->dispositivos[i].id;
        }
    }
    return -1;  
}
