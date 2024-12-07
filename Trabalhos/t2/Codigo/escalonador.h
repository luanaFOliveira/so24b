#ifndef ESCALONADOR_H
#define ESCALONADOR_H

#include <stdbool.h>
#include "processo.h"
#include "console.h"

typedef struct escalonador_t escalonador_t;

typedef enum {
    SIMPLES,
    CIRCULAR,
    PRIORIDADE
} escalonador_tipo_t;


escalonador_t *escalonador_cria(escalonador_tipo_t tipo_escalonador);

void escalonador_destroi(escalonador_t *self, mmu_t *mmu,controle_quadros_t *controle_quadros);

void escalonador_adiciona_processo(escalonador_t *self, processo_t *processo);

void escalonador_remove_processo(escalonador_t *self, processo_t *processo);

processo_t *escalonador_proximo(escalonador_t *self);

processo_t *escalonador_proximo_processo_simples(escalonador_t *self);
processo_t *escalonador_proximo_processo_circular(escalonador_t *self);
processo_t *escalonador_proximo_processo_prioridade(escalonador_t *self);



#endif 
