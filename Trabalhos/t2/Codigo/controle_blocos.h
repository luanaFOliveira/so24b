#ifndef CONTROLE_BLOCOS_H
#define CONTROLE_BLOCOS_H

#include <stdbool.h>

typedef struct {
    bool em_uso;
    int processo_pid;
    int pagina;
    int ciclos;
    bool chance;
} bloco_t;

typedef struct controle_blocos_t {
    bloco_t *blocos; 
    int total_blocos; 
} controle_blocos_t;

controle_blocos_t *cria_controle_blocos(int tam);
bool controle_blocos_bloco_disponivel(controle_blocos_t *self);
int controle_blocos_busca_bloco_disponivel(controle_blocos_t *self);
#endif // CONTROLE_BLOCOS_H
