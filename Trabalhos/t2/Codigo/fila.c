#include <queue>

typedef struct {
  std::queue<int> fila_paginas; 
} fifo_t;

typedef struct {
  std::queue<int> fila_paginas; 
} segunda_chance_t;


fifo_t* fifo_cria() {
  return (fifo_t*)malloc(sizeof(fifo_t));
}

void fifo_destroi(fifo_t* fifo) {
  if (fifo) {
    free(fifo);
  }
}

int fifo_seleciona_pagina(fifo_t* fifo, controle_quadros_t* controle_quadros) {
  if (fifo->fila_paginas.empty()) {
    return -1; 
  }
  int pagina = fifo->fila_paginas.front();
  fifo->fila_paginas.pop();
  return pagina;
}

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

int segunda_chance_seleciona_pagina(segunda_chance_t* sc, tabpag_t* tabela) {
  while (!sc->fila_paginas.empty()) {
    int pagina = sc->fila_paginas.front();
    sc->fila_paginas.pop();

    if (tabpag_bit_acesso(tabela, pagina)) {
      tabpag_zera_bit_acesso(tabela, pagina);
      sc->fila_paginas.push(pagina); 
    } else {
      return pagina; 
    }
  }
  return -1; 
}

void segunda_chance_adiciona_pagina(segunda_chance_t* sc, int pagina) {
  sc->fila_paginas.push(pagina);
}
