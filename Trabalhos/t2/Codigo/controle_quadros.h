#ifndef CONTROLE_QUADROS_H
#define CONTROLE_QUADROS_H

#include <stdbool.h>

typedef struct controle_quadros_t controle_quadros_t;

controle_quadros_t *controle_quadros_cria(int total_quadros);

void controle_quadros_destroi(controle_quadros_t *self);

int controle_quadros_aloca(controle_quadros_t *self);

void controle_quadros_libera(controle_quadros_t *self, int quadro);

bool controle_quadros_esta_livre(controle_quadros_t *self, int quadro);

#endif 
