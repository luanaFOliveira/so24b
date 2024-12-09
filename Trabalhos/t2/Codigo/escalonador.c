#include <stdlib.h>

#include "escalonador.h"
#include "es.h"
#include "tela.h"


struct no_t {
    processo_t *processo;
    struct no_t *prox;
};

typedef struct no_t no_t;

struct escalonador_t {
    no_t *inicio;
    no_t *fim;  
    escalonador_tipo_t tipo_escalonador;
};

escalonador_t *escalonador_cria(escalonador_tipo_t tipo_escalonador) {
    escalonador_t *self = (escalonador_t *) malloc(sizeof(*self));
    if (self == NULL) return NULL;
    self->inicio = NULL;
    self->fim = NULL;
    self->tipo_escalonador = tipo_escalonador;
    return self;
}

void escalonador_destroi(escalonador_t *self, mmu_t *mmu,controle_quadros_t *controle_quadros) {
    if (self == NULL) return;

    no_t *p = self->inicio;
    while (p != NULL) {
        no_t *t = p->prox;
        processo_destroi(p->processo, mmu, controle_quadros);
        free(p);
        if (t == self->inicio) break;
        p = t;
    }
    free(self);
}


void escalonador_adiciona_processo(escalonador_t *self, processo_t *processo) {
    if (self == NULL || processo == NULL) return;
    no_t *no = (no_t *) malloc(sizeof(*no));
    no->processo = processo;
    no->prox = self->inicio;

    if (self->fim == NULL) {
        self->inicio = no;
    } else {
        self->fim->prox = no;
    }
    self->fim = no;
    self->fim->prox = self->inicio; 
}

void escalonador_remove_processo(escalonador_t *self, processo_t *processo) {
    if (self == NULL || self->inicio == NULL || processo == NULL) return;

    no_t *ant = self->fim;
    no_t *p = self->inicio;

    do {
        if (p->processo == processo) {
            if (p == self->inicio && p == self->fim) {
                self->inicio = self->fim = NULL;
            }
            else if (p == self->inicio) {
                self->inicio = p->prox;
                self->fim->prox = self->inicio;
            }
            else if (p == self->fim) {
                self->fim = ant;
                self->fim->prox = self->inicio; 
            }
            else {
                ant->prox = p->prox;
            }
            free(p);
            return;
        }
        ant = p;
        p = p->prox;
    } while (p != self->inicio);
    console_printf("Processo não encontrado\n");
}



processo_t *escalonador_proximo(escalonador_t *self){
    switch (self->tipo_escalonador)
    {
    case SIMPLES:
        return escalonador_proximo_processo_simples(self);
        break;
    case CIRCULAR:
        return escalonador_proximo_processo_circular(self);
        break;
    case PRIORIDADE:
        return escalonador_proximo_processo_prioridade(self);
        break;
    default:
        return NULL;
        break;
    }
}

processo_t *escalonador_proximo_processo_simples(escalonador_t *self) {
    if (self == NULL || self->inicio == NULL) return NULL;
    return self->inicio->processo; 
}

processo_t *escalonador_proximo_processo_circular(escalonador_t *self) {
    if (self == NULL || self->inicio == NULL) return NULL;
    
    no_t *p = self->inicio;
    if (p == NULL) return NULL; 
    
    self->inicio = self->inicio->prox; 
    return p->processo;
}



processo_t *escalonador_proximo_processo_prioridade(escalonador_t *self) {
    if (self == NULL || self->inicio == NULL) return NULL;

    no_t *p = self->inicio;
    no_t *selecionado = NULL;

    do {
        if (processo_estado(p->processo) == PRONTO) {
            if (selecionado == NULL || processo_prioridade(p->processo) < processo_prioridade(selecionado->processo)) {
                selecionado = p;
            }
        }
        p = p->prox;
    } while (p != self->inicio);

    return (selecionado != NULL) ? selecionado->processo : NULL;
}







