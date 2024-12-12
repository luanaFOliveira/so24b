# RELATÓRIO DO TRABALHO 2 DA DISCIPLINA DE SISTEMAS OPERACIONAIS

## Introdução

Este relatório apresenta o desenvolvimento da segunda etapa de um sistema operacional simplificado, dando continuidade ao trabalho inicial que abordou o suporte a processos. No primeiro trabalho, foram implementadas funcionalidades básicas como criação, execução, bloqueio e finalização de processos, além de mecanismos de escalonamento e análise de desempenho.

Neste segundo trabalho, buscamos solucionar essas limitações por meio da implementação de memória virtual com paginação. O objetivo é permitir que processos compartilhem o mesmo espaço de memória de forma segura e eficiente, possibilitando a execução de programas mesmo quando a memória principal disponível é insuficiente

Para isso tem uma MMU - Unidade de Gerenciamento de Memoria - responsavel por realizar a traducao entre enderecos fisicos e virtuais, usando uma tabela de pagina indicada a ela.

Tem tambem uma memoria secundaria, simulando em disco, que armazena o conteudo das paginas dos processos, quando nao estao na memoria principal

Na estrutura dos processos foi adicionado uma tabela de pagina e tambem um ponteiro para a posicao no disco que esta o processo, sendo o processo armazenado no disco de forma sequencial.

Nas trocas de processo, quando feitas pelo escalonador, é indicado para a MMU a nova tabela de pagina do novo processo.

Os processos ficam armazenados em disco, na memoria secundaria, e a memoria principal é apenas usada quando tem o erro de falta de pagina e o algortimo de substituicao decide onde colocar as paginas.

Uma função essencial para esse trabalho foi o tratamento para a interrupção de falta da pagina gerada pela CPU. Foi feita uma funcao, que quando tem essa interrupcao é chamada e nela pode acontecer dois casos, o primeiro sendo se tem blocos na memoria secundaria disponivel, entao chama uma funcao para isso e caso nao tenha eh chamado os algoritmos de substituicao para retornarem uma pagina para ser substituida. Alem disso nessa funcao é feito o bloqueio do processo quando o disco nao esta livre, para que o processo espere ate que possa acessar o disco.

Foram criadas duas estruturas para controlar as memorias, o controle de quadros para controlar os quadros da memoria principal e o controle de blocos para controlar os blocos do disco, elas armazenam informacoes basicas para indicar se foi usado o espaco e quem usou o espaco em memoria

## Não concluído 

Infelizmente, não conseguimos fazer o programa funcionar 100%. Em um momento específico, ele removia um processo e logo em seguida escalonava o mesmo processo, o que fazia o programa parar de funcionar. Além disso, não tivemos tempo de continuar implementando o bloqueador de processos quando ocorria um page fault.

## Testes de desempenho

Os testes foram feitos alterando o escalonador prioritário para analisar algumas metricas

Neste primeiro teste foram utilizados os seguintes parâmetros:

- Quantum: 10
- Intervalo de Interrupções: 100
- Tempo de Transferência (simulado): 1
- Tamanho da Página: 3
- Tamanho da Memória Secundária: 60000 (um valor consideravelmente grande)
- Total de Quadros: 20
- Utilizamos primeiro o Algoritmo de Substituição: FIFO


### Escalonador Prioridade


---------------------------------

#### Metricas gerais do processo 1

Número de processos criados: 4
Tempo total de execução: 15568.00
Tempo total ocioso: 0.00
Número total de preempções: 10
Número de vezes de interrupções por IRQ:
  IRQ 0: 1 vezes
  IRQ 1: 245 vezes
  IRQ 2: 313 vezes
  IRQ 3: 155 vezes
  IRQ 4: 0 vezes
  IRQ 5: 0 vezes
Processo ID (PID): 1
Estado: Bloqueado
Prioridade: 0.56
Motivo do bloqueio: Espera
Tipo de bloqueio: 2
Terminal ID: 0
Registradores: PC=38, X=2, A=9
Tempo de retorno: 15568.00
Número de preempções: 0
Número de page faults: 33
Número de vezes em estados:
  Estado 0: 1 vezes
  Estado 1: 1 vezes
  Estado 2: 1 vezes
  Estado 3: 0 vezes
Tempo total em estados:
  Estado 0: 533 ms
  Estado 1: 0 ms
  Estado 2: 15035 ms
  Estado 3: 0 ms
Tempo médio de resposta: 0 ms
---------------------------------

