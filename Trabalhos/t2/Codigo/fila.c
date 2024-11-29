#include <queue>

// Estrutura para gerenciar as páginas em memória com FIFO
typedef struct {
  std::queue<int> fila_paginas; // Fila para rastrear a ordem das páginas
} fifo_t;

// Estrutura para gerenciar páginas usando Segunda Chance
typedef struct {
  std::queue<int> fila_paginas; // Fila de páginas
} segunda_chance_t;


fifo_t* fifo_cria() {
  return (fifo_t*)malloc(sizeof(fifo_t));
}

void fifo_destroi(fifo_t* fifo) {
  if (fifo) {
    free(fifo);
  }
}

// Escolhe uma página para substituição usando FIFO
int fifo_seleciona_pagina(fifo_t* fifo, controle_quadros_t* controle_quadros) {
  if (fifo->fila_paginas.empty()) {
    return -1; // Nenhuma página em memória
  }
  int pagina = fifo->fila_paginas.front();
  fifo->fila_paginas.pop();
  return pagina;
}

// Adiciona uma nova página na fila FIFO
void fifo_adiciona_pagina(fifo_t* fifo, int pagina) {
  fifo->fila_paginas.push(pagina);
}

segunda_chance_t* segunda_chance_cria() {
  return (segunda_chance_t*)malloc(sizeof(segunda_chance_t));
}

void segunda_chance_destroi(segunda_chance_t* sc) {
  if (sc) {
    free(sc);
  }
}

// Escolhe uma página para substituir usando Segunda Chance
int segunda_chance_seleciona_pagina(segunda_chance_t* sc, tabpag_t* tabela) {
  while (!sc->fila_paginas.empty()) {
    int pagina = sc->fila_paginas.front();
    sc->fila_paginas.pop();

    if (tabpag_bit_acesso(tabela, pagina)) {
      tabpag_zera_bit_acesso(tabela, pagina);
      sc->fila_paginas.push(pagina); // Move para o final da fila
    } else {
      return pagina; // Página escolhida para substituição
    }
  }
  return -1; // Nenhuma página disponível
}

// Adiciona uma nova página à fila de Segunda Chance
void segunda_chance_adiciona_pagina(segunda_chance_t* sc, int pagina) {
  sc->fila_paginas.push(pagina);
}
