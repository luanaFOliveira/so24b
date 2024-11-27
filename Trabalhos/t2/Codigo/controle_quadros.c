#include "controle_quadros.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>

// Estrutura interna para controle de quadros
struct controle_quadros_t {
    int total_quadros;      // Número total de quadros
    char *bitmap;           // Bitmap para rastrear quadros livres/ocupados
};

controle_quadros_t *controle_quadros_cria(int total_quadros) {
    controle_quadros_t *self = malloc(sizeof(*self));
    assert(self != NULL);

    self->total_quadros = total_quadros;
    self->bitmap = calloc((total_quadros + 7) / 8, sizeof(char));
    assert(self->bitmap != NULL);

    return self;
}

void controle_quadros_destroi(controle_quadros_t *self) {
    if (self != NULL) {
        free(self->bitmap);
        free(self);
    }
}

int controle_quadros_aloca(controle_quadros_t *self) {
    assert(self != NULL);
    for (int i = 0; i < self->total_quadros; i++) {
        int byte = i / 8;
        int bit = i % 8;

        if (!(self->bitmap[byte] & (1 << bit))) {
            self->bitmap[byte] |= (1 << bit); // Marca como ocupado
            return i;
        }
    }
    return -1; // Nenhum quadro livre encontrado
}

void controle_quadros_libera(controle_quadros_t *self, int quadro) {
    assert(self != NULL);
    assert(quadro >= 0 && quadro < self->total_quadros);

    int byte = quadro / 8;
    int bit = quadro % 8;

    self->bitmap[byte] &= ~(1 << bit); // Marca como livre
}

bool controle_quadros_esta_livre(controle_quadros_t *self, int quadro) {
    assert(self != NULL);
    assert(quadro >= 0 && quadro < self->total_quadros);

    int byte = quadro / 8;
    int bit = quadro % 8;

    return !(self->bitmap[byte] & (1 << bit));
}