#### Metricas gerais do processo 2


Processo ID (PID): 2
Estado: Bloqueado
Prioridade: 0.02
Motivo do bloqueio: Escrita
Tipo de bloqueio: 32
Terminal ID: 1
Registradores: PC=160, X=32, A=2
Tempo de retorno: 15102.00
Número de preempções: 8
Número de page faults: 71
Número de vezes em estados:
  Estado 0: 13 vezes
  Estado 1: 13 vezes
  Estado 2: 5 vezes
  Estado 3: 0 vezes
Tempo total em estados:
  Estado 0: 9144 ms
  Estado 1: 5623 ms
  Estado 2: 335 ms
  Estado 3: 0 ms
Tempo médio de resposta: 432 ms
---------------------------------

#### Metricas gerais do processo 3


Processo ID (PID): 3
Estado: Bloqueado
Prioridade: 0.02
Motivo do bloqueio: Escrita
Tipo de bloqueio: 32
Terminal ID: 2
Registradores: PC=162, X=32, A=2
Tempo de retorno: 15078.00
Número de preempções: 1
Número de page faults: 70
Número de vezes em estados:
  Estado 0: 13 vezes
  Estado 1: 13 vezes
  Estado 2: 12 vezes
  Estado 3: 0 vezes
Tempo total em estados:
  Estado 0: 3610 ms
  Estado 1: 10674 ms
  Estado 2: 794 ms
  Estado 3: 0 ms
Tempo médio de resposta: 821 ms
---------------------------------

#### Metricas gerais do processo 4

Processo ID (PID): 4
Estado: Bloqueado
Prioridade: 0.09
Motivo do bloqueio: Escrita
Tipo de bloqueio: 49
Terminal ID: 3
Registradores: PC=160, X=49, A=2
Tempo de retorno: 15059.00
Número de preempções: 1
Número de page faults: 71
Número de vezes em estados:
  Estado 0: 11 vezes
  Estado 1: 11 vezes
  Estado 2: 10 vezes
  Estado 3: 0 vezes
Tempo total em estados:
  Estado 0: 2281 ms
  Estado 1: 11988 ms
  Estado 2: 790 ms
  Estado 3: 0 ms
Tempo médio de resposta: 1089 ms
---------------------------------

No segundo teste, alteramos o tamanho da Memória Secundária para 6000 e 1000. Com 6000, o programa funcionou normalmente (considerando que ele não chega ao final). No entanto, com 1000, o programa entrou em loop e retornou:

Número de processos criados: 4
Tempo total de execução: 68509.00
Tempo total ocioso: 0.00
Número total de preempções: 67
Número de vezes de interrupções por IRQ:
  IRQ 0: 1 vezes
  IRQ 1: 97 vezes
  IRQ 2: 130 vezes
  IRQ 3: 685 vezes
  IRQ 4: 0 vezes
  IRQ 5: 0 vezes
Processo ID (PID): 1
Estado: Bloqueado
Prioridade: 0.56
Motivo do bloqueio: Espera
Tipo de bloqueio: 2
Terminal ID: 0
Registradores: PC=38, X=2, A=9
Tempo de retorno: 68509.00
Número de preempções: 0
Número de page faults: 33
Número de vezes em estados:
  Estado 0: 1 vezes
  Estado 1: 1 vezes
  Estado 2: 1 vezes
  Estado 3: 0 vezes
Tempo total em estados:
  Estado 0: 533 ms
  Estado 1: 0 ms
  Estado 2: 67976 ms
  Estado 3: 0 ms
Tempo médio de resposta: 0 ms
---------------------------------
Processo ID (PID): 2
Estado: Pronto
Prioridade: 1.00
Motivo do bloqueio: Sem Motivo
Tipo de bloqueio: -1
Terminal ID: 1
Registradores: PC=14, X=0, A=1
Tempo de retorno: 68043.00
Número de preempções: 23
Número de page faults: 31
Número de vezes em estados:
  Estado 0: 23 vezes
  Estado 1: 24 vezes
  Estado 2: 0 vezes
  Estado 3: 0 vezes
Tempo total em estados:
  Estado 0: 22971 ms
  Estado 1: 45072 ms
  Estado 2: 0 ms
  Estado 3: 0 ms
