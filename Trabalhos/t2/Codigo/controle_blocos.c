#include <stdlib.h>
#include "controle_blocos.h"

// Criação do controle de blocos
controle_blocos_t *cria_controle_blocos(int tam) {
    controle_blocos_t *self = malloc(sizeof(controle_blocos_t));
    if (!self) {
        return NULL; 
    }

    self->blocos = malloc(sizeof(bloco_t) * tam);
    if (!self->blocos) {
        free(self);
        return NULL; 
    }

    self->total_blocos = tam;
    for (int i = 0; i < tam; i++) {
        if (i < 10) {
            self->blocos[i].em_uso = true; 
        } else {
            self->blocos[i].em_uso = false; 
            self->blocos[i].processo_pid = 0;
        }
    }

    return self;
}

bool controle_blocos_bloco_disponivel(controle_blocos_t *self) {
    for (int i = 0; i < self->total_blocos; i++) {
        if (!self->blocos[i].em_uso) {
            return true;
        }
    }
    return false;
}

// Encontra o índice de um bloco livre
int controle_blocos_busca_bloco_disponivel(controle_blocos_t *self) {
    for (int i = 0; i < self->total_blocos; i++) {
        if (!self->blocos[i].em_uso) {
            return i;
        }
    }
    return -1;
}
