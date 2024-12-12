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

void escalonador_destroi(escalonador_t *self,controle_quadros_t *controle_quadros) {
    if (self == NULL) return;

    no_t *p = self->inicio;
    while (p != NULL) {
        no_t *t = p->prox;
        free(p);
        if (t == self->inicio) break;
        p = t;
    }
    free(self);
}


void escalonador_adiciona_processo(escalonador_t *self, processo_t *processo) {
    if (self == NULL || processo == NULL) return;

    no_t *no = (no_t *)malloc(sizeof(*no));
    if (no == NULL) return; // Falha na alocação de memória

    no->processo = processo;

    if (self->inicio == NULL) { // Lista vazia
        self->inicio = self->fim = no;
        no->prox = self->inicio; // Torna a lista circular
    } else {
        no->prox = self->inicio;
        self->fim->prox = no;
        self->fim = no;
    }
}


void escalonador_remove_processo(escalonador_t *self, processo_t *processo) {
    if (self == NULL || self->inicio == NULL || processo == NULL) return;

    no_t *ant = self->fim;
    no_t *p = self->inicio;

    do {
        if (p->processo == processo) {
            if (p == self->inicio && p == self->fim) {
                self->inicio = self->fim = NULL;
                console_printf("Processo removido pid do carai = %d\n", processo_pid(processo));
                free(p);
                return;
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
            console_printf("Processo removido pid = %d\n", processo_pid(processo));
            free(p);
            return;
        }
        ant = p;
        p = p->prox;
    } while (p != self->inicio);
    console_printf("Processo não encontrado pid = %d\n", processo_pid(processo));
}



processo_t *escalonador_proximo(escalonador_t *self){

    console_printf("Escalonador tipo: %d\n", self->tipo_escalonador);
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
    if(self == NULL ) {
        console_printf("Escalonador  self NULL\n");
        return NULL;
    }
    if (self->inicio == NULL) {
        console_printf("Escalonador inicio NULL\n");
        return NULL;

    } 
    return self->inicio->processo; 
}

processo_t *escalonador_proximo_processo_circular(escalonador_t *self) {
    console_printf("Escalonador tipo circular00\n");

    if (self->inicio == NULL) {
        console_printf("luana\n");
        return NULL;

    } 

    console_printf("Escalonador tipo circular01\n");

        console_printf("Escalonador tipo circular02\n");

    return self->inicio->processo;
}



processo_t *escalonador_proximo_processo_prioridade(escalonador_t *self) {
        console_printf("Escalonador tipo prioridade\n");
    if (self == NULL || self->inicio == NULL) return NULL;

    no_t *p = self->inicio;
    no_t *selecionado = NULL;


    do {
        if (processo_estado(p->processo) == PRONTO) {
            if (selecionado == NULL || processo_prioridade(p->processo) < processo_prioridade(selecionado->processo)) {
                selecionado = p;
            }
        }
        console_printf("nao ta pronto\n");
        p = p->prox;
    } while (p != self->inicio);

    return (selecionado != NULL) ? selecionado->processo : NULL;
}







