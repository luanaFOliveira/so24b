#include <stdlib.h>
#include "controle_blocos.h"

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
