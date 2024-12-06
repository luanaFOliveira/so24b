// so.c
// sistema operacional
// simulador de computador
// so24b

// INCLUDES {{{1
#include "so.h"
#include "dispositivos.h"
#include "irq.h"
#include "programa.h"
#include "instrucao.h"
#include "escalonador.h"
#include "processo.h"
#include "controle_portas.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

// CONSTANTES E TIPOS {{{1
// intervalo entre interrupções do relógio
#define INTERVALO_INTERRUPCAO 50   // em instruções executadas
#define MAX_PROCESSOS 16 // numero máximo de processos
#define QUANTUM 5
#define NUM_TERMINAIS 4
#define TIPO_ESCALONADOR PRIORIDADE

struct so_t {
  cpu_t *cpu;
  mem_t *mem;
  es_t *es;
  console_t *console;
  bool erro_interno;

  processo_t **tabela_processos;  
  int num_processos, tamanho_tabela_processos;  

  processo_t *processo_corrente;  

  escalonador_t *escalonador;

  controle_portas_t *controle_portas;

  int tempo_quantum, tempo_restante, tempo_relogio_atual;

  //metricas
  int num_processos_criados;
  float tempo_total_exec;
  float tempo_total_ocioso;
  int num_total_preempcoes;
  int num_vezes_interrupocoes[N_IRQ];

};


// função de tratamento de interrupção (entrada no SO)
static int so_trata_interrupcao(void *argC, int reg_A);

// funções auxiliares
// carrega o programa contido no arquivo na memória do processador; retorna end. inicial
static int so_carrega_programa(so_t *self, char *nome_do_executavel);
// copia para str da memória do processador, até copiar um 0 (retorna true) ou tam bytes
static bool copia_str_da_mem(int tam, char str[tam], mem_t *mem, int ender);
static void so_imprime_metricas(so_t *self);

// CRIAÇÃO {{{1


so_t *so_cria(cpu_t *cpu, mem_t *mem, es_t *es, console_t *console)
{
  so_t *self = malloc(sizeof(*self));
  if (self == NULL) return NULL;

  self->cpu = cpu;
  self->mem = mem;
  self->es = es;
  self->console = console;
  self->erro_interno = false;

  self->escalonador = escalonador_cria(TIPO_ESCALONADOR);

  self->processo_corrente = NULL;
  self->num_processos = 0;
  self->tamanho_tabela_processos = MAX_PROCESSOS;

  self->tabela_processos = malloc(self->tamanho_tabela_processos * sizeof(processo_t *));

  self->tempo_quantum = QUANTUM;
  self->tempo_restante = 0;
  self->tempo_relogio_atual = -1;

  self->controle_portas = controle_cria_portas(es);
  controle_registra_porta(self->controle_portas, D_TERM_A_TECLADO, D_TERM_A_TECLADO_OK, D_TERM_A_TELA, D_TERM_A_TELA_OK);
  controle_registra_porta(self->controle_portas, D_TERM_B_TECLADO, D_TERM_B_TECLADO_OK, D_TERM_B_TELA, D_TERM_B_TELA_OK);
  controle_registra_porta(self->controle_portas, D_TERM_C_TECLADO, D_TERM_C_TECLADO_OK, D_TERM_C_TELA, D_TERM_C_TELA_OK);
  controle_registra_porta(self->controle_portas, D_TERM_D_TECLADO, D_TERM_D_TECLADO_OK, D_TERM_D_TELA, D_TERM_D_TELA_OK);
  
  //metricas
  self->num_processos_criados = 0;
  self->tempo_total_exec = 0;
  self->tempo_total_ocioso = 0;
  self->num_total_preempcoes = 0;
  for (int i = 0; i < N_IRQ; i++) {
    self->num_vezes_interrupocoes[i] = 0;
  }

  // quando a CPU executar uma instrução CHAMAC, deve chamar a função
  //   so_trata_interrupcao, com primeiro argumento um ptr para o SO
  cpu_define_chamaC(self->cpu, so_trata_interrupcao, self);

  // coloca o tratador de interrupção na memória
  // quando a CPU aceita uma interrupção, passa para modo supervisor, 
  //   salva seu estado à partir do endereço 0, e desvia para o endereço
  //   IRQ_END_TRATADOR
  // colocamos no endereço IRQ_END_TRATADOR o programa de tratamento
  //   de interrupção (escrito em asm). esse programa deve conter a 
  //   instrução CHAMAC, que vai chamar so_trata_interrupcao (como
  //   foi definido acima)
  int ender = so_carrega_programa(self, "trata_int.maq");
  if (ender != IRQ_END_TRATADOR) {
    console_printf("SO: problema na carga do programa de tratamento de interrupção");
    self->erro_interno = true;
  }

  // programa o relógio para gerar uma interrupção após INTERVALO_INTERRUPCAO
  if (es_escreve(self->es, D_RELOGIO_TIMER, INTERVALO_INTERRUPCAO) != ERR_OK) {
    console_printf("SO: problema na programação do timer");
    self->erro_interno = true;
  }
  
  
  return self;
}