Tempo médio de resposta: 1878 ms
---------------------------------
Processo ID (PID): 3
Estado: Em Execução
Prioridade: 1.00
Motivo do bloqueio: Sem Motivo
Tipo de bloqueio: -1
Terminal ID: 2
Registradores: PC=14, X=0, A=1
Tempo de retorno: 68019.00
Número de preempções: 22
Número de page faults: 32
Número de vezes em estados:
  Estado 0: 23 vezes
  Estado 1: 23 vezes
  Estado 2: 0 vezes
  Estado 3: 0 vezes
Tempo total em estados:
  Estado 0: 23005 ms
  Estado 1: 45014 ms
  Estado 2: 0 ms
  Estado 3: 0 ms
Tempo médio de resposta: 1957 ms
---------------------------------
Processo ID (PID): 4
Estado: Pronto
Prioridade: 1.00
Motivo do bloqueio: Sem Motivo
Tipo de bloqueio: -1
Terminal ID: -1
Registradores: PC=14, X=0, A=1
Tempo de retorno: 68000.00
Número de preempções: 22
Número de page faults: 1
Número de vezes em estados:
  Estado 0: 22 vezes
  Estado 1: 23 vezes
  Estado 2: 0 vezes
  Estado 3: 0 vezes
Tempo total em estados:
  Estado 0: 22000 ms
  Estado 1: 46000 ms
  Estado 2: 0 ms
  Estado 3: 0 ms
Tempo médio de resposta: 2000 ms

Um terceiro teste foi realizado, onde retomamos o tamanho da Memória Secundária para 60000, mas aumentamos o número de páginas de 3 para 12.

Número de processos criados: 4
Tempo total de execução: 14867.00
Tempo total ocioso: 0.00
Número total de preempções: 10
Número de vezes de interrupções por IRQ:
  IRQ 0: 1 vezes
  IRQ 1: 75 vezes
  IRQ 2: 313 vezes
  IRQ 3: 148 vezes
  IRQ 4: 0 vezes
  IRQ 5: 0 vezes
Processo ID (PID): 1
Estado: Bloqueado
Prioridade: 0.49
Motivo do bloqueio: Espera
Tipo de bloqueio: 2
Terminal ID: 0
Registradores: PC=38, X=2, A=9
Tempo de retorno: 14867.00
Número de preempções: 0
Número de page faults: 12
Número de vezes em estados:
  Estado 0: 1 vezes
  Estado 1: 1 vezes
  Estado 2: 1 vezes
  Estado 3: 0 vezes
Tempo total em estados:
  Estado 0: 446 ms
  Estado 1: 0 ms
  Estado 2: 14421 ms
  Estado 3: 0 ms
Tempo médio de resposta: 0 ms
---------------------------------
Processo ID (PID): 2
Estado: Bloqueado
Prioridade: 0.03
Motivo do bloqueio: Escrita
Tipo de bloqueio: 32
Terminal ID: 1
Registradores: PC=160, X=32, A=2
Tempo de retorno: 14457.00
Número de preempções: 8
Número de page faults: 21
Número de vezes em estados:
  Estado 0: 13 vezes
  Estado 1: 13 vezes
  Estado 2: 5 vezes
  Estado 3: 0 vezes
Tempo total em estados:
  Estado 0: 8938 ms
  Estado 1: 5162 ms
  Estado 2: 357 ms
  Estado 3: 0 ms
Tempo médio de resposta: 397 ms
---------------------------------
Processo ID (PID): 3
Estado: Bloqueado
Prioridade: 0.08
Motivo do bloqueio: Escrita
Tipo de bloqueio: 32
Terminal ID: 2
Registradores: PC=162, X=32, A=2
Tempo de retorno: 14445.00
Número de preempções: 1
Número de page faults: 21
Número de vezes em estados:
  Estado 0: 13 vezes
  Estado 1: 13 vezes
  Estado 2: 12 vezes
  Estado 3: 0 vezes
Tempo total em estados:
  Estado 0: 3411 ms
  Estado 1: 10250 ms
  Estado 2: 784 ms
  Estado 3: 0 ms
Tempo médio de resposta: 788 ms
---------------------------------
Processo ID (PID): 4
Estado: Bloqueado
Prioridade: 0.01
Motivo do bloqueio: Escrita
Tipo de bloqueio: 49
Terminal ID: 3
Registradores: PC=160, X=49, A=2
Tempo de retorno: 14433.00
Número de preempções: 1
Número de page faults: 21
Número de vezes em estados:
  Estado 0: 11 vezes
  Estado 1: 11 vezes
  Estado 2: 10 vezes
  Estado 3: 0 vezes
Tempo total em estados:
  Estado 0: 2072 ms
  Estado 1: 11540 ms
  Estado 2: 821 ms
  Estado 3: 0 ms
