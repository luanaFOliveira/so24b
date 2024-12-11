#ifndef PROCESSO_H
#define PROCESSO_H
#include "tabpag.h"
#include "mmu.h"
#include "err.h"

typedef struct processo_t processo_t;


typedef enum {
    EM_EXECUCAO,
    PRONTO,
    BLOQUEADO,
    MORTO
} processo_estado_t;

typedef enum {
    LEITURA,
    ESCRITA,
    ESPERA,
    ESPERA_PAGINA,
    SEM_MOTIVO
} bloqueio_motivo_t;


processo_t *processo_cria(int id);
void processo_destroi(processo_t *self, controle_quadros_t *controle_quadros);
void processo_set_pc(processo_t *processo, int pc);

// Getters
int processo_pid(processo_t *processo);
processo_estado_t processo_estado(processo_t *processo);
bloqueio_motivo_t processo_motivo_bloqueio(processo_t *processo);
int processo_tipo_bloqueio(processo_t *processo);
int processo_es_id(processo_t *processo);
int processo_PC(processo_t *processo);
int processo_A(processo_t *processo);
int processo_X(processo_t *processo);
int processo_complemento(processo_t *processo);
int processo_endereco_disco(processo_t *processo);
err_t processo_erro(processo_t *processo);
double processo_prioridade(processo_t *processo);
float processo_tempo_retorno(processo_t *processo);
int processo_num_preempcoes(processo_t *processo);
int processo_tempo_medio_resposta(processo_t *processo);
const char* processo_estado_nome(processo_t* processo);
const char* processo_bloqueio_nome(processo_t* processo);
int processo_num_vezes_estado(processo_t *processo, int estado_id);
int processo_tempo_estado(processo_t *processo, int estado_id);
tabpag_t *processo_tab_pag(processo_t *processo);
// Setters
void processo_set_es_id(processo_t *processo, int es_id);
void processo_set_PC(processo_t *processo, int pc);
void processo_set_A(processo_t *processo, int a);
void processo_set_X(processo_t *processo, int x);
void processo_set_complemento(processo_t *processo, int complemento);
void processo_set_erro(processo_t *processo, err_t erro);
void processo_set_endereco_disco(processo_t *processo, int ender_disco);
void processo_set_prioridade(processo_t *processo, double prioridade);
void processo_set_estado(processo_t *processo,processo_estado_t estado);

void processo_muda_estado(processo_t *processo, processo_estado_t estado);
void processo_bloqueia(processo_t *processo, bloqueio_motivo_t motivo_bloqueio, int tipo_bloqueio);
void processo_desbloqueia(processo_t *processo);
void processo_para(processo_t *processo);
void processo_executa(processo_t *processo;
void processo_encerra(processo_t *processo);
void processo_libera_es(processo_t *processo);
void atualiza_prioridade(processo_t *processo);
void processo_metricas(processo_t *processo, int delta);
#endif 
