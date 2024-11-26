#include "controle_portas.h"
#include <stdlib.h>
#include <assert.h>

typedef struct porta_t {
    dispositivo_id_t leitura;
    dispositivo_id_t leitura_ok;
    dispositivo_id_t escrita;
    dispositivo_id_t escrita_ok;
    bool reservada;
    int id;  
} porta_t;

struct controle_portas_t {
    es_t* es;
    porta_t* portas; 
    int num_portas;   
};

controle_portas_t* controle_cria_portas(es_t* es) {
    controle_portas_t* self = (controle_portas_t*)malloc(sizeof(controle_portas_t));
    assert(self != NULL);
    self->es = es;
    self->portas = NULL;   
    self->num_portas = 0;  
    return self;
}

void controle_destroi_portas(controle_portas_t* self) {
    if (self->portas) {
        free(self->portas);
    }
    free(self);
}

int controle_registra_porta(controle_portas_t* self, dispositivo_id_t leitura, dispositivo_id_t leitura_ok, dispositivo_id_t escrita, dispositivo_id_t escrita_ok) {
    self->portas = realloc(self->portas, (self->num_portas + 1) * sizeof(porta_t));
    assert(self->portas != NULL);

    porta_t* nova_porta = &self->portas[self->num_portas];

    nova_porta->leitura = leitura;
    nova_porta->leitura_ok = leitura_ok;
    nova_porta->escrita = escrita;
    nova_porta->escrita_ok = escrita_ok;
    nova_porta->reservada = false;
    nova_porta->id = self->num_portas;  

    self->num_portas++;

    return nova_porta->id;
}

bool controle_le_porta(controle_portas_t* self, int id, int* pvalor) {
    for (int i = 0; i < self->num_portas; i++) {
        porta_t* porta_atual = &self->portas[i];
        if (porta_atual->id == id) {
            int ok;
            if (es_le(self->es, porta_atual->leitura_ok, &ok) != ERR_OK || !ok) {
                return false;
            }
            return es_le(self->es, porta_atual->leitura, pvalor) == ERR_OK;
        }
    }
    return false;
}

bool controle_escreve_porta(controle_portas_t* self, int id, int valor) {
    for (int i = 0; i < self->num_portas; i++) {
        porta_t* porta_atual = &self->portas[i];
        if (porta_atual->id == id) {
            int ok;
            if (es_le(self->es, porta_atual->escrita_ok, &ok) != ERR_OK || !ok) {
                return false;
            }
            return es_escreve(self->es, porta_atual->escrita, valor) == ERR_OK;
        }
    }
    return false;
}

void controle_reserva_porta(controle_portas_t* self, int id) {
    for (int i = 0; i < self->num_portas; i++) {
        porta_t* porta_atual = &self->portas[i];
        if (porta_atual->id == id && !porta_atual->reservada) {
            porta_atual->reservada = true;
            return;
        }
    }
}

void controle_libera_porta(controle_portas_t* self, int id) {
    for (int i = 0; i < self->num_portas; i++) {
        porta_t* porta_atual = &self->portas[i];
        if (porta_atual->id == id && porta_atual->reservada) {
            porta_atual->reservada = false;
            return;
        }
    }
}

int controle_porta_disponivel(controle_portas_t* self) {
    for (int i = 0; i < self->num_portas; i++) {
        if (!self->portas[i].reservada) {
            controle_reserva_porta(self, self->portas[i].id);
            return self->portas[i].id;
        }
    }
    return -1;  
}