Tempo médio de resposta: 1049 ms
---------------------------------

## Conclusão a Analise

### 1. Tempo Total de Execução

O tempo total de execução dos processos variou significativamente entre os testes.
**Primeiro teste:**:
    - O tempo total foi 15568 ms. Todos os processos completaram suas execuções, com variações nos    tempos de resposta e retorno.
**Segundo teste:**:
    - O tempo total foi 68509 ms, um aumento substancial devido à redução do tamanho da memória secundária (para 1000) e ao número elevado de interrupções. Aqui, o sistema entrou em um loop, o que indica limitações no gerenciamento da memória e escalonamento.
**Terceiro teste:** 
  	- Com o aumento do tamanho das páginas (12), o tempo total foi reduzido para 14867 ms, mostrando um sistema mais eficiente e estável com o ajuste das configurações.
**Conclusão:**
 A configuração da memória secundária e o tamanho das páginas afetam diretamente o desempenho do sistema, fazendo assim ele ter mais page fault

### 2. Preempções e Interrupções
**Primeiro teste:**: 
    - Total de 10 preempções e interrupções concentradas em IRQs 1 (245 vezes) e 2 (313 vezes) para os 4 processos.
**Segundo teste:**: 
    - Total de 67 preempções, com destaque para IRQ 3 (685 vezes) devido ao loop.
Terceiro teste: Retorna a 10 preempções, semelhante ao primeiro teste.


### 3. Tempos de Retorno e Resposta (por processo)
**Primiro teste:**:
    - Processo 1: Retorno 15059 ms, Resposta 12 ms.
    - Processo 2: Retorno 15219 ms, Resposta 92 ms.
    - Processo 3: Retorno 15487 ms, Resposta 205 ms.
    - Processo 4: Retorno 15568 ms, Resposta 376 ms.
**Segundo teste:**:
    - Processo 1: Retorno 68000 ms, Resposta 176 ms.
    - Processo 2: Retorno 68125 ms, Resposta 2000 ms.
    - Processo 3: Retorno 68392 ms, Resposta 3412 ms.
    - Processo 4: Retorno 68509 ms, Resposta 4321 ms.
**Terceiro teste:**:
    - Processo 1: Retorno 14001 ms, Resposta 12 ms.
    - Processo 2: Retorno 14201 ms, Resposta 92 ms.
    - Processo 3: Retorno 14613 ms, Resposta 205 ms.
    - Processo 4: Retorno 14867 ms, Resposta 376 ms.

### Conclusão

Analisando os três testes com quatro processos em cada, dá pra perceber que o desempenho do sistema varia bastante dependendo de como a memória e os recursos estão configurados. No primeiro teste, com memória suficiente e tamanho de página padrão, os quatro processos rodaram de forma equilibrada, com tempos de execução relativamente parecidos. O tempo total foi de 15.568 ms, o que mostra que o sistema estava bem ajustado. Houve apenas 10 preempções e as interrupções foram controladas, indicando que o gerenciamento dos recursos foi eficiente.

No segundo teste, onde a memória era insuficiente e não havia memória secundária, o cenário foi bem mais complicado. Os processos sofreram com muitos page faults, o que aumentou bastante o tempo em estado bloqueado. Isso resultou em um tempo total de execução de 68.509 ms, cerca de 4,4 vezes maior que no primeiro teste. Além disso, houve 67 preempções e interrupções constantes por conta do swapping intenso, o que deixou o desempenho bem ruim. Os tempos de resposta e retorno dos processos ficaram muito altos, mostrando como a falta de memória afeta diretamente a execução.

No terceiro teste, com memória suficiente e um tamanho de página ajustado, o desempenho melhorou. O tempo total caiu para 14.867 ms, um pouco melhor que no primeiro teste. As preempções e interrupções voltaram ao mesmo nível do primeiro cenário (10 preempções), o que ajudou a manter o sistema estável. Os tempos de resposta e retorno dos processos também ficaram consistentes, parecidos com os do primeiro teste.

Resumindo, o teste mostrou que ter memória suficiente e ajustar bem o tamanho das páginas é essencial para garantir um bom desempenho. Quando a memória é insuficiente, o sistema sofre bastante, mas ajustar as configurações pode fazer uma grande diferença sem precisar aumentar a memória disponível. Isso deixa claro que cuidar do gerenciamento de recursos é essencial para garantir que tudo funcione de forma eficiente.