void so_destroi(so_t *self)
{
  so_imprime_metricas(self);
  cpu_define_chamaC(self->cpu, NULL, NULL);
  escalonador_destroi(self->escalonador);
  controle_destroi_portas(self->controle_portas);
  for (int i = 0; i < self->num_processos; i++) {
    processo_destroi(self->tabela_processos[i]);
  }
  free(self->tabela_processos);
  free(self);

}


// TRATAMENTO DE INTERRUPÇÃO {{{1

// funções auxiliares para o tratamento de interrupção
static void so_salva_estado_da_cpu(so_t *self);
static void so_trata_irq(so_t *self, int irq);
static void so_trata_pendencias(so_t *self);
static void so_escalona(so_t *self);
static int so_despacha(so_t *self);
static void so_tick(so_t *self);
static void so_pendencias_bloqueio(so_t *self, processo_t *processo);
static double so_calcula_prioridade_processo(so_t *self, processo_t *processo);
static void so_calcula_mudanca_estado_processo(so_t *self, processo_t *processo);
static bool so_deve_escalonar(so_t *self);
static void so_executa_processo(so_t *self, processo_t *processo);
static void so_pendencias_leitura(so_t *self, processo_t *processo);
static void so_pendencias_escrita(so_t *self, processo_t *processo);
static void so_pendencias_espera(so_t *self, processo_t *processo);
static processo_t *so_busca_processo(so_t *self,int pid);
static void so_bloqueia_processo(so_t *self, processo_t *processo, bloqueio_motivo_t motivo,int tipo_bloqueio);
static void so_desbloqueia_processo(so_t *self, processo_t *processo);
static processo_t *so_gera_processo(so_t *self, char *programa);
static void so_mata_processo(so_t *self, processo_t *processo);
static void so_reserva_terminal_processo(so_t *self, processo_t *processo);
static int so_termina(so_t *self);
static bool so_tem_trabalho(so_t *self);
static void so_metricas(so_t *self, int delta);


// função a ser chamada pela CPU quando executa a instrução CHAMAC, no tratador de
//   interrupção em assembly
// essa é a única forma de entrada no SO depois da inicialização
// na inicialização do SO, a CPU foi programada para chamar esta função para executar
//   a instrução CHAMAC
// a instrução CHAMAC só deve ser executada pelo tratador de interrupção
//
// o primeiro argumento é um ponteiro para o SO, o segundo é a identificação
//   da interrupção
// o valor retornado por esta função é colocado no registrador A, e pode ser
//   testado pelo código que está após o CHAMAC. No tratador de interrupção em
//   assembly esse valor é usado para decidir se a CPU deve retornar da interrupção
//   (e executar o código de usuário) ou executar PARA e ficar suspensa até receber
//   outra interrupção
static int so_trata_interrupcao(void *argC, int reg_A)
{
  so_t *self = argC;
  irq_t irq = reg_A;
  self->num_vezes_interrupocoes[irq]++;
  // esse print polui bastante, recomendo tirar quando estiver com mais confiança
  //console_printf("SO: recebi IRQ %d (%s)", irq, irq_nome(irq));
  // salva o estado da cpu no descritor do processo que foi interrompido
  so_salva_estado_da_cpu(self);
  so_tick(self);
  // faz o atendimento da interrupção
  so_trata_irq(self, irq);
  // faz o processamento independente da interrupção
  so_trata_pendencias(self);
  // escolhe o próximo processo a executar
  so_escalona(self);
  // recupera o estado do processo escolhido
  if(so_tem_trabalho(self)) return so_despacha(self);
  else return so_termina(self);
 // return so_despacha(self);
}


static void so_trata_pendencias(so_t *self) {
    for (int i = 0; i < self->num_processos; i++) {
        processo_t *processo = self->tabela_processos[i];
        if (processo_estado(processo) == BLOQUEADO) {
          so_pendencias_bloqueio(self,processo);
        }
    }
}

static double so_calcula_prioridade_processo(so_t *self, processo_t *processo){
  int t_exec = self->tempo_quantum - self->tempo_restante;

  double prioridade = processo_prioridade(processo);
  prioridade += (double)t_exec / self->tempo_quantum;
  prioridade /= 2.0;
  return prioridade;
}

static void so_calcula_mudanca_estado_processo(so_t *self, processo_t *processo){
    
    if(processo_estado(processo) == EM_EXECUCAO){
        double prioridade_nova = so_calcula_prioridade_processo(self, processo);
        processo_set_prioridade(processo, prioridade_nova);
    }
}

static void so_escalona(so_t *self) {
  if(!so_deve_escalonar(self)) return;

  if(self->processo_corrente != NULL){

    double prioridade = so_calcula_prioridade_processo(self, self->processo_corrente);
    processo_set_prioridade(self->processo_corrente, prioridade);
  }

  if(self->processo_corrente != NULL && processo_estado(self->processo_corrente) == EM_EXECUCAO){
    console_printf("ADICIONANDO PROCESSO NO ESCALONADOR");
    escalonador_adiciona_processo(self->escalonador, self->processo_corrente);
  }

  processo_t *processo = escalonador_proximo(self->escalonador);

  if(processo != NULL) console_printf("SO: escalona processo %d", processo_pid(processo));
  else console_printf("SO: nenhum processo para escalonar");

  so_executa_processo(self,processo);
}


static int so_despacha(so_t *self) {
    if(self->erro_interno) return 1;
    processo_t *processo = self->processo_corrente;
    if (processo != NULL) {
        int pc = processo_PC(processo);
        int a = processo_A(processo);
        int x = processo_X(processo);
        mem_escreve(self->mem, IRQ_END_PC, pc);
        mem_escreve(self->mem, IRQ_END_A, a);
        mem_escreve(self->mem, IRQ_END_X, x);
        return 0; 
    } else {
        return 1; 
    }
}


static void so_salva_estado_da_cpu(so_t *self) {
    processo_t *processo = self->processo_corrente;
    if (processo != NULL) {
       
        int pc, a, x;
        mem_le(self->mem, IRQ_END_PC, &pc);
        mem_le(self->mem, IRQ_END_A, &a);
        mem_le(self->mem, IRQ_END_X, &x);

        processo_set_PC(processo, pc);
        processo_set_A(processo, a);
        processo_set_X(processo, x);
    }
    return;
}

static int so_termina(so_t *self)
{
  err_t e1, e2;
  e1 = es_escreve(self->es, D_RELOGIO_INTERRUPCAO, 0);
  e2 = es_escreve(self->es, D_RELOGIO_TIMER, 0);

  if (e1 != ERR_OK || e2 != ERR_OK) {
    console_printf("SO: problema de desarme do timer");
    self->erro_interno = true;
  }

  so_imprime_metricas(self);

  return 1;
}

static void so_pendencias_bloqueio(so_t *self, processo_t *processo)
{
  bloqueio_motivo_t motivo = processo_motivo_bloqueio(processo);
  //console_printf("SO: tratando pendencias de bloqueio %s",processo_bloqueio_nome(processo));

  switch (motivo)
  {
  case LEITURA:
    so_pendencias_leitura(self, processo);
    break;
  case ESCRITA:
    so_pendencias_escrita(self, processo);
    break;
  case ESPERA:
    so_pendencias_espera(self, processo);
    break;
  default:
    console_printf("SO: motivo de bloqueio invalido");
    self->erro_interno = true;
    break;
  }
}

static void so_pendencias_leitura(so_t *self, processo_t *processo) {

    so_reserva_terminal_processo(self, processo);

    int terminal_id = processo_terminal_id(processo);
    int dado;
    if (terminal_id != -1 && controle_le_porta(self->controle_portas, terminal_id, &dado)) {
        processo_set_A(processo,dado);  
        so_desbloqueia_processo(self, processo);  

        console_printf("SO: processo %d desbloqueado após leitura no terminal %d", processo_pid(processo), terminal_id);
    } else {
        console_printf("SO: terminal %d não está pronto para leitura", terminal_id);
    }
   
}

static void so_pendencias_escrita(so_t *self, processo_t *processo) {
    so_reserva_terminal_processo(self, processo);

    int terminal_id = processo_terminal_id(processo);
    int dado = processo_tipo_bloqueio(processo);  

    if (terminal_id != -1 && controle_escreve_porta(self->controle_portas, terminal_id, dado)) {
        processo_set_A(processo,0); 
        so_desbloqueia_processo(self, processo);  

        console_printf("SO: processo %d desbloqueado após escrita no terminal %d", processo_pid(processo), terminal_id);
    } else {
        console_printf("SO: terminal %d não está pronto para escrita", terminal_id);
    }
    
}

static void so_pendencias_espera(so_t *self, processo_t *processo) {
  int pid_alvo = processo_tipo_bloqueio(processo);

  processo_t *processo_alvo = so_busca_processo(self, pid_alvo);

  if (processo_alvo == NULL || processo_estado(processo_alvo) == MORTO) {
    processo_set_A(processo, 0);

    so_desbloqueia_processo(self, processo);

    console_printf("SO: processo %d - desbloqueia de espera de processo", processo_pid(processo));
  }
}




// TRATAMENTO DE UMA IRQ {{{1

// funções auxiliares para tratar cada tipo de interrupção
static void so_trata_irq_reset(so_t *self);
static void so_trata_irq_chamada_sistema(so_t *self);
static void so_trata_irq_err_cpu(so_t *self);
static void so_trata_irq_relogio(so_t *self);
static void so_trata_irq_desconhecida(so_t *self, int irq);

static void so_trata_irq(so_t *self, int irq)
{
  // verifica o tipo de interrupção que está acontecendo, e atende de acordo
  console_printf("vendo tipo de interrupcao ");
  switch (irq) {
    case IRQ_RESET:
      so_trata_irq_reset(self);
      break;
    case IRQ_SISTEMA:
      so_trata_irq_chamada_sistema(self);
      break;
    case IRQ_ERR_CPU:
      so_trata_irq_err_cpu(self);
      break;
    case IRQ_RELOGIO:
      so_trata_irq_relogio(self);
      break;
    default:
      so_trata_irq_desconhecida(self, irq);
  }
}

// interrupção gerada uma única vez, quando a CPU inicializa
static void so_trata_irq_reset(so_t *self)
{
  console_printf("interrupcao reset");
  // t1: deveria criar um processo para o init, e inicializar o estado do
  //   processador para esse processo com os registradores zerados, exceto
  //   o PC e o modo.
  // como não tem suporte a processos, está carregando os valores dos
  //   registradores diretamente para a memória, de onde a CPU vai carregar
  //   para os seus registradores quando executar a instrução RETI
  processo_t *processo = so_gera_processo(self, "init.maq");

  if (processo_PC(processo) != 100) {
    console_printf("SO: problema na carga do programa inicial");
    self->erro_interno = true;
    return;
  }

  mem_escreve(self->mem, IRQ_END_modo, usuario);
}

// interrupção gerada quando a CPU identifica um erro
static void so_trata_irq_err_cpu(so_t *self)
{
  // Ocorreu um erro interno na CPU
  // O erro está codificado em IRQ_END_erro
  // Em geral, causa a morte do processo que causou o erro
  // Ainda não temos processos, causa a parada da CPU
  int err_int;
  // t1: com suporte a processos, deveria pegar o valor do registrador erro
  //   no descritor do processo corrente, e reagir de acordo com esse erro
  //   (em geral, matando o processo)
  mem_le(self->mem, IRQ_END_erro, &err_int);
  err_t err = err_int;
  console_printf("SO: IRQ não tratada -- erro na CPU: %s", err_nome(err));
  self->erro_interno = true;
}

// interrupção gerada quando o timer expira
static void so_trata_irq_relogio(so_t *self)
{
  // rearma o interruptor do relógio e reinicializa o timer para a próxima interrupção
  err_t e1, e2;
  e1 = es_escreve(self->es, D_RELOGIO_INTERRUPCAO, 0); // desliga o sinalizador de interrupção
  e2 = es_escreve(self->es, D_RELOGIO_TIMER, INTERVALO_INTERRUPCAO);
  if (e1 != ERR_OK || e2 != ERR_OK) {
    console_printf("SO: problema da reinicialização do timer");
    self->erro_interno = true;
  }
  // t1: deveria tratar a interrupção
  //   por exemplo, decrementa o quantum do processo corrente, quando se tem
  //   um escalonador com quantum
  console_printf("SO: interrupção do relógio (não tratada)");
  if (self->tempo_restante > 0) {
    self->tempo_restante--;
  }
}

// foi gerada uma interrupção para a qual o SO não está preparado
static void so_trata_irq_desconhecida(so_t *self, int irq)
{
  console_printf("SO: não sei tratar IRQ %d (%s)", irq, irq_nome(irq));
  self->erro_interno = true;
}

// CHAMADAS DE SISTEMA {{{1

// funções auxiliares para cada chamada de sistema
static void so_chamada_le(so_t *self);
static void so_chamada_escr(so_t *self);
static void so_chamada_cria_proc(so_t *self);
static void so_chamada_mata_proc(so_t *self);
static void so_chamada_espera_proc(so_t *self);




static void so_trata_irq_chamada_sistema(so_t *self)
{
  // a identificação da chamada está no registrador A
  // t1: com processos, o reg A tá no descritor do processo corrente
  int id_chamada;
  if (mem_le(self->mem, IRQ_END_A, &id_chamada) != ERR_OK) {
    console_printf("SO: erro no acesso ao id da chamada de sistema");
    self->erro_interno = true;
    return;
  }
  console_printf("SO: chamada de sistema %d", id_chamada);
  switch (id_chamada) {
    case SO_LE:
      so_chamada_le(self);
      break;
    case SO_ESCR:
      so_chamada_escr(self);
      break;
    case SO_CRIA_PROC:
      so_chamada_cria_proc(self);
      break;
    case SO_MATA_PROC:
      so_chamada_mata_proc(self);
      break;
    case SO_ESPERA_PROC:
      so_chamada_espera_proc(self);
      break;
    default:
      console_printf("SO: chamada de sistema desconhecida (%d)", id_chamada);
      self->erro_interno = true;
  }
}

// implementação da chamada se sistema SO_LE
// faz a leitura de um dado da entrada corrente do processo, coloca o dado no reg A
static void so_chamada_le(so_t *self)
{
  // implementação com espera ocupada
  //   T1: deveria realizar a leitura somente se a entrada estiver disponível,
  //     senão, deveria bloquear o processo.
  //   no caso de bloqueio do processo, a leitura (e desbloqueio) deverá
  //     ser feita mais tarde, em tratamentos pendentes em outra interrupção,
  //     ou diretamente em uma interrupção específica do dispositivo, se for
  //     o caso
  // implementação lendo direto do terminal A
  //   T1: deveria usar dispositivo de entrada corrente do processo
  processo_t *processo = self->processo_corrente;
  
  if(processo == NULL) return;

  so_reserva_terminal_processo(self, processo);
  int terminal_id = processo_terminal_id(processo);
  int dado;
  if (terminal_id != -1 && controle_le_porta(self->controle_portas, terminal_id, &dado)) {
      processo_set_A(processo,dado);  
      console_printf("Dado %d vindo do processo %d",dado,processo_pid(processo));
  } else {
      console_printf("SO: terminal %d não está pronto para leitura", terminal_id);
      so_bloqueia_processo(self,processo,LEITURA,0);
  }
  
}

// implementação da chamada se sistema SO_ESCR
// escreve o valor do reg X na saída corrente do processo
static void so_chamada_escr(so_t *self)
{
  
  processo_t *processo = self->processo_corrente;

  if(processo == NULL) return;

  so_reserva_terminal_processo(self, processo);
  int terminal_id = processo_terminal_id(processo);
  int dado = processo_X(processo); 

  if (terminal_id != -1 && controle_escreve_porta(self->controle_portas, terminal_id, dado)) {
      processo_set_A(processo,0);  

  } else {
      console_printf("SO: terminal %d não está pronto para escrita", terminal_id);
      so_bloqueia_processo(self,processo,ESCRITA,dado);
  }
  
}

// implementação da chamada se sistema SO_CRIA_PROC
// cria um processo
static void so_chamada_cria_proc(so_t *self)
{
  // ainda sem suporte a processos, carrega programa e passa a executar ele
  // quem chamou o sistema não vai mais ser executado, coitado!
  // T1: deveria criar um novo processo
  //ver qual seria o id do sistema
  processo_t *processo = self->processo_corrente;

  if(processo == NULL) return;
  int ender_proc = processo_X(processo);
  char nome[100];
  if (copia_str_da_mem(100, nome, self->mem, ender_proc)) {
    processo_t *processo_alvo = so_gera_processo(self, nome);
    if(processo_alvo != NULL)
    {
      // t1: deveria escrever no PC do descritor do processo criado
      processo_set_A(processo,processo_pid(processo_alvo));
      return;
    } // else?
  }
 
  // deveria escrever -1 (se erro) ou o PID do processo criado (se OK) no reg A
  //   do processo que pediu a criação
  processo_set_A(processo,-1);
}



// implementação da chamada se sistema SO_MATA_PROC
// mata o processo com pid X (ou o processo corrente se X é 0)
static void so_chamada_mata_proc(so_t *self)
{
  // T1: deveria matar um processo
  // ainda sem suporte a processos, retorna erro -1
  processo_t *processo = self->processo_corrente;

  if(processo == NULL) return;

  int pid_alvo = processo_X(processo);
  processo_t *processo_alvo = so_busca_processo(self,pid_alvo);

  if(pid_alvo == 0){
    processo_alvo = self->processo_corrente;
  }

  if(processo_alvo != NULL){
    so_mata_processo(self,processo_alvo);
    processo_set_A(processo,0);
  }else{
    processo_set_A(processo,-1);
  }


}

// implementação da chamada se sistema SO_ESPERA_PROC
// espera o fim do processo com pid X
static void so_chamada_espera_proc(so_t *self)
{
  processo_t *processo = self->processo_corrente;

  if(processo == NULL) return;

  int pid_alvo = processo_X(processo);
  processo_t *processo_alvo = so_busca_processo(self,pid_alvo);

  if(processo_alvo == NULL || processo_alvo == processo){
    processo_set_A(processo,-1);
    return;
  }

  if(processo_estado(processo_alvo) == MORTO){
    processo_set_A(processo,0);
  }else{
    so_bloqueia_processo(self,processo,ESPERA,pid_alvo);
  }
}


static void so_bloqueia_processo(so_t *self, processo_t *processo, bloqueio_motivo_t motivo,int tipo_bloqueio){
  escalonador_remove_processo(self->escalonador, processo);
  so_calcula_mudanca_estado_processo(self,processo);
  processo_bloqueia(processo, motivo, tipo_bloqueio);
}

static void so_desbloqueia_processo(so_t *self, processo_t *processo){
  so_calcula_mudanca_estado_processo(self,processo);
  processo_desbloqueia(processo);
  console_printf("ADICIONANDO PROCESSO NO ESCALONADOR");
  escalonador_adiciona_processo(self->escalonador,processo);
}

static void so_reserva_terminal_processo(so_t *self, processo_t *processo)
{
  if (processo_terminal_id(processo) != -1) {
    return;
  }

  int terminal_id = controle_porta_disponivel(self->controle_portas);
  processo_set_terminal_id(processo, terminal_id);
  controle_reserva_porta(self->controle_portas, terminal_id);
}

static processo_t *so_busca_processo(so_t *self,int pid){
  if (pid <= 0 || pid > self->num_processos) {
    return NULL;
  }

  return self->tabela_processos[pid - 1];
}

static processo_t *so_gera_processo(so_t *self, char *programa) {

    int end = so_carrega_programa(self, programa);

    if (end <= 0) {
        return NULL;
    }

    if (self->num_processos == self->tamanho_tabela_processos) {
      self->tamanho_tabela_processos = self->tamanho_tabela_processos *2;
      self->tabela_processos = realloc(self->tabela_processos, self->tamanho_tabela_processos * sizeof(*self->tabela_processos));
      if(self->tabela_processos == NULL){
        return NULL;
      }
    }

    processo_t *processo = processo_cria(self->num_processos + 1, end);
    self->tabela_processos[self->num_processos++] = processo;
    escalonador_adiciona_processo(self->escalonador, processo);
    self->num_processos_criados++;
    return processo;
}


static void so_mata_processo(so_t *self, processo_t *processo) {
    int terminal_id = processo_terminal_id(processo);

    if (terminal_id != -1) {
        processo_libera_terminal(processo);
        controle_libera_porta(self->controle_portas,terminal_id);
    }

    escalonador_remove_processo(self->escalonador, processo);
    processo_encerra(processo);
}

static void so_executa_processo(so_t *self, processo_t *processo) {


    if (self->processo_corrente != NULL && self->processo_corrente != processo && processo_estado(self->processo_corrente) == EM_EXECUCAO) {
        console_printf("Parando processo.\n");
        processo_para(self->processo_corrente);
        so_calcula_mudanca_estado_processo(self, self->processo_corrente);
        self->num_total_preempcoes++;
    }

    if (processo != NULL && processo_estado(processo) != EM_EXECUCAO) {
        console_printf("Executando processo.\n");
        processo_executa(processo);
    }

  
    if( processo != NULL){
      escalonador_remove_processo(self->escalonador, processo);
      console_printf("Processo removido do escalonador.\n");
    }

    self->processo_corrente = processo;
    self->tempo_restante = QUANTUM;
}


static bool so_deve_escalonar(so_t *self){
  if (self->processo_corrente == NULL || processo_estado(self->processo_corrente) != EM_EXECUCAO || self->tempo_restante <= 0) {
    return true;
  }
  return false;
}

static void so_tick(so_t *self){
  int tempo_relogio_anterior = self->tempo_relogio_atual;

  if (es_le(self->es, D_RELOGIO_INSTRUCOES, &self->tempo_relogio_atual) != ERR_OK) {
      console_printf("SO: erro ao ler o relógio");
      return;
  }

  if (tempo_relogio_anterior == -1) {
    return;
  }

  int delta = self->tempo_relogio_atual - tempo_relogio_anterior;

  so_metricas(self, delta);
}

static bool so_tem_trabalho(so_t *self)
{
  for (int i = 0; i < self->num_processos; i++) {
    if (processo_estado(self->tabela_processos[i]) != MORTO) {
      return true;
    }
  }

  return false;
}

static void so_metricas(so_t *self, int delta){
  self->tempo_total_exec += delta;
  if(self->processo_corrente == NULL){
    self->tempo_total_ocioso += delta;
  }
  for (int i = 0; i < self->num_processos; i++) {
    processo_metricas(self->tabela_processos[i],delta);
  }
}

const char* tipo_escalonador_nome() {
    switch (TIPO_ESCALONADOR) {
        case SIMPLES:     return "simples";
        case CIRCULAR:     return "circular";
        case PRIORIDADE:      return "prioridade";
        default:          return "desconhecido";
    }
}
 

static void so_imprime_metricas(so_t *self){
  char nome_arquivo[100];
  sprintf(nome_arquivo, "metricas_%s.txt", tipo_escalonador_nome());

  FILE *file = fopen(nome_arquivo, "w");
  if (file == NULL) {
      perror("Erro ao abrir o arquivo");
      return;
  }

  fprintf(file, "Número de processos criados: %d\n", self->num_processos_criados);
  fprintf(file, "Tempo total de execução: %.2f\n", self->tempo_total_exec);
  fprintf(file, "Tempo total ocioso: %.2f\n", self->tempo_total_ocioso);
  fprintf(file, "Número total de preempções: %d\n", self->num_total_preempcoes);
  fprintf(file, "Número de vezes de interrupções por IRQ:\n");
  for (int i = 0; i < N_IRQ; i++) {
      fprintf(file, "  IRQ %d: %d vezes\n", i, self->num_vezes_interrupocoes[i]);
  }

  for (int i = 0; i < self->num_processos; i++) {
    processo_t *processo = self->tabela_processos[i];
    if (processo == NULL) continue; 

    fprintf(file, "Processo ID (PID): %d\n", processo_pid(processo));
    fprintf(file, "Estado: %s\n", processo_estado_nome(processo)); 
    fprintf(file, "Prioridade: %.2f\n", processo_prioridade(processo));
    fprintf(file, "Motivo do bloqueio: %s\n", processo_bloqueio_nome(processo)); 
    fprintf(file, "Tipo de bloqueio: %d\n", processo_tipo_bloqueio(processo));
    fprintf(file, "Terminal ID: %d\n", processo_terminal_id(processo));
    fprintf(file, "Registradores: PC=%d, X=%d, A=%d\n", processo_PC(processo), processo_X(processo), processo_A(processo));

    fprintf(file, "Tempo de retorno: %.2f\n", processo_tempo_retorno(processo));
    fprintf(file, "Número de preempções: %d\n", processo_num_preempcoes(processo));

    fprintf(file, "Número de vezes em estados:\n");
    for (int j = 0; j < 4; j++) {
        fprintf(file, "  Estado %d: %d vezes\n", j, processo_num_vezes_estado(processo,j));
    }

    fprintf(file, "Tempo total em estados:\n");
    for (int j = 0; j < 4; j++) {
        fprintf(file, "  Estado %d: %d ms\n", j, processo_tempo_estado(processo,j));
    }

    fprintf(file, "Tempo médio de resposta: %d ms\n", processo_tempo_medio_resposta(processo));
    fprintf(file, "---------------------------------\n");
  }
  fclose(file);
  console_printf("metricas impresas no arquivo");

}

// CARGA DE PROGRAMA {{{1

// carrega o programa na memória
// retorna o endereço de carga ou -1
static int so_carrega_programa(so_t *self, char *nome_do_executavel)
{
  // programa para executar na nossa CPU
  programa_t *prog = prog_cria(nome_do_executavel);
  if (prog == NULL) {
    console_printf("Erro na leitura do programa '%s'\n", nome_do_executavel);
    return -1;
  }

  int end_ini = prog_end_carga(prog);
  int end_fim = end_ini + prog_tamanho(prog);

  for (int end = end_ini; end < end_fim; end++) {
    if (mem_escreve(self->mem, end, prog_dado(prog, end)) != ERR_OK) {
      console_printf("Erro na carga da memória, endereco %d\n", end);
      return -1;
    }
  }

  prog_destroi(prog);
  console_printf("SO: carga de '%s' em %d-%d", nome_do_executavel, end_ini, end_fim);
  return end_ini;
}

// ACESSO À MEMÓRIA DOS PROCESSOS {{{1

// copia uma string da memória do simulador para o vetor str.
// retorna false se erro (string maior que vetor, valor não char na memória,
//   erro de acesso à memória)
// T1: deveria verificar se a memória pertence ao processo
static bool copia_str_da_mem(int tam, char str[tam], mem_t *mem, int ender)
{
  for (int indice_str = 0; indice_str < tam; indice_str++) {
    int caractere;
    if (mem_le(mem, ender + indice_str, &caractere) != ERR_OK) {
      return false;
    }
    if (caractere < 0 || caractere > 255) {
      return false;
    }
    str[indice_str] = caractere;
    if (caractere == 0) {
      return true;
    }
  }
  // estourou o tamanho de str
  return false;
}


// vim: foldmethod=marker
