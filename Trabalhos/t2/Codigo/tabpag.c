// tabpag.c
// tabela de páginas para a MMU
// simulador de computador
// so24b

#include "tabpag.h"
#include "console.h"
#include <stdlib.h>
#include <assert.h>

// estrutura auxiliar, contém informação sobre uma página
typedef struct {
  // quadro da memória principal correspondente à página
  int quadro;
  // a página está mapeada ou não
  bool valida;
  // a página foi acessada ou não
  bool acessada;
  // a página foi alterada ou não
  bool alterada;
} descritor_t;

struct tabpag_t {
  // número de descritores na tabela (pode ser 0)
  int tam_tab;
  // vetor com os descritores
  // o último descritor do vetor sempre contém uma página válida
  // pode ser NULL (se tam_tab == 0)
  descritor_t *tabela;
};

tabpag_t *tabpag_cria(void)
{
  tabpag_t *self = malloc(sizeof(*self));
  assert(self != NULL);
  self->tam_tab = 0;
  self->tabela = NULL;
  return self;
}

void tabpag_destroi(tabpag_t *self, controle_quadros_t *controle_quadros)
{
  if (self != NULL) {
    if (self->tabela != NULL) {
      for (int i = 0; i < self->tam_tab; i++) {
        if (self->tabela[i].valida) {
          controle_quadros_libera(controle_quadros, self->tabela[i].quadro);
        }
      }
      free(self->tabela);
    }
    free(self);
  }
}

// retorna true se a página for válida (pode ser traduzida em um quadro)
bool tabpag__pagina_valida(tabpag_t *self, int pagina)
{ 
  console_printf("pagina: %d tamanho tabela: %d\n", pagina, self->tam_tab);
  if (pagina < 0 || pagina >= self->tam_tab) return false;
  return self->tabela[pagina].valida;
}

void tabpag_invalida_pagina(tabpag_t *self, int pagina, controle_quadros_t *controle_quadros)
{
  // página já é inválida -- não faz nada
  if (!tabpag__pagina_valida(self, pagina)) return;
  //libera quadro
  int quadro = self->tabela[pagina].quadro;
  controle_quadros_libera(controle_quadros, quadro);
  // página não é a última da tabela -- marca como inválida
  if (pagina < self->tam_tab - 1) {
    self->tabela[pagina].valida = false;
    return;
  }
  // última página na tabela -- reduz a tabela até que a última seja válida
  do {
    self->tam_tab--;
  } while (self->tam_tab > 0 && !self->tabela[self->tam_tab - 1].valida);
  if (self->tam_tab == 0) {
    free(self->tabela);
    self->tabela = NULL;
  } else {
    self->tabela = realloc(self->tabela, self->tam_tab * sizeof(descritor_t));
    assert(self->tabela != NULL);
  }
}

// aumenta a tabela, se necessário, para que contenha 'pagina'
static void tabpag__insere_pagina(tabpag_t *self, int pagina)
{
  if (pagina < self->tam_tab) return;
  int novo_tam = pagina + 1;
  if (self->tam_tab == 0) {
    self->tabela = malloc(novo_tam * sizeof(descritor_t));
  } else {
    self->tabela = realloc(self->tabela, novo_tam * sizeof(descritor_t));
  }
  assert(self->tabela != NULL);
  // marca as páginas inseridas como não válidas
  while (self->tam_tab < novo_tam) {
    self->tabela[self->tam_tab].valida = false;
    self->tam_tab++;
  }
}

void tabpag_define_quadro(tabpag_t *self, int pagina, int quadro, controle_quadros_t *controle_quadros)
{
  assert(pagina >= 0);
  if(quadro == -1){
    quadro = controle_quadros_aloca(controle_quadros);
    if(quadro == -1){
      //nenhum quadro livre
    }
  }
  tabpag__insere_pagina(self, pagina);
  self->tabela[pagina].quadro = quadro;
  self->tabela[pagina].valida = true;
  self->tabela[pagina].acessada = false;
  self->tabela[pagina].alterada = false;
}

void tabpag_marca_bit_acesso(tabpag_t *self, int pagina, bool alteracao)
{
  if (!tabpag__pagina_valida(self, pagina)) return;
  self->tabela[pagina].acessada = true;
  if (alteracao) {
    self->tabela[pagina].alterada = true;
  }
}

void tabpag_zera_bit_acesso(tabpag_t *self, int pagina)
{
  if (!tabpag__pagina_valida(self, pagina)) return;
  self->tabela[pagina].acessada = false;
}

bool tabpag_bit_acesso(tabpag_t *self, int pagina)
{
  if (!tabpag__pagina_valida(self, pagina)) return false;
  return self->tabela[pagina].acessada;
}

bool tabpag_bit_alteracao(tabpag_t *self, int pagina)
{
  if (!tabpag__pagina_valida(self, pagina)) return false;
  return self->tabela[pagina].alterada;
}

err_t tabpag_traduz(tabpag_t *self, int pagina, int *pquadro)
{
  if (!tabpag__pagina_valida(self, pagina)) return ERR_PAG_AUSENTE;
  *pquadro = self->tabela[pagina].quadro;

  return ERR_OK;
}

int tabpag_encontra_pagina(tabpag_t *self, int quadro_fisico) {
    for (int i = 0; i < self->tam_tab; i++) {
        if (self->tabela[i].valida && self->tabela[i].quadro == quadro_fisico) {
            return i; 
        }
    }
    return -1; 
}

int tabpag_pagina_modificada(tabpag_t *self, int quadro_fisico) {
    for (int i = 0; i < self->tam_tab; i++) {
        if (self->tabela[i].alterada && self->tabela[i].quadro == quadro_fisico) {
            return i; 
        }
    }
    return -1; 
}

void tabpag_invalida_quadro(tabpag_t *self, int quadro, controle_quadros_t *controle_quadros) {
    int pagina = tabpag_encontra_pagina(self, quadro); 
    if (pagina != -1) {
        tabpag_invalida_pagina(self, pagina, controle_quadros);
    }
}


//getters

int tabpag__pagina_quadro(tabpag_t *self, int pagina)
{
  if (pagina < 0 || pagina >= self->tam_tab) return -1;
  return self->tabela[pagina].quadro;
}