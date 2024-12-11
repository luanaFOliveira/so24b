# RELATÓRIO DO TRABALHO 1 DA DISCIPLINA DE SISTEMAS OPERACIONAIS

## Introdução

Este relatório apresenta o desenvolvimento de um sistema operacional simplificado, abordando conceitos fundamentais como gerenciamento de processos, escalonamento e análise de desempenho. O objetivo é implementar funcionalidades que simulem o funcionamento de um sistema operacional, permitindo a criação, execução, bloqueio e finalização de processos, além de implementar mecanismos de escalonamento e coleta de métricas para avaliação do sistema.

Para representar os processos, foi criada uma estrutura. O estado de um processo é definido por um enum com quatro estados possíveis: em execução, pronto, bloqueado ou morto. Além disso, o processo possui um atributo para indicar o motivo do bloqueio, que pode ser determinado por um enum com quatro razões: bloqueio por leitura, por escrita, por espera ou sem motivo. O sistema operacional adota comportamentos diferentes dependendo do tipo de bloqueio.

A estrutura do processo também armazena o identificador da porta que está sendo utilizada para operações de entrada e saída, além de conter os registradores do processador, como o PC, A e X, necessários para salvar e restaurar o contexto do processo.

Para registrar as métricas de cada processo, a estrutura também armazena informações como o tempo de retorno, o número de preempções, a quantidade de vezes que o processo esteve em cada estado, o tempo total em cada estado e o tempo médio de resposta do processo.

Na classe de processos, foram implementadas funções para realizar as operações básicas de obtenção (get) e modificação (set) dos atributos de um processo. Além disso, a classe conta com métodos para realizar operações como bloquear, desbloquear, executar, encerrar e pausar um processo. Também foram desenvolvidas funções auxiliares para alterar o estado de um processo, realizar verificações e armazenar métricas relacionadas ao seu comportamento. Por fim, há uma função específica para calcular as métricas de desempenho de um processo.

Para representar o escalonador, foi criada uma estrutura que suporta os três tipos de escalonamento: simples, circular e por prioridade. A estrutura utiliza uma lista circular para armazenar os processos, mantendo o controle tanto do início quanto do fim da lista. Cada nó da lista contém um processo e um ponteiro para o próximo nó, permitindo a navegação entre os processos.

Os processos precisam acessar portas para realizar operações de leitura e escrita. Para garantir que esses acessos ocorram corretamente, é necessário verificar se a tela está pronta para receber dados e se o teclado está disponível para enviar informações. Para isso, na estrutura de controle_es armazena o status da tela e do teclado, além de um indicador que mostra se a porta está em uso ou não. Cada processo possui um identificador de porta, o qual indica a porta que está sendo utilizada.

Nos três tipos de escalonadores, a adição e remoção de processos da lista seguem o mesmo procedimento: o processo é inserido no final da lista e, para removê-lo, é excluído apenas o processo específico da lista. A principal diferença entre os escalonadores está na função responsável por escolher o próximo processo a ser executado.

### Escalonador Simples
No escalonador simples, o próximo processo a ser executado é sempre o primeiro processo pronto encontrado na lista. Esse processo permanece em execução até que seja bloqueado ou finalize sua execução.

### Escalonador Round-Robin

No escalonador round-robin, o próximo processo a ser executado também é o primeiro processo pronto encontrado na lista. No entanto, se o tempo de execução do processo exceder um quantum pré-definido, ele é interrompido e movido para o final da fila. Isso permite a alternância entre os processos, garantindo que todos tenham a oportunidade de executar.

### Escalonador por Prioridade

Para o escalonador com prioridades, ao buscar o próximo processo para ser executado, ele seleciona o processo pronto com a maior prioridade. A prioridade de um processo é ajustada dinamicamente: quando um processo para de executar, seja por bloqueio ou por exceder o tempo de quantum, sua prioridade é recalculada. Esse ajuste favorece processos que realizam operações rápidas.


## Testes de desempenho

Os testes foram feitos alterando o tipo de escalonador para analisar algumas metricas, para todos os testes foram usados um quantum igual a 5

### Escalonador Simples

Número de processos criados: 4
Tempo total de execução: 24049.00
Tempo total ocioso: 4610.00
Número total de preempções: 56
Número de vezes de interrupções por IRQ:
  IRQ 0: 1 vezes
  IRQ 1: 0 vezes
  IRQ 2: 462 vezes
  IRQ 3: 479 vezes
  IRQ 4: 0 vezes
  IRQ 5: 0 vezes

#### Metricas gerais para cada processo

| PID | Tempo de Retorno | Preempções | Tempo Médio de Resposta |
|-----|------------------|------------|-----------------|
| 1   | 24049.00 ms        | 0          | 0 ms        |
| 2   | 17097.00 ms         | 39          | 152 ms        |
| 3   | 11989.00 ms         | 12          | 284 ms        |
| 4   | 23331.00 ms        | 5          | 96 ms        | 

#### Metricas de tempo em cada estado para cada processo

| PID | Tempo em execução  | Tempo pronto | Tempo bloqueado | Tempo morto |
|-----|------------------|------------|-----------------|-----------------|
| 1   | 743 ms        | 0 ms         | 23306 ms        | 0 ms        |
| 2   | 9198 ms         | 7307 ms          | 592 ms        | 6569 ms        |
| 3   | 3756 ms         | 7403 ms         | 830 ms        | 11669 ms        |
| 4   | 5742 ms        | 10610 ms          | 6979 ms        | 319 ms        | 

#### Metricas de numero de vezes em cada estado para cada processo

| PID |EM_EXECUCAO  | PRONTO | BLOQUEADO | MORTO |
|-----|------------------|------------|-----------------|-----------------|
| 1   | 3 vezes        | 3 vezes         | 2 vezes        | 1 vezes      |
| 2   | 48 vezes         | 48 vezes          | 8 vezes        | 1 vezes        |
| 3   | 26 vezes         | 26 vezes          | 13 vezes        | 1 vezes       |
| 4   | 110 vezes        | 110 vezes          | 104 vezes        | 1 vezes        | 

### Escalonador Round-Robin

Número de processos criados: 4
Tempo total de execução: 24049.00
Tempo total ocioso: 4610.00
Número total de preempções: 56
Número de vezes de interrupções por IRQ:
  IRQ 0: 1 vezes
  IRQ 1: 0 vezes
  IRQ 2: 462 vezes
  IRQ 3: 479 vezes
  IRQ 4: 0 vezes
  IRQ 5: 0 vezes

#### Metricas gerais para cada processo

| PID | Tempo de Retorno | Preempções | Tempo Médio de Resposta |
|-----|------------------|------------|-----------------|
| 1   | 24049.00 ms        | 0          | 0 ms        |
| 2   | 17097.00 ms         | 39          | 152 ms        |
| 3   | 11989.00 ms         | 12          | 284 ms        |
| 4   | 23331.00 ms        | 5          | 96 ms        | 

#### Metricas de tempo em cada estado para cada processo

| PID | Tempo em execução  | Tempo pronto | Tempo bloqueado | Tempo morto |
|-----|------------------|------------|-----------------|-----------------|
| 1   | 743 ms        | 0 ms         | 23306 ms        | 0 ms        |
| 2   | 9198 ms         | 7307 ms          | 592 ms        | 6569 ms        |
| 3   | 3756 ms         | 7403 ms         | 830 ms        | 11669 ms        |
| 4   | 5742 ms        | 10610 ms          | 6979 ms        | 319 ms        | 

#### Metricas de numero de vezes em cada estado para cada processo

| PID |EM_EXECUCAO  | PRONTO | BLOQUEADO | MORTO |
|-----|------------------|------------|-----------------|-----------------|
| 1   | 3 vezes        | 3 vezes         | 2 vezes        | 1 vezes      |
| 2   | 48 vezes         | 48 vezes          | 8 vezes        | 1 vezes        |
| 3   | 26 vezes         | 26 vezes          | 13 vezes        | 1 vezes       |
| 4   | 110 vezes        | 110 vezes          | 104 vezes        | 1 vezes        | 

