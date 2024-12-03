#include <stdlib.h>

#include "processo.h"

struct processo_t {
    processo_estado_t estado;
    int pid;
    double prioridade;
    bloqueio_motivo_t motivo_bloqueio;
    int tipo_bloqueio, es_id, PC, X, A;

    //metricas
    float tempo_retorno;
    int num_preempcoes;

    int num_vezes_estados[4];
    int tempo_estados[4];

    int tempo_medio_resposta;

    tabpag_t *tab_pag;

};


processo_t *processo_cria(int id, int pc) {
    processo_t *self = malloc(sizeof(*self));
    if (self == NULL) return NULL;
    self->estado = PRONTO;
    self->motivo_bloqueio = SEM_MOTIVO;
    self->tipo_bloqueio = -1;
    self->pid = id;
    self->PC = pc;
    self->A = 0;
    self->X = 0;
    self->es_id = -1;
    self->prioridade = 0.5;   

    //metricas
    self->tempo_retorno = 0;
    self->num_preempcoes = 0;
    self->tempo_medio_resposta = 0;
    for (int i = 0; i < 4; i++) {
        self->num_vezes_estados[i] = 0;
        self->tempo_estados[i] = 0;
    }
    self->num_vezes_estados[PRONTO]++;
    
    self->tab_pag = tabpag_cria();
    return self;
}


//getters

int processo_pid(processo_t *processo) {
    if (processo == NULL)
        exit(1);
    return processo->pid;
}

processo_estado_t processo_estado(processo_t *processo) {
    if (processo == NULL)
        exit(1);
    return processo->estado;
}

bloqueio_motivo_t processo_motivo_bloqueio(processo_t *processo) {
    if (processo == NULL)
        exit(1);
    return processo->motivo_bloqueio;
}

int processo_tipo_bloqueio(processo_t *processo) {
    if (processo == NULL)
        exit(1);
    return processo->tipo_bloqueio;
}

int processo_es_id(processo_t *processo){
    if (processo == NULL)
        exit(1);
    return processo->es_id;
}

int processo_PC(processo_t *processo){
    if (processo == NULL)
        exit(1);
    return processo->PC;
}

int processo_A(processo_t *processo){
    if (processo == NULL)
        exit(1);
    return processo->A;
}

int processo_X(processo_t *processo){
    if (processo == NULL)
        exit(1);
    return processo->X;
}

double processo_prioridade(processo_t *processo){
    if (processo == NULL)
        exit(1);
    return processo->prioridade;
}

float processo_tempo_retorno(processo_t *processo){
    if (processo == NULL)
        exit(1);
    return processo->tempo_retorno;
}

int processo_num_preempcoes(processo_t *processo){
    if (processo == NULL)
        exit(1);
    return processo->num_preempcoes;
}

int processo_tempo_medio_resposta(processo_t *processo){
    if (processo == NULL)
        exit(1);
    return processo->tempo_medio_resposta;
}

const char* processo_estado_nome(processo_t* processo) {
    switch (processo->estado) {
        case EM_EXECUCAO: return "Em Execução";
        case PRONTO:      return "Pronto";
        case BLOQUEADO:   return "Bloqueado";
        case MORTO:       return "Morto";
        default:          return "Estado Desconhecido";
    }
}

const char* processo_bloqueio_nome(processo_t* processo) {
    switch (processo->motivo_bloqueio) {
        case LEITURA:     return "Leitura";
        case ESCRITA:     return "Escrita";
        case ESPERA:      return "Espera";
        case SEM_MOTIVO:  return "Sem Motivo";
        default:          return "Motivo Desconhecido";
    }
}

int processo_num_vezes_estado(processo_t *processo, int estado_id){
    if (processo == NULL)
        exit(1);
    return processo->num_vezes_estados[estado_id];
}

int processo_tempo_estado(processo_t *processo, int estado_id){
    if (processo == NULL)
        exit(1);
    return processo->tempo_estados[estado_id];
}

tabpag_t *processo_tab_pag(processo_t *processo) {
    return self->tab_pag;
}

//setters
void processo_set_es_id(processo_t *processo, int es_id){
    if (processo == NULL)
        exit(1);
    processo->es_id = es_id;
}

void processo_set_PC(processo_t *processo, int pc){
    if (processo == NULL)
        exit(1);
    processo->PC = pc;
}

void processo_set_A(processo_t *processo, int a){
    if (processo == NULL)
        exit(1);
    processo->A = a;
}

void processo_set_X(processo_t *processo, int x){
    if (processo == NULL)
        exit(1);
    processo->X = x;
}

void processo_set_prioridade(processo_t *processo,double prioridade){
    if (processo == NULL)
        exit(1);
    processo->prioridade = prioridade;
}

void processo_set_estado(processo_t *processo,processo_estado_t estado){
    if (processo == NULL)
        exit(1);

    // metricas
    if(processo->estado == EM_EXECUCAO && estado == PRONTO){
        processo->num_preempcoes++;
    }
    processo->num_vezes_estados[estado]++;
    processo->estado = estado;
}

// funcoes


void processo_destroi(processo_t *self, mmu_t *mmu,controle_quadros_t *controle_quadros) {
    if (self == NULL)
        exit(1);
    tabpag_destroi(self->tab_pag, controle_quadros);
    mmu_define_tabpag(mmu, NULL);   
    free(self);
}


void processo_bloqueia(processo_t *processo, bloqueio_motivo_t motivo_bloqueio, int tipo_bloqueio) {
    if (processo == NULL)
        exit(1);
    if( processo->estado == PRONTO || processo->estado == EM_EXECUCAO){
        processo_set_estado(processo, BLOQUEADO);
        //processo->estado = BLOQUEADO;
        processo->motivo_bloqueio = motivo_bloqueio;
        processo->tipo_bloqueio = tipo_bloqueio;
        
    }   
}

void processo_desbloqueia(processo_t *processo) {
    if (processo == NULL)
        exit(1);   
    if( processo->estado == BLOQUEADO){
        processo_set_estado(processo, PRONTO);
        //processo->estado = PRONTO;
        // processo->motivo_bloqueio = SEM_MOTIVO;
        // processo->tipo_bloqueio = -1;
    }   
}

void processo_para(processo_t *processo) {
    if (processo == NULL)
        exit(1);  
    if(processo->estado == EM_EXECUCAO){
        processo_set_estado(processo, PRONTO);
        //processo->estado = PRONTO;
    } 
}


void processo_executa(processo_t *processo, mmu_t *mmu) {
    if (processo == NULL)
        exit(1);  
    if(processo->estado == PRONTO){
        processo_set_estado(processo, EM_EXECUCAO);
        //processo->estado = EM_EXECUCAO;
    } 
    mmu_define_tabpag(mmu, tab_pag);
}

void processo_encerra(processo_t *processo) {
    if (processo == NULL)
        exit(1);  
    processo_set_estado(processo, MORTO);
    //processo->estado = MORTO;
    
}

void processo_libera_es(processo_t *processo){
    if (processo == NULL)
        exit(1);
    processo->es_id = -1;
}

void processo_metricas(processo_t *processo, int delta){
    if(processo->estado != MORTO){
        processo->tempo_retorno += delta;
    }
    processo->tempo_estados[processo->estado] += delta;
    processo->tempo_medio_resposta = processo->tempo_estados[PRONTO];
    processo->tempo_medio_resposta /= processo->num_vezes_estados[PRONTO];
}