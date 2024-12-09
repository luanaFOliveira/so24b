// so.c
// sistema operacional
// simulador de computador
// so24b

// INCLUDES {{{1
#include "so.h"
#include "dispositivos.h"
#include "irq.h"
#include "programa.h"
#include "tabpag.h"
#include "instrucao.h"
#include "escalonador.h"
#include "processo.h"
#include "controle_es.h"
#include "controle_quadros.h"
#include "memoria.h"
#include "controle_blocos.h"
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
// CONSTANTES E TIPOS {{{1
// intervalo entre interrupções do relógio
#define INTERVALO_INTERRUPCAO 100   // em instruções executadas
#define MAX_PROCESSOS 16 // numero máximo de processos
#define QUANTUM 5
#define NUM_TERMINAIS 4
#define TIPO_ESCALONADOR CIRCULAR
#define ALGORITMO_FIFO 0
#define ALGORITMO_SEGUNDA_CHANCE 1
#define TOTAL_QUADROS 1024 // Número total de quadros na memória física
#define TAMANHO_MEM_SECUNDARIA 10000
#define TEMPO_TRANSFERENCIA 1

// Não tem processos nem memória virtual, mas é preciso usar a paginação,
//   pelo menos para implementar relocação, já que os programas estão sendo
//   todos montados para serem executados no endereço 0 e o endereço 0
//   físico é usado pelo hardware nas interrupções.
// Os programas estão sendo carregados no início de um quadro, e usam quantos
//   quadros forem necessárias. Para isso a variável quadro_livre contém
//   o número do primeiro quadro da memória principal que ainda não foi usado.
//   Na carga do processo, a tabela de páginas (deveria ter uma por processo,
//   mas não tem processo) é alterada para que o endereço virtual 0 resulte
//   no quadro onde o programa foi carregado. Com isso, o programa carregadocesso ao qual elas se referem. Para isso, precisa de u
//   é acessível, mas o acesso ao anterior é perdido.

// t2: a interface de algumas funções que manipulam memória teve que ser alterada,
//   para incluir o prom
//   tipo para o processo. Neste código, não tem processos implementados, e não
//   tem um tipo para isso. Chutei o tipo int. Foi necessário também um valor para
//   representar a inexistência de um processo, coloquei -1. Altere para o seu
//   tipo, ou substitua os usos de processo_t e NENHUM_PROCESSO para o seu tipo.


//@TODO= ver como criar e definir o tamanho da memoria secundaria
//@TODO= ver com o benhur sobre a funcao so_copia_str_do_processo que nao esta sendo chamada, onde chamar, no mesmo lugar de copia_str..
//o procesos q manda eh o processo que chama
//@TODO= ver com o benhur qual unidade de medida do tempo de transferencia- tipo 10 - quantos tick de relogio - ver q horas sao pelo relogio e ver o tempo - marca la processo ta bloqueado por troca de pagina ate tal hora e na pendencia - outro tipo de bloqueia  - transfere os dados tudo na hora mas bloqueia o processo e manda ele esperar 
//@TODO=  oq seria esse tempo de agora? "atualiza-se esse tempo para "agora" mais o tempo de espera; "
struct so_t {
  cpu_t *cpu;
  mem_t *mem;
  mmu_t *mmu;
  es_t *es;
  console_t *console;
  bool erro_interno;
  // t1: tabela de processos, processo corrente, pendências, etc
  processo_t **tabela_processos;  
  int num_processos, tamanho_tabela_processos;  

  processo_t *processo_corrente;  

  escalonador_t *escalonador;

  controle_es_t *controle_es;

  int tempo_quantum, tempo_restante, tempo_relogio_atual;

  //metricas
  int num_processos_criados;
  float tempo_total_exec;
  float tempo_total_ocioso;
  int num_total_preempcoes;
  int num_vezes_interrupocoes[N_IRQ];

  // primeiro quadro da memória que está livre (quadros anteriores estão ocupados)
  // t2: com memória virtual, o controle de memória livre e ocupada é mais
  //     completo que isso
  controle_quadros_t *controle_quadros;
  // uma tabela de páginas para poder usar a MMU
  // t2: com processos, não tem esta tabela global, tem que ter uma para
  //     cada processo
  //tabpag_t *tabpag_global;
  int algoritmo_substituicao; // Para armazenar o algoritmo escolhido
  int fila_quadros[TOTAL_QUADROS]; // Gerenciamento FIFO ou Segunda Chance
  int pos_fila_inicio;            // Início da fila circular
  int pos_fila_fim;               // Fim da fila circular
  int bits_acesso[TOTAL_QUADROS]; // Para Segunda Chance

  mem_t *mem_secundaria;
  int ponteiro_disco;
  controle_blocos_t *controle_blocos;
  int num_pag_fisica;
};


// função de tratamento de interrupção (entrada no SO)
static int so_trata_interrupcao(void *argC, int reg_A);

// funções auxiliares
// no t2, foi adicionado o 'processo' aos argumentos dessas funções 
// carrega o programa na memória virtual de um processo; retorna end. inicial
static int so_carrega_programa(so_t *self, processo_t *processo,
                               char *nome_do_executavel);
// copia para str da memória do processo, até copiar um 0 (retorna true) ou tam bytes
static bool so_copia_str_do_processo(so_t *self, int tam, char str[tam],
                                     int end_virt, processo_t *processo);
static void so_imprime_metricas(so_t *self);

// CRIAÇÃO {{{1


so_t *so_cria(cpu_t *cpu, mem_t *mem, mmu_t *mmu,
              es_t *es, console_t *console)
{
  so_t *self = malloc(sizeof(*self));
  assert(self != NULL);

  self->cpu = cpu;
  self->mem = mem;
  self->mem_secundaria = mem_cria(TAMANHO_MEM_SECUNDARIA);
  self->mmu = mmu;
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

  self->pos_fila_inicio = 0;
  self->pos_fila_fim = 0;
  self->algoritmo_substituicao = ALGORITMO_FIFO; // Escolha padrão
  memset(self->bits_acesso, 0, sizeof(self->bits_acesso));

  self->controle_es = cria_controle_es(es);
  controle_registra_dispositivo(self->controle_es, D_TERM_A_TECLADO, D_TERM_A_TECLADO_OK, D_TERM_A_TELA, D_TERM_A_TELA_OK);
  controle_registra_dispositivo(self->controle_es, D_TERM_B_TECLADO, D_TERM_B_TECLADO_OK, D_TERM_B_TELA, D_TERM_B_TELA_OK);
  controle_registra_dispositivo(self->controle_es, D_TERM_C_TECLADO, D_TERM_C_TECLADO_OK, D_TERM_C_TELA, D_TERM_C_TELA_OK);
  controle_registra_dispositivo(self->controle_es, D_TERM_D_TECLADO, D_TERM_D_TECLADO_OK, D_TERM_D_TELA, D_TERM_D_TELA_OK);

  
  self->controle_quadros = controle_quadros_cria(TOTAL_QUADROS);

  self->ponteiro_disco = 0;
  self->num_pag_fisica = mem_tam(self->mem)/TAM_PAGINA;
  self->controle_blocos = cria_controle_blocos(self->num_pag_fisica);

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
//no lugar de nenhum processo = NULL?

  int ender = so_carrega_programa(self, NULL, "trata_int.maq");
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
  escalonador_destroi(self->escalonador, self->mmu, self->controle_quadros);
  destroi_controle_es(self->controle_es);
  for (int i = 0; i < self->num_processos; i++) {
    processo_destroi(self->tabela_processos[i], self->mmu, self->controle_quadros);

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
static void so_pendencias_espera_pagina(so_t *self, processo_t *processo);
static processo_t *so_busca_processo(so_t *self,int pid);
static void so_bloqueia_processo(so_t *self, processo_t *processo, bloqueio_motivo_t motivo,int tipo_bloqueio);
static void so_desbloqueia_processo(so_t *self, processo_t *processo);
static processo_t *so_gera_processo(so_t *self, char *programa);
static void so_mata_processo(so_t *self, processo_t *processo);
static void so_reserva_es_processo(so_t *self, processo_t *processo);
static int so_termina(so_t *self);
static bool so_tem_trabalho(so_t *self);
static void so_metricas(so_t *self, int delta);
static void so_trata_page_fault(so_t *self);

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
  // esse print polui bastante, recomendo tirar quando estiver com mais confiança
  // salva o estado da cpu no descritor do processo que foi interrompido
  console_printf("SO: antes de trata_irq, irq=%d", irq);
  so_salva_estado_da_cpu(self);
  so_tick(self);
  // faz o atendimento da interrupção
  so_trata_irq(self, irq);
  // faz o processamento independente da interrupção
  so_trata_pendencias(self);
  // escolhe o próximo processo a executar
  so_escalona(self);
  console_printf("SO: voltou do so_escalona, proximo coisa eh ver se tem trabalho e despachar");
  // recupera o estado do processo escolhido

  if(so_tem_trabalho(self)) return so_despacha(self);
  else return so_termina(self);
}

static void so_salva_estado_da_cpu(so_t *self)
{
  console_printf("SO:dentro de salva estado da cpu");
  processo_t *processo = self->processo_corrente;
  if (processo != NULL) {
    int pc, a, x, complemento, erro;

    mem_le(self->mem, IRQ_END_PC, &pc);
    mem_le(self->mem, IRQ_END_A, &a);
    mem_le(self->mem, IRQ_END_X, &x);
    mem_le(self->mem, IRQ_END_complemento, &complemento);
    mem_le(self->mem, IRQ_END_erro, &erro);

    processo_set_PC(processo, pc);
    processo_set_A(processo, a);
    processo_set_X(processo, x);
    processo_set_complemento(processo, complemento);
    processo_set_erro(processo, erro);

  }
  return;
}

static void so_trata_pendencias(so_t *self)
{
  // t1: realiza ações que não são diretamente ligadas com a interrupção que
  //   está sendo atendida:
  // - E/S pendente
  // - desbloqueio de processos
  // - contabilidades
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

static void so_escalona(so_t *self)
{
 
  console_printf("SO: entrou em escalonar");
  if(!so_deve_escalonar(self)) return;

  if(self->processo_corrente != NULL){

    double prioridade = so_calcula_prioridade_processo(self, self->processo_corrente);
    processo_set_prioridade(self->processo_corrente, prioridade);
  }

  if(self->processo_corrente != NULL && processo_estado(self->processo_corrente) == EM_EXECUCAO){
    escalonador_adiciona_processo(self->escalonador, self->processo_corrente);
    //console_printf("SO: escalonador adicionou processo, %d", processo_pid(self->processo_corrente));
  }

  processo_t *processo = escalonador_proximo(self->escalonador);
  if(processo != NULL);
  else console_printf("SO: nenhum processo para escalonar");

  //console_printf("SO: escalonador escolheu proximo processo, %d", processo_pid(processo));
  so_executa_processo(self,processo);
  //console_printf("SO: voltou do executa processo");
}


static int so_despacha(so_t *self)
{
  console_printf("SO: dentro de so despacha");
    mem_escreve(self->mem, IRQ_END_erro, ERR_OK);

  if(self->erro_interno) return 1;

  processo_t *processo = self->processo_corrente;

  if (processo != NULL) {

    console_printf("SO: dentro de so despacha - processo nao eh nullo, %d", processo_pid(processo));
    int pc = processo_PC(processo);
    int a = processo_A(processo);
    int x = processo_X(processo);
    int complemento = processo_complemento(processo);
    
    mem_escreve(self->mem, IRQ_END_PC, pc);
    mem_escreve(self->mem, IRQ_END_A, a);
    mem_escreve(self->mem, IRQ_END_X, x);
    mem_escreve(self->mem, IRQ_END_complemento, complemento);
    console_printf("SO: dentro de so despacha - escreveu na memoria em PC- %d, A-%d, X-%d, complemento-%d", pc, a, x, complemento);

     // Configura a tabela de páginas da MMU
    tabpag_t *tab_pag = processo_tab_pag(processo); // Obtenha a tabela de páginas do processo
    mmu_define_tabpag(self->mmu, tab_pag); // Configura a MMU para usar a tabela do processo

    int q;
    int err = tabpag_traduz(tab_pag, pc/TAM_PAGINA, &q);
    console_printf("Espero ler instrução do quadro físico %d, traduzido da página virtual %d", q, pc/TAM_PAGINA);
    console_printf("Erro foi: %d. ERR_OK é %d", err, ERR_OK);
    console_printf("Processo = #%d", processo_pid(processo));
     mem_escreve(self->mem, IRQ_END_erro, ERR_OK);
    return 0; 
  } else {
    return 1; 
  }
}

static int so_termina(so_t *self)
{
  //console_printf("SO: dentro de so termina");
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
  console_printf("SO: tratando pendencias de bloqueio %s",processo_bloqueio_nome(processo));

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
  case ESPERA_PAGINA:
    so_pendencias_espera_pagina(self, processo);
    break;
  default:
    console_printf("SO: motivo de bloqueio invalido");
    self->erro_interno = true;
    break;
  }
}

static void so_pendencias_leitura(so_t *self, processo_t *processo) {
    console_printf("SO: pendencias leitura");
    so_reserva_es_processo(self, processo);

    int es_id = processo_es_id(processo);
    int dado;
    if (es_id != -1 && controle_le_dispositivo(self->controle_es, es_id, &dado)) {
        processo_set_A(processo,dado);  
        so_desbloqueia_processo(self, processo);  

        console_printf("SO: processo %d desbloqueado após leitura no terminal %d", processo_pid(processo), es_id);
    } else {
        console_printf("SO: terminal %d não está pronto para leitura", es_id);
    }
   
}

static void so_pendencias_escrita(so_t *self, processo_t *processo) {
    console_printf("SO: pendencias escrita");
    so_reserva_es_processo(self, processo);

    int es_id = processo_es_id(processo);
    int dado = processo_tipo_bloqueio(processo);  

    if (es_id != -1 && controle_escreve_dispositivo(self->controle_es, es_id, dado)) {
        processo_set_A(processo,0); 
        so_desbloqueia_processo(self, processo);  

        console_printf("SO: processo %d desbloqueado após escrita no terminal %d", processo_pid(processo), es_id);
    } else {
        console_printf("SO: terminal %d não está pronto para escrita", es_id);
    }
    
}

static void so_pendencias_espera(so_t *self, processo_t *processo) {
  console_printf("SO: pendencias espera");
  int pid_alvo = processo_tipo_bloqueio(processo);

  processo_t *processo_alvo = so_busca_processo(self, pid_alvo);

  if (processo_alvo == NULL || processo_estado(processo_alvo) == MORTO) {
    processo_set_A(processo, 0);

    so_desbloqueia_processo(self, processo);

    console_printf("SO: processo %d - desbloqueia de espera de processo", processo_pid(processo));
  }
}

static void so_pendencias_espera_pagina(so_t *self, processo_t *processo) {
  console_printf("SO: verificando pendências de espera por página para o processo %d\n", 
      processo_pid(processo));

  if(processo_tipo_bloqueio(processo) == 0){
    so_desbloqueia_processo(self, processo);
    return;
  }
  processo_set_tipo_bloqueio(processo, processo_tipo_bloqueio(processo) -1);
  // int tempo_bloqueio = processo_tipo_bloqueio(processo);

  // if (self->tempo_relogio_atual >= tempo_bloqueio + TEMPO_TRANSFERENCIA) {
  //     console_printf("SO: desbloqueando processo %d\n", processo_pid(processo));
  //     processo_set_A(processo, 0); // Atualiza o estado interno do processo, se necessário
  //     so_desbloqueia_processo(self, processo);
  // }
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
  console_printf("SO: entrou em trata irq irq=%d", irq);
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
  // t1: deveria criar um processo para o init, e inicializar o estado do
  //   processador para esse processo com os registradores zerados, exceto
  //   o PC e o modo.
  // como não tem suporte a processos, está carregando os valores dos
  //   registradores diretamente para a memória, de onde a CPU vai carregar
  //   para os seus registradores quando executar a instrução RETI

  // coloca o programa "init" na memória
  // t2: deveria criar um processo, e programar a tabela de páginas dele
  console_printf("SO: dentro de irq reset antes de gerar processo");
  processo_t *processo = so_gera_processo(self, "init.maq");
  int ender = so_carrega_programa(self, processo, "init.maq");
  if (ender != 0) {
    console_printf("SO: problema na carga do programa inicial");
    self->erro_interno = true;
    return;
  }

  self->processo_corrente = processo;

  // altera o PC para o endereço de carga (deve ter sido o endereço virtual 0)
  mem_escreve(self->mem, IRQ_END_PC, processo_PC(self->processo_corrente));
  // passa o processador para modo usuário
  mem_escreve(self->mem, IRQ_END_modo, usuario);
}

// interrupção gerada quando a CPU identifica um erro
static void so_trata_irq_err_cpu(so_t *self)
{
  console_printf("SO: tratando erro na CPU");
  err_t erro = processo_erro(self->processo_corrente);
  if(erro == ERR_PAG_AUSENTE){
    console_printf("SO: tratando falha de pagina vindo da cpu");
    so_trata_page_fault(self);
    return;
  }

   if(erro == ERR_INSTR_INV)
  {
    console_printf("SO: caguei");
    int v;
    mmu_le(self->mmu, 0, &v, usuario);
    console_printf("SO: %d", v);

  }

  console_printf("SO: IRQ não tratada -- erro na CPU: %s", err_nome(erro));
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
  if (self->tempo_restante > 0) {
    self->tempo_restante--;
  }
  console_printf("SO: interrupção do relógio (não tratada)");
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
  
  processo_t *processo = self->processo_corrente;
  
  if(processo == NULL) return;

  so_reserva_es_processo(self, processo);
  int es_id = processo_es_id(processo);
  int dado;
  if (es_id != -1 && controle_le_dispositivo(self->controle_es, es_id, &dado)) {
      processo_set_A(processo,dado);  
      console_printf("Dado %d vindo do processo %d",dado,processo_pid(processo));
  } else {
      console_printf("SO: terminal %d não está pronto para leitura", es_id);
      so_bloqueia_processo(self,processo,LEITURA,0);
  }
 
}

// implementação da chamada se sistema SO_ESCR
// escreve o valor do reg X na saída corrente do processo
static void so_chamada_escr(so_t *self)
{
  
  processo_t *processo = self->processo_corrente;

  if(processo == NULL) return;

  so_reserva_es_processo(self, processo);
  int es_id = processo_es_id(processo);
  int dado = processo_X(processo); 

  if (es_id != -1 && controle_escreve_dispositivo(self->controle_es, es_id, dado)) {
      processo_set_A(processo,0);  

  } else {
      console_printf("SO: terminal %d não está pronto para escrita", es_id);
      so_bloqueia_processo(self,processo,ESCRITA,dado);
  }
}

// implementação da chamada se sistema SO_CRIA_PROC
// cria um processo
static void so_chamada_cria_proc(so_t *self)
{
  
  processo_t *processo = self->processo_corrente;
  console_printf("SO: chamada cria processo %d", processo_pid(processo));

  if(processo == NULL) return;
  int ender_proc = processo_X(processo);
  char nome[100];
  if (so_copia_str_do_processo(self, 100, nome, ender_proc, processo)) {
    console_printf("SO: endereço do programa %d", ender_proc);
    console_printf("SO: carreguei %s", nome);
    processo_t *processo_alvo = so_gera_processo(self, nome);
    console_printf("Processo alvo %d criado.", processo_pid(processo_alvo));
    if(processo_alvo != NULL)
    {
      processo_set_A(processo,processo_pid(processo_alvo));
      return;
    } 
  }
 
  processo_set_A(processo,-1);
}

// implementação da chamada se sistema SO_MATA_PROC
// mata o processo com pid X (ou o processo corrente se X é 0)
static void so_chamada_mata_proc(so_t *self)
{
  
  processo_t *processo = self->processo_corrente;

  if(processo == NULL) return;

  int pid_alvo = processo_X(processo);
  processo_t *processo_alvo = so_busca_processo(self,pid_alvo);

  if(pid_alvo == 0){
    processo_alvo = self->processo_corrente;
  }

  if(processo_alvo != NULL){
    console_printf("SO: so matando processo %d", processo_pid(processo_alvo));
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
  // T1: deveria bloquear o processo se for o caso (e desbloquear na morte do esperado)
  // ainda sem suporte a processos, retorna erro -1
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
  //console_printf("SO: escalonador removeu processo por bloqueio, %d", processo_pid(processo));
  escalonador_remove_processo(self->escalonador, processo);
  so_calcula_mudanca_estado_processo(self,processo);
  processo_bloqueia(processo, motivo, tipo_bloqueio);
}

static void so_desbloqueia_processo(so_t *self, processo_t *processo){
  so_calcula_mudanca_estado_processo(self,processo);
  processo_desbloqueia(processo);
  escalonador_adiciona_processo(self->escalonador,processo);
}

static void so_reserva_es_processo(so_t *self, processo_t *processo)
{
  if (processo_es_id(processo) != -1) {
    return;
  }

  int es_id = controle_dispositivo_disponivel(self->controle_es);
  processo_set_es_id(processo, es_id);
  controle_reserva_dispositivo(self->controle_es, es_id);
}

static processo_t *so_busca_processo(so_t *self,int pid){
  if (pid <= 0 || pid > self->num_processos) {
    return NULL;
  }

  return self->tabela_processos[pid - 1];
}

//@TODO = ver como alterar isso agora que so_carrega_programa recebe um processo, mas aqui o processo nao esta gerado ainda
static processo_t *so_gera_processo(so_t *self, char *programa) {
    console_printf("gerando processos");

    
    processo_t *processo = processo_cria(self->num_processos + 1);
    int end = so_carrega_programa(self, processo ,programa);
    processo_set_pc(processo, end);


    if (end < 0) {
        return NULL;
    }

    if (self->num_processos == self->tamanho_tabela_processos) {
      console_printf("Aumentando tamanho da tabela de processos");
      self->tamanho_tabela_processos = self->tamanho_tabela_processos *2;
      self->tabela_processos = realloc(self->tabela_processos, self->tamanho_tabela_processos * sizeof(*self->tabela_processos));
      if(self->tabela_processos == NULL){
        return NULL;
      }
    }
    self->tabela_processos[self->num_processos++] = processo;
    escalonador_adiciona_processo(self->escalonador, processo);
    self->num_processos_criados++;
    return processo;
}


static void so_mata_processo(so_t *self, processo_t *processo) {
    int es_id = processo_es_id(processo);

    if (es_id != -1) {
        processo_libera_es(processo);
        controle_libera_dispositivo(self->controle_es,es_id);
    }
    //console_printf("SO: escalonador removeu processo, %d", processo_pid(processo));
    escalonador_remove_processo(self->escalonador, processo);
    processo_encerra(processo);
}

static void so_executa_processo(so_t *self, processo_t *processo) {
    console_printf("dentro de executa processo %d.\n", processo_pid(processo));

    // Gerenciar troca de contexto, verificando se o processo precisa carregar páginas
    if (self->processo_corrente != NULL && self->processo_corrente != processo && processo_estado(self->processo_corrente) == EM_EXECUCAO) {
        console_printf("Parando processo.\n");
        processo_para(self->processo_corrente);
        so_calcula_mudanca_estado_processo(self, self->processo_corrente);
        self->num_total_preempcoes++;

        // Se o processo pausado não está morto, re-insira no escalonador
        if (processo_estado(self->processo_corrente) != MORTO) {
            escalonador_adiciona_processo(self->escalonador, self->processo_corrente);
        }
    }

    // Aqui, ao retomar a execução do processo, é necessário garantir que a MMU seja restaurada
    if (processo != NULL && processo_estado(processo) != EM_EXECUCAO) {
        console_printf("Executando processo.\n");

        // Carregar as páginas do processo na memória física, se necessário
        //mmu_carrega_paginas(self->mmu, processo);

        // Executar o processo, com a memória sendo gerida pela MMU
        processo_executa(processo, self->mmu);
    }

    if (processo != NULL && processo_estado(processo) != MORTO) {
        escalonador_remove_processo(self->escalonador, processo);
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
  //console_printf("SO: dentro de so tem trabalho");
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
    fprintf(file, "Terminal ID: %d\n", processo_es_id(processo));
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


int substituir_pagina_fifo(so_t *self) {
    int quadro_removido = self->fila_quadros[self->pos_fila_inicio];
    self->pos_fila_inicio = (self->pos_fila_inicio + 1) % TOTAL_QUADROS;
    return quadro_removido;
}

void adicionar_pagina_fifo(so_t *self, int quadro) {
    self->fila_quadros[self->pos_fila_fim] = quadro;
    self->pos_fila_fim = (self->pos_fila_fim + 1) % TOTAL_QUADROS;
}
int substituir_pagina_segunda_chance(so_t *self) {
    while (true) {
        int quadro_atual = self->fila_quadros[self->pos_fila_inicio];
        if (self->bits_acesso[quadro_atual] == 0) {
            self->pos_fila_inicio = (self->pos_fila_inicio + 1) % TOTAL_QUADROS;
            return quadro_atual; // Substitui página sem segunda chance
        } else {
            // Dá segunda chance: zera o bit e move para o fim da fila
            self->bits_acesso[quadro_atual] = 0;
            adicionar_pagina_fifo(self, quadro_atual);
            self->pos_fila_inicio = (self->pos_fila_inicio + 1) % TOTAL_QUADROS;
        }
    }
}

void atualizar_bit_acesso(so_t *self, int quadro) {
    self->bits_acesso[quadro] = 1; // Indica que o quadro foi acessado
}


static int fifo(so_t *self)
{
  int max_cycles = -1;
  int purged_block = -1;
  int cur_cicles;

  if(es_le(self->es, D_RELOGIO_INSTRUCOES, &cur_cicles) != ERR_OK)
  {
    console_printf("Erro crítico de atualização da memória");
    return 2;
  }

  // começa em 2 pois os dois primeiros blocos são espaço reservado
  for (int i = 2; i < self->num_pag_fisica; i++)
  {
    int this_cicles = cur_cicles - self->controle_blocos->blocos[i].ciclos;
    if (this_cicles > max_cycles)
    {
      max_cycles = this_cicles;
      purged_block = i;
    }
  }

  return purged_block;
}

static int second_chance(so_t *self)
{
  int max_cycles = -1;
  int purged_block = -1;
  int cur_cicles;

  if(es_le(self->es, D_RELOGIO_INSTRUCOES, &cur_cicles) != ERR_OK)
  {
    console_printf("Erro crítico de atualização da memória");
    return 2;
  }

  // começa em 2 pois os dois primeiros blocos são espaço reservado

  // loop para os sem segunda chance
  for (int i = 2; i < self->num_pag_fisica; i++)
  {   
    tabpag_t *proc_tabpag = processo_tab_pag(self->tabela_processos[self->controle_blocos->blocos[i].processo_pid]);
    if (tabpag_bit_acesso(proc_tabpag, self->controle_blocos->blocos[i].pagina) == 0)
    {
      int this_cicles = cur_cicles - self->controle_blocos->blocos[i].ciclos;
      if (this_cicles > max_cycles)
      {
        max_cycles = this_cicles;
        purged_block = i;
      }
    }
  }

  if (purged_block == -1)
  {
    for (int i = 2; i < self->num_pag_fisica; i++)
    {   
      tabpag_t *proc_tabpag = processo_tab_pag(self->tabela_processos[self->controle_blocos->blocos[i].processo_pid]);
      if (tabpag_bit_acesso(proc_tabpag, self->controle_blocos->blocos[i].pagina) == 1)
      {
        tabpag_zera_bit_acesso(proc_tabpag, self->controle_blocos->blocos[i].pagina);
        int this_cicles = cur_cicles - self->controle_blocos->blocos[i].ciclos;
        if (this_cicles > max_cycles)
        {
          max_cycles = this_cicles;
          purged_block = i;
        }
      }
    }
  }
  
  return purged_block;
}



static int escolhe_pagina_substituicao(so_t *self) {
    if (self == NULL) {
        console_printf("Erro: Sistema operacional não inicializado.\n");
        return -1;
    }

    switch (self->algoritmo_substituicao) {
      case ALGORITMO_FIFO:
          return fifo(self);

      case ALGORITMO_SEGUNDA_CHANCE:
          return second_chance(self);

      default:
          console_printf("Erro: Algoritmo de substituição desconhecido.\n");
          return -1;
    }
}

static void so_trata_page_fault_espaco_encontrado(so_t *self, int end_causador)
{
    int free_page = controle_blocos_busca_bloco_disponivel(self->controle_blocos);
    
    int end_disk_ini = processo_endereco_disco(self->processo_corrente) + end_causador - end_causador%TAM_PAGINA;
    int end_disk = end_disk_ini;

    int end_virt_ini = end_causador;
    int end_virt_fim = end_virt_ini + TAM_PAGINA - 1;

    for (int end_virt = end_virt_ini; end_virt <= end_virt_fim; end_virt++) {
      int dado;
      if (mem_le(self->mem_secundaria, end_disk, &dado) != ERR_OK) {
        console_printf("Erro na leitura no tratamento de page fault");
        return;
      }

      int physical_target_address = free_page*TAM_PAGINA + (end_virt - end_virt_ini);

      if (mem_escreve(self->mem, physical_target_address, dado) != ERR_OK) {
        console_printf("Erro na escrita no tratamento de page fault");
        return;
      }
      end_disk++;
    }

    self->controle_blocos->blocos[free_page].em_uso = true;
    self->controle_blocos->blocos[free_page].processo_pid = processo_pid(self->processo_corrente);
    self->controle_blocos->blocos[free_page].pagina = end_causador/TAM_PAGINA;
    self->controle_blocos->blocos[free_page].chance = false;

    if(es_le(self->es, D_RELOGIO_INSTRUCOES, &self->controle_blocos->blocos[free_page].ciclos) != ERR_OK)
    {
      console_printf("Erro crítico de atualização da memória");
    }

    tabpag_t *tabela = processo_tab_pag(self->processo_corrente);
    tabpag_define_quadro(tabela, end_causador/TAM_PAGINA, free_page, self->controle_quadros);
}
// Função para escrever uma página da memória principal para o disco
static int escreve_para_disco(mem_t *mem_principal, mem_t *mem_disco, int bloco, int endereco_disco) {
  for (int i = 0; i < TAM_PAGINA; i++) {
    int v;
    if (mem_le(mem_principal, bloco * TAM_PAGINA + i, &v) != ERR_OK) {
      console_printf("Erro na leitura no tratamento de page fault");
      return 0;
    }
    if (mem_escreve(mem_disco, endereco_disco + i, v) != ERR_OK) {
      console_printf("Erro na escrita no tratamento de page fault");
      return 0;
    }
  }
  return ERR_OK;
}

// Função para ler uma página do disco para a memória principal
static int le_do_disco(mem_t *mem_disco, mem_t *mem_principal, int endereco_disco, int bloco) {
  for (int i = 0; i < TAM_PAGINA; i++) {
    int v;
    if (mem_le(mem_disco, endereco_disco + i, &v) != ERR_OK) {
      console_printf("Erro na leitura no tratamento de page fault");
      return 0;
    }
    if (mem_escreve(mem_principal, bloco * TAM_PAGINA + i, v) != ERR_OK) {
      console_printf("Erro na escrita no tratamento de page fault");
      return 0;
    }
  }
  return ERR_OK;
}

// Função principal de swap
static void so_swap_pagina(so_t *self, int end_causador) {
  int to_remove_mem_block = escolhe_pagina_substituicao(self);
  processo_t *outgoing_process = self->tabela_processos[self->controle_blocos->blocos[to_remove_mem_block].processo_pid];
  processo_t *incoming_process = self->processo_corrente;

  if (outgoing_process != NULL) {
    tabpag_t *outgoing_page_table = processo_tab_pag(outgoing_process);
    int outgoing_page = self->controle_blocos->blocos[to_remove_mem_block].pagina;
    int disk_write_address = processo_endereco_disco(outgoing_process) + (outgoing_page * TAM_PAGINA);

    // Escreve no disco se a página foi alterada
    if (tabpag_bit_alteracao(outgoing_page_table, outgoing_page)) {
      if (escreve_para_disco(self->mem, self->mem_secundaria, to_remove_mem_block, disk_write_address) != ERR_OK) {
        return;
      }
    }

    console_printf("SO: Removeu o conteúdo do bloco %d usado pela página %d do processo #%d", to_remove_mem_block, outgoing_page, processo_pid(outgoing_process));
    tabpag_invalida_pagina(outgoing_page_table, outgoing_page, self->controle_quadros);
  }

  int end_disk_ini = processo_endereco_disco(incoming_process) + end_causador - end_causador % TAM_PAGINA;

  // Lê do disco para a memória
  if (le_do_disco(self->mem_secundaria, self->mem, end_disk_ini, to_remove_mem_block) != ERR_OK) {
    return;
  }

  self->controle_blocos->blocos[to_remove_mem_block].em_uso = true;
  self->controle_blocos->blocos[to_remove_mem_block].processo_pid = processo_pid(incoming_process);
  self->controle_blocos->blocos[to_remove_mem_block].pagina = end_causador / TAM_PAGINA;

  if (es_le(self->es, D_RELOGIO_INSTRUCOES, &self->controle_blocos->blocos[to_remove_mem_block].ciclos) != ERR_OK) {
    console_printf("Erro crítico de atualização da memória");
  }

  tabpag_t *incoming_page_table = processo_tab_pag(incoming_process);
  tabpag_define_quadro(incoming_page_table, end_causador / TAM_PAGINA, to_remove_mem_block, self->controle_quadros);

  console_printf("SO: Inseriu no bloco %d a página %d do processo #%d", to_remove_mem_block, end_causador / TAM_PAGINA, processo_pid(incoming_process));
}

// static void so_swap_pagina(so_t *self, int end_causador)
// {
//   int to_remove_mem_block = escolhe_pagina_substituicao(self);
//   processo_t *outgoing_process = self->tabela_processos[self->controle_blocos->blocos[to_remove_mem_block].processo_pid];
//   processo_t *incoming_process = self->processo_corrente;

//   if (outgoing_process != NULL)
//   {
//     // trata possível alteração e necessidade de writeback na memória secundária
//     tabpag_t *outgoing_page_table = processo_tab_pag(outgoing_process);
//     int outgoing_page = self->controle_blocos->blocos[to_remove_mem_block].pagina;

//     // encontra onde escrever de volta
//     int disk_write_address = processo_endereco_disco(outgoing_process) + (outgoing_page * TAM_PAGINA);


//     // escreve de volta se foi alterada
//     if (tabpag_bit_alteracao(outgoing_page_table, outgoing_page))
//     {
//       for (int i = 0; i < TAM_PAGINA; i++)
//       {
//         int v;
//         if (mem_le(self->mem, to_remove_mem_block*TAM_PAGINA + i, &v) != ERR_OK)
//         {
//           console_printf("Erro na leitura no tratamento de page fault");
//           return;
//         }
        
//         if (mem_escreve(self->mem_secundaria, disk_write_address + i, v) != ERR_OK) 
//         {
//           console_printf("Erro na escrita no tratamento de page fault");
//           return;
//         }
//       }
//     }

//     console_printf("SO: Removeu o conteúdo do bloco %d usado pela página %d do processo #%d", to_remove_mem_block, outgoing_page, processo_pid(outgoing_process));

//     // invalida página na tabela do processo de saída
//     tabpag_invalida_pagina(outgoing_page_table, self->controle_blocos->blocos[to_remove_mem_block].pagina, self->controle_quadros);
//   }
  

//   int end_disk_ini = processo_endereco_disco(incoming_process) + end_causador - end_causador%TAM_PAGINA;

//   // lê a página
//   for (int i = 0; i < TAM_PAGINA; i++)
//   {
//     int v;
//     if (mem_le(self->mem_secundaria, end_disk_ini + i, &v) != ERR_OK)
//     {
//       console_printf("Erro na leitura no tratamento de page fault");
//       return;
//     }
    
//     if (mem_escreve(self->mem, to_remove_mem_block*TAM_PAGINA + i, v) != ERR_OK) 
//     {
//       console_printf("Erro na escrita no tratamento de page fault");
//       return;
//     }
//   }

//   self->controle_blocos->blocos[to_remove_mem_block].em_uso = true;
//   self->controle_blocos->blocos[to_remove_mem_block].processo_pid = processo_pid(incoming_process);
//   self->controle_blocos->blocos[to_remove_mem_block].pagina = end_causador/TAM_PAGINA;
//   self->controle_blocos->blocos[to_remove_mem_block].chance = false;

//   if(es_le(self->es, D_RELOGIO_INSTRUCOES, &self->controle_blocos->blocos[to_remove_mem_block].ciclos) != ERR_OK)
//   {
//     console_printf("Erro crítico de atualização da memória");
//   }

//   tabpag_t *incoming_page_table = processo_tab_pag(incoming_process);
//   tabpag_define_quadro(incoming_page_table, end_causador/TAM_PAGINA, to_remove_mem_block, self->controle_quadros);

//   console_printf("SO: Inseriu no bloco %d a página %d do processo #%d", to_remove_mem_block, end_causador/TAM_PAGINA, processo_pid(incoming_process));
// }

static void so_trata_page_fault(so_t *self)
{
  //proc_get_metrics_ptr(self->current_process)->page_faults++;
  int end_causador = processo_complemento(self->processo_corrente);

  bool has_free_block = controle_blocos_bloco_disponivel(self->controle_blocos);
  if(has_free_block)
  {
    console_printf("SO: tratando falha de página com bloco livre");
    so_trata_page_fault_espaco_encontrado(self, end_causador);
  }

  else
  {
    console_printf("SO: tratando falha de página sem bloco livre");
    so_swap_pagina(self, end_causador);
  }

  so_bloqueia_processo(self, self->processo_corrente, ESPERA_PAGINA, TEMPO_TRANSFERENCIA);
}


// static void so_trata_page_fault_espaco_encontrado(so_t *self, int end_causador)
// {
//     int free_page = controle_blocos_busca_bloco_disponivel(self->controle_blocos);
    
//     int end_disk_ini = processo_endereco_disco(self->processo_corrente) + end_causador - end_causador % TAM_PAGINA;
//     int end_disk = end_disk_ini;
//     console_printf("SO: end disk = %d", end_disk);

//     int end_virt_ini = end_causador;
//     int end_virt_fim = end_virt_ini + TAM_PAGINA - 1;

//     for (int end_virt = end_virt_ini; end_virt <= end_virt_fim; end_virt++) {
//       int dado;
//       if (mem_le(self->mem_secundaria, end_disk, &dado) != ERR_OK) {
//         console_printf("Erro na leitura no tratamento de page fault");
//         return;
//       }

//       int physical_target_address = free_page*TAM_PAGINA + (end_virt - end_virt_ini);

//       if (mem_escreve(self->mem, physical_target_address, dado) != ERR_OK) {
//         console_printf("Erro na escrita no tratamento de page fault");
//         return;
//       }

//       end_disk++;
//     }

//     self->controle_blocos->blocos[free_page].em_uso = true;
//     self->controle_blocos->blocos[free_page].processo_pid = processo_pid(self->processo_corrente);
//     self->controle_blocos->blocos[free_page].pagina = end_causador/TAM_PAGINA;
    

//     tabpag_t *tabela = processo_tab_pag(self->processo_corrente);
//     tabpag_define_quadro(tabela, end_causador/TAM_PAGINA, free_page, self->controle_quadros);

//     console_printf("SO: falta de página tratada - havia quadro livre");
// }

// static bool copiar_pagina_disco_para_quadro(so_t *self, int endereco_disco, int quadro_destino) {
//     for (int offset = 0; offset < TAM_PAGINA; offset++) {
//         int dado;
//         if (mem_le(self->mem_secundaria, endereco_disco + offset, &dado) != ERR_OK) {
//             console_printf("Erro ao ler dado do disco\n");
//             return false;
//         }

//         int endereco_fisico = quadro_destino * TAM_PAGINA + offset;
//         if (mem_escreve(self->mem, endereco_fisico, dado) != ERR_OK) {
//             console_printf("Erro ao escrever dado na memória\n");
//             return false;
//         }
//     }
//     return true;
// }

// static bool copiar_quadro_para_disco(so_t *self, int quadro_fonte, int endereco_disco) {
//     for (int offset = 0; offset < TAM_PAGINA; offset++) {
//         int dado;
//         int endereco_fisico = quadro_fonte * TAM_PAGINA + offset;

//         if (mem_le(self->mem, endereco_fisico, &dado) != ERR_OK) {
//             console_printf("Erro ao ler dado do quadro físico\n");
//             return false;
//         }

//         if (mem_escreve(self->mem_secundaria, endereco_disco + offset, dado) != ERR_OK) {
//             console_printf("Erro ao escrever dado no disco\n");
//             return false;
//         }
//     }
//     return true;
// }



// // Função swap_out: Salva a página vítima no disco
// static bool swap_out(so_t *self, int quadro_vitima) {
//     int pid_vitima = self->controle_blocos->blocos[quadro_vitima].processo_pid;
//     int pagina_vitima = self->controle_blocos->blocos[quadro_vitima].pagina;
//     processo_t *processo_vitima = so_busca_processo(self, pid_vitima);
//     tabpag_t *tabela_vitima = processo_tab_pag(processo_vitima);

//     if (tabpag_pagina_modificada(tabela_vitima, quadro_vitima)) {
//         int endereco_disco_vitima = processo_endereco_disco(processo_vitima) + (pagina_vitima * TAM_PAGINA);
//         if (endereco_disco_vitima < 0) {
//             console_printf("Erro ao calcular o endereço de disco.");
//         }

//         // Copiar os dados do quadro físico para o disco
//         if (!copiar_quadro_para_disco(self, quadro_vitima, endereco_disco_vitima)) {
//             console_printf("Erro ao salvar página vítima no disco");
//             return false;
//         }

//         console_printf("SO: página vítima salva no disco");
//     }

//     // Invalidar o quadro da tabela de páginas
//     tabpag_invalida_quadro(tabela_vitima, pagina_vitima, self->controle_quadros);
//     return true;
// }

// // Função swap_in: Carrega a página do disco para o quadro físico
// static bool swap_in(so_t *self, int quadro_destino, int end_causador) {
//     int end_disk_ini = processo_endereco_disco(self->processo_corrente) + 
//                        end_causador - (end_causador % TAM_PAGINA);
//     int end_disk = end_disk_ini;

//     // Copiar os dados do disco para o quadro físico
//     if (!copiar_pagina_disco_para_quadro(self, end_disk, quadro_destino)) {
//         console_printf("Erro ao carregar página do disco no quadro físico");
//         return false;
//     }

//     // Atualizar a tabela de blocos e a tabela de páginas
//     self->controle_blocos->blocos[quadro_destino].em_uso = true;
//     self->controle_blocos->blocos[quadro_destino].processo_pid = processo_pid(self->processo_corrente);
//     self->controle_blocos->blocos[quadro_destino].pagina = end_causador / TAM_PAGINA;

//     tabpag_define_quadro(processo_tab_pag(self->processo_corrente), 
//                          end_causador / TAM_PAGINA, quadro_destino, 
//                          self->controle_quadros);
//     set_mem_tempo_disponivel(self->mem_secundaria, self->tempo_relogio_atual + TEMPO_TRANSFERENCIA);

//     console_printf("SO: página carregada no quadro físico");
//     return true;
// }

// // Função de tratamento de page fault com substituição
// static void so_trata_page_fault_com_substituicao(so_t *self, int end_causador, int quadro_vitima) {
//     if (!swap_out(self, quadro_vitima)) {
//         self->erro_interno = true;
//         return;
//     }

//     if (!swap_in(self, quadro_vitima, end_causador)) {
//         self->erro_interno = true;
//         return;
//     }

//     console_printf("SO: falta de página tratada com substituição");
// }

// // Função de tratamento de page fault quando há espaço livre
// static void so_trata_page_fault_espaco_encontrado(so_t *self, int end_causador) {
//     int bloco_disponivel = controle_blocos_busca_bloco_disponivel(self->controle_blocos);

//     if (!swap_in(self, bloco_disponivel, end_causador)) {
//         self->erro_interno = true;
//         return;
//     }

//     console_printf("SO: falta de página tratada - havia quadro livre");
// }


// // Atualização da função principal de tratamento de page fault
// static void so_trata_page_fault(so_t *self) {
//     int end_causador = processo_complemento(self->processo_corrente);
//     console_printf("SO: endereço causador do page fault = %d", end_causador);

//     int tempo_disponivel = mem_tempo_disponivel(self->mem_secundaria);

//     if (tempo_disponivel > self->tempo_relogio_atual) {
//         // O disco está ocupado; bloquear o processo até que esteja disponível
//         so_bloqueia_processo(
//             self,
//             self->processo_corrente,
//             ESPERA_PAGINA,
//             tempo_disponivel - self->tempo_relogio_atual
//         );
//         console_printf("SO: processo %d bloqueado aguardando disco.\n", 
//             processo_pid(self->processo_corrente));
//         return;
//     }

//     if (controle_blocos_bloco_disponivel(self->controle_blocos)) {
//         so_trata_page_fault_espaco_encontrado(self, end_causador);
//     } else {
//         console_printf("SO: memória principal cheia");
//         int vitima = escolhe_pagina_substituicao(self);

//         if (vitima == -1) {
//             console_printf("SO: erro ao escolher página vítima");
//             self->erro_interno = true;
//             return;
//         }

//         console_printf("SO: página vítima escolhida = %d", vitima);
//         so_trata_page_fault_com_substituicao(self, end_causador, vitima);
//     }
// }





//@TODO - colocar os erros q tem no readme -- soh tem dois acessos ao disco - memoria principal - alem dos registradores - toda falha de pagina gera pelo menos dois acessos ao disco
//tem q ver esse negocio do bloco - duas funcoes para copiar o bloco para o quadro e copia pagina para o bloco - e funcao para alterar o tempo do disco
// toda vez q tiver page fault ele bloqueia o processo por falta de pagina - soh tem q ver o quanto de tempo q ele vai ficar bloqueado
// static bool so_trata_page_fault(so_t *self, int pagina_virt, processo_t *processo) {
//     // Verifica se a página já está na memória principal
//     int quadro_fisico;
//     err_t err = mem_le(self->mem, pagina_virt, &quadro_fisico);
//     // quadro_fisico = tabpag_quadro(processo_tab_pag(processo), pagina_virt);
//     // if (quadro_fisico != -1 && tabpag_pagina_valida(processo_tab_pag(processo), pagina_virt)) {
//     //     return true; // Página já está na memória principal
//     // }

//     if (err != ERR_OK || quadro_fisico == -1) {
//         // Verifica se o disco está livre
//         int tempo_disponivel = mem_tempo_disponivel(self->mem_secundaria);
//         if (tempo_disponivel > self->tempo_relogio_atual) {
//             //pega a hora do relogio
//             // O disco está ocupado; bloquear o processo até que esteja disponível
//             //@TODO- mando como argumento do bloqueio a hora que era quando bloqueou o processo
//             so_bloqueia_processo(self, processo, ESPERA_PAGINA, tempo_disponivel - self->tempo_relogio_atual);
//             //console_printf("Processo %d bloqueado aguardando disco.\n", processo);
//             return false;
//         }

//         // Buscar a página na memória secundária
//         int quadro_secundario;
//         err = mem_le(self->mem_secundaria, pagina_virt, &quadro_secundario);
//         if (err != ERR_OK) {
//             //console_printf("Erro: Página %d não encontrada na memória secundária.\n", pagina_virt);
//             return false;
//         }

//         // Tentar alocar um quadro na memória principal
//         quadro_fisico = controle_quadros_aloca(self->controle_quadros);
//         if (quadro_fisico == -1) {
//             // Substituir página se não houver quadro livre
//             if (self->algoritmo_substituicao == ALGORITMO_FIFO) {
//                 quadro_fisico = substituir_pagina_fifo(self);
//             } else if (self->algoritmo_substituicao == ALGORITMO_SEGUNDA_CHANCE) {
//                 quadro_fisico = substituir_pagina_segunda_chance(self);
//             }

//             if (quadro_fisico == -1) {
//                 console_printf("Erro: Falha na substituição de páginas.\n");
//                 return false;
//             }

//             // Salvar página substituída se necessário
//            if (tabpag_pagina_modificada(processo_tab_pag(processo), quadro_fisico)) {
//               int pagina_removida = tabpag_encontra_pagina(processo_tab_pag(processo), quadro_fisico);
//               if (pagina_removida == -1) {
//                   //console_printf("Erro: Quadro %d não está associado a nenhuma página.\n", quadro_fisico);
//                   return false;
//               }
//               err = mem_escreve(self->mem_secundaria, pagina_removida, quadro_fisico);
//               if (err != ERR_OK) {
//                   console_printf("Erro ao salvar página substituída.\n");
//                   return false;
//               }
//               tabpag_invalida_pagina(processo_tab_pag(processo), pagina_removida, self->controle_quadros);

//           }
//         }

//         // Carregar a página da memória secundária para o quadro físico
//         err = mem_escreve(self->mem, quadro_fisico, quadro_secundario);
//         if (err != ERR_OK) {
//             console_printf("Erro ao carregar página para a memória principal.\n");
//             return false;
//         }

//         // Atualizar a tabela de páginas
//         tabpag_define_quadro(processo_tab_pag(processo), pagina_virt, quadro_fisico, self->controle_quadros);

//         // Atualizar tempo de disponibilidade do disco
//         set_mem_tempo_disponivel(self->mem_secundaria, self->tempo_relogio_atual + TEMPO_TRANSFERENCIA);

//         //console_printf("Página %d carregada no quadro %d.\n", pagina_virt, quadro_fisico);
//         return true;
//     }

//     return false;
// }
// static bool so_trata_page_fault(so_t *self, int pagina_virt, processo_t *processo) {
//     console_printf("Dentro de page fault para a página %d\n", pagina_virt);

//     // Usando a MMU para traduzir o endereço virtual
//     int quadro_fisico;
//     err_t err = mmu_le(self->mmu, pagina_virt * TAM_PAGINA, &quadro_fisico, usuario);
//     if (err == ERR_OK) {
//         // Página já está na memória principal (a MMU fez a tradução)
//         return true;
//     }

//     // Verifica se há tempo suficiente no disco
//     int tempo_disponivel = mem_tempo_disponivel(self->mem_secundaria);
//     if (tempo_disponivel > self->tempo_relogio_atual) {
//         so_bloqueia_processo(self, processo, ESPERA_PAGINA, tempo_disponivel - self->tempo_relogio_atual);
//         return false;
//     }

//     // Aloca ou substitui um quadro na memória principal
//     quadro_fisico = controle_quadros_aloca(self->controle_quadros);
//     if (quadro_fisico == -1) {
//         if (self->algoritmo_substituicao == ALGORITMO_FIFO) {
//             quadro_fisico = substituir_pagina_fifo(self);
//         } else if (self->algoritmo_substituicao == ALGORITMO_SEGUNDA_CHANCE) {
//             quadro_fisico = substituir_pagina_segunda_chance(self);
//         }
//         if (quadro_fisico == -1) {
//             return false;
//         }
//     }

//     // Carrega a página da memória secundária para a memória principal
//     for (int offset = 0; offset < TAM_PAGINA; offset++) {
//         int dado;
//         int end_virt = pagina_virt * TAM_PAGINA + offset;
//         int end_fis = quadro_fisico * TAM_PAGINA + offset;

//         if (mem_le(self->mem_secundaria, end_virt, &dado) != ERR_OK ||
//             mem_escreve(self->mem, end_fis, dado) != ERR_OK) {
//             return false;
//         }
//     }

//     // Atualiza a tabela de páginas com o novo quadro físico usando a MMU
//     tabpag_define_quadro(processo_tab_pag(processo), pagina_virt, quadro_fisico, self->controle_quadros);

//     // Atualiza o tempo de disponibilidade do disco
//     set_mem_tempo_disponivel(self->mem_secundaria, self->tempo_relogio_atual + TEMPO_TRANSFERENCIA);

//     console_printf("Página %d carregada no quadro %d.\n", pagina_virt, quadro_fisico);
//     return true;
// }


// CARGA DE PROGRAMA {{{1

static void so_marca_uso_memoria(so_t *self, int end_ini, int end_fim, int proc_id)
{
  for (int address = 0; address < end_fim; address += TAM_PAGINA)
  {
    self->controle_blocos->blocos[address/TAM_PAGINA].em_uso = true;
    self->controle_blocos->blocos[address/TAM_PAGINA].processo_pid = proc_id;
  }
}

// funções auxiliares
static int so_carrega_programa_na_memoria_fisica(so_t *self, programa_t *programa);
static int so_carrega_programa_na_memoria_virtual(so_t *self,
                                                  programa_t *programa,
                                                  processo_t *processo);

// carrega o programa na memória de um processo ou na memória física se NENHUM_PROCESSO
// retorna o endereço de carga ou -1
static int so_carrega_programa(so_t *self, processo_t *processo,
                               char *nome_do_executavel)
{

  programa_t *programa = prog_cria(nome_do_executavel);
  if (programa == NULL) {
    console_printf("Erro na leitura do programa '%s'\n", nome_do_executavel);
    return -1;
  }

  int end_carga;
  if (processo == NULL) {
    console_printf("\n\nSO: carregando1 programa na memória física processo \n");
    end_carga = so_carrega_programa_na_memoria_fisica(self, programa);
  } else {
    console_printf("\n\nSO: carregando2 programa na memória virtual processo \n");
    end_carga = so_carrega_programa_na_memoria_virtual(self, programa, processo);
    processo_set_endereco_disco(processo, end_carga);
    //console_printf("\n end carga %d\n", end_carga);
  }


  prog_destroi(programa);
  return end_carga;
}

static int so_carrega_programa_na_memoria_fisica(so_t *self, programa_t *programa)
{
  int end_ini = prog_end_carga(programa);
  int end_fim = end_ini + prog_tamanho(programa);

  for (int end = end_ini; end < end_fim; end++) {
    if (mem_escreve(self->mem, end, prog_dado(programa, end)) != ERR_OK) {
      console_printf("Erro na carga da memória, endereco %d\n", end);
      return -1;
    }
  }

  so_marca_uso_memoria(self, end_ini, end_fim, 0);
  console_printf("\ncarregado na memória física, %d-%d\n", end_ini, end_fim);
  return end_ini;
}


// tem q guardar os blocos da memoria virtual, dai tem q converter pagina do processo em bloco

//teste - parece faltar colcoar as paginas da tabela como invalidas
static int so_carrega_programa_na_memoria_virtual(so_t *self,
                                                  programa_t *programa,
                                                  processo_t *processo)
{
  int end_disco_ini = self->ponteiro_disco;
  int end_disco = end_disco_ini;

  int end_virt_ini = 0;
  int end_virt_fim = end_virt_ini + prog_tamanho(programa) - 1;

  self->ponteiro_disco = end_disco_ini + end_virt_fim + 1;

  for (int end_virt = end_virt_ini; end_virt <= end_virt_fim; end_virt++) {

    if (mem_escreve(self->mem_secundaria, end_disco, prog_dado(programa, end_virt)) != ERR_OK) {
        console_printf("Erro ao carregar programa na memória secundária no endereço virtual, fisico- %d, virtual- %d\n", end_disco, end_virt);
        return -1; 
    }
    end_disco++;
  }
  return end_disco_ini;

}
//brizzgui
// static bool so_copia_str_do_processo(so_t *self, int tam, char str[tam],
//                                      int end_virt, processo_t *processo)
// {
//   if (processo == NULL) return false;

//   for (int indice_str = 0; indice_str < tam; indice_str++) {
//     int caractere;
//     // não tem memória virtual implementada, posso usar a mmu para traduzir
//     //   os endereços e acessar a memória
//     if (mmu_le(self->mmu, end_virt + indice_str, &caractere, usuario) != ERR_OK) {
//       return false;
//     }
//     if (caractere < 0 || caractere > 255) {
//       return false;
//     }
//     str[indice_str] = caractere;
//     if (caractere == 0) {
//       return true;
//     }
//   }
//   // estourou o tamanho de str
//   return false;
// }

// static int so_carrega_programa_na_memoria_virtual(so_t *self,
//                                                   programa_t *programa,
//                                                   processo_t *processo)
// {
//     // Define o intervalo de endereços virtuais para o programa
//     int end_virt_ini = prog_end_carga(programa);
//     int end_virt_fim = end_virt_ini + prog_tamanho(programa) - 1;
//     int pagina_ini = end_virt_ini / TAM_PAGINA;
//     int pagina_fim = end_virt_fim / TAM_PAGINA;

//     // Inicializa a tabela de páginas do processo com entradas inválidas
//     for (int pagina = pagina_ini; pagina <= pagina_fim; pagina++) {
//         tabpag_invalida_pagina(processo_tab_pag(processo), pagina, self->controle_quadros);
//     }

//     // Aloca espaço na memória secundária para armazenar o programa
//     for (int offset = 0; offset < prog_tamanho(programa); offset++) {
//         int end_virt = end_virt_ini + offset;

//         // Escreve os dados do programa na memória secundária
//         if (mem_escreve(self->mem_secundaria, end_virt, prog_dado(programa, offset)) != ERR_OK) {
//             console_printf("Erro ao carregar programa na memória secundária no endereço virtual %d\n", end_virt);
//             return -1; // Retorna em caso de erro
//         }
//     }

//     console_printf("\nPrograma carregado na memória virtual de %d até %d (todas as páginas inválidas inicialmente).\n",
//                    end_virt_ini, end_virt_fim);

//     return end_virt_ini; // Retorna o endereço virtual inicial do programa
// }



// ACESSO À MEMÓRIA DOS PROCESSOS {{{1

// copia uma string da memória do processo para o vetor str.
// retorna false se erro (string maior que vetor, valor não char na memória,
//   erro de acesso à memória)
// O endereço é um endereço virtual de um processo.
// T2: Com memória virtual, cada valor do espaço de endereçamento do processo
//   pode estar em memória principal ou secundária (e tem que achar onde)
static bool so_copia_str_do_processo(so_t *self, int tam, char str[tam],
                                     int end_virt, processo_t *processo)
{
  if (processo == NULL) return false;
  for (int indice_str = 0; indice_str < tam; indice_str++) {
    int caractere;
    if (mmu_le(self->mmu, end_virt + indice_str, &caractere, usuario) != ERR_OK) {
      console_printf("Erro ao ler o endereço virtual %d do processo %d\n", end_virt + indice_str, processo_pid(processo));
      mem_le(self->mem_secundaria, processo_endereco_disco(self->processo_corrente) + end_virt + indice_str, &caractere);
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


// static bool so_copia_str_do_processo(so_t *self, int tam, char str[tam],
//                                      int end_virt, processo_t *processo)
// {
//   if (processo == NULL) return false;
  
//   for (int indice_str = 0; indice_str < tam; indice_str++) {
//     int caractere;
//     // Calcular a página e o offset do endereço virtual
//     int pagina_virt = (end_virt + indice_str) / TAM_PAGINA;
//     int offset = (end_virt + indice_str) % TAM_PAGINA;

//     int quadro_fisico;
//     err_t status = tabpag_traduz(processo_tab_pag(processo), pagina_virt, &quadro_fisico);

//     if (status == ERR_PAG_AUSENTE) {
//         // A página não está na memória física, precisa carregar da memória secundária
//         if (!so_trata_page_fault(self, pagina_virt, processo)) {
//             return false; // Falha ao tratar o page fault
//         }
//         // Tenta traduzir novamente após tratar o page fault
//         status = tabpag_traduz(processo_tab_pag(processo), pagina_virt, &quadro_fisico);
//         if (status != ERR_OK) {
//             return false; // Falha na tradução
//         }
//     }
// //novo comentario pro make nfofhoashf
//     // Agora, temos o quadro físico, podemos calcular o endereço físico real
//     int end_fis = quadro_fisico * TAM_PAGINA + offset;
    
//     // Ler o caractere da memória física
//     if (mem_le(self->mem, end_fis, &caractere) != ERR_OK) {
//       return false; // Falha na leitura da memória
//     }

//     // Verificar se o caractere está no intervalo válido
//     if (caractere < 0 || caractere > 255) {
//       return false;
//     }

//     str[indice_str] = caractere;

//     // Se encontrou o caractere nulo, termina a cópia
//     if (caractere == 0) {
//       return true;
//     }
//   }
  
//   // Se a string ultrapassar o tamanho do vetor str, retorna erro
//   return false;
// }

// vim: foldmethod=marker
