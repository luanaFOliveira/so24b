#ifndef ESCALONADOR_H
#define ESCALONADOR_H

#include <stdbool.h>
#include "processo.h"
#include "console.h"

typedef struct escalonador_t escalonador_t;

// Enum para definir o tipo de escalonador
typedef enum {
    SIMPLES,
    CIRCULAR,
    PRIORIDADE
} escalonador_tipo_t;

// Funções do escalonador

// Cria um novo escalonador
escalonador_t *escalonador_cria(escalonador_tipo_t tipo_escalonador);

// Destroi o escalonador e libera a memória associada
void escalonador_destroi(escalonador_t *self);

// Adiciona um processo ao escalonador
void escalonador_adiciona_processo(escalonador_t *self, processo_t *processo);

// Remove um processo do escalonador
void escalonador_remove_processo(escalonador_t *self, processo_t *processo);

// Retorna o próximo processo para execução, baseado no tipo de escalonador
processo_t *escalonador_proximo(escalonador_t *self);

// Funções auxiliares para selecionar o próximo processo, conforme o tipo de escalonador
processo_t *escalonador_proximo_processo_simples(escalonador_t *self);
processo_t *escalonador_proximo_processo_circular(escalonador_t *self);
processo_t *escalonador_proximo_processo_prioridade(escalonador_t *self);



#endif // ESCALONADOR_H