### Escalonador Prioridade

Número de processos criados: 4
Tempo total de execução: 23496.00
Tempo total ocioso: 4045.00
Número total de preempções: 59
Número de vezes de interrupções por IRQ:
  IRQ 0: 1 vezes
  IRQ 1: 0 vezes
  IRQ 2: 462 vezes
  IRQ 3: 468 vezes
  IRQ 4: 0 vezes
  IRQ 5: 0 vezes

#### Metricas gerais para cada processo

| PID | Tempo de Retorno | Preempções | Tempo Médio de Resposta |
|-----|------------------|------------|-----------------|
| 1   | 23496.00 ms        | 2          | 20 ms        |
| 2   | 17550.00 ms         | 40          | 158 ms        |
| 3   | 10180.00 ms         | 12          | 206 ms        |
| 4   | 22678.00 ms        | 5          | 99 ms        | 

#### Metricas de tempo em cada estado para cada processo

| PID | Tempo em execução  | Tempo pronto | Tempo bloqueado | Tempo morto |
|-----|------------------|------------|-----------------|-----------------|
| 1   | 743 ms        | 100 ms         | 22653 ms        | 0 ms        |
| 2   | 9216 ms         | 7788 ms          | 546 ms        | 5513 ms        |
| 3   | 3771 ms         | 5380 ms         | 1029 ms        | 12875 ms        |
| 4   | 5721 ms        | 10457 ms          | 6500 ms        | 369 ms        | 

#### Metricas de numero de vezes em cada estado para cada processo

| PID |EM_EXECUCAO  | PRONTO | BLOQUEADO | MORTO |
|-----|------------------|------------|-----------------|-----------------|
| 1   | 5 vezes        | 5 vezes         | 2 vezes        | 1 vezes      |
| 2   | 49 vezes         | 49 vezes          | 8 vezes        | 1 vezes        |
| 3   | 26 vezes         | 26 vezes          | 13 vezes        | 1 vezes       |
| 4   | 105 vezes        | 105 vezes          | 99 vezes        | 1 vezes        | 

## Conclusão a Analise

### 1. Tempo Total de Execução

O escalonador de Prioridade foi o mais eficiente em termos de tempo total de execução (23496 ms), reduzindo 553 ms em relação aos outros dois (Simples e Round-Robin), que levaram 24049 ms cada.

Isso indica que a priorização permitiu uma melhor utilização dos recursos de CPU.

### 2. Tempo Total Ocioso

O escalonador de Prioridade também apresentou o menor tempo ocioso (4045 ms), sugerindo maior aproveitamento dos ciclos de CPU.

Tanto o Simples quanto o Round-Robin apresentaram o mesmo tempo ocioso (4610 ms)

### 3. Preempções

O escalonador de Prioridade teve o maior número de preempções (59).

Simples e Round-Robin apresentaram 56 preempções cada.

### 4. Respostas a Interrupções

O comportamento de resposta a interrupções foi semelhante entre os escalonadores, com pequenas diferenças nos números de IRQ 2 e IRQ 3. O escalonador de Prioridade respondeu menos ao IRQ 3 (468 vezes), sugerindo menor dependência de eventos periódicos.

### 5. Tempo de retorno e de resposta

O escalonador de Prioridade destacou-se ao reduzir o tempo médio de resposta de processos

Para processos com maior prioridade, o tempo de retorno foi menor no escalonador de Prioridade.

### 5. Tempo nos estados

#### Tempo em execução:

Os tempos são muito próximos entre os escalonadores, mas o escalonador de Prioridade mostra maior consistência em manter processos importantes em execução.

#### Tempo pronto e bloqueado:

O escalonador de Prioridade apresentou menor tempo bloqueado para os processos prioritários, o que reflete a redução do tempo total de execução.

### Conclusão

A análise mostrou que os escalonadores têm comportamentos distintos dependendo da carga e das características dos processos. O escalonador de Prioridade mostrou ser o mais eficiente em cenários com alta concorrência e prioridades definidas, enquanto os outros dois são mais adequados para sistemas mais simples e previsíveis.


