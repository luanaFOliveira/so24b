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


## Testes de desempenho

Os testes foram feitos alterando o escalonador prioritário para analisar algumas metricas

Neste primeiro teste foram utilizados os seguintes parâmetros:
### Parametros gerais:
- Quantum: 10
- Intervalo de Interrupções: 100
- Tempo de Transferência (simulado): 1
- Total de Quadros: 20
- Utilizamos primeiro o Algoritmo de Substituição: FIFO

### Primeiro teste

- Tamanho da Página: 12
- Tamanho da Memória Secundária: 60000 (um valor consideravelmente grande)

---------------------------------
#### Metricas gerais

- **Número de processos criados**: 4  
- **Tempo total de execução**: 28790.00 ms  
- **Tempo total ocioso**: 9697.00 ms  
- **Número total de preempções**: 6

#### Interrupções por IRQ:
- **IRQ 0**: 1 vez  
- **IRQ 1**: 77 vezes  
- **IRQ 2**: 462 vezes  
- **IRQ 3**: 287 vezes  
- **IRQ 4**: 0 vezes  
- **IRQ 5**: 0 vezes  


#### Metricas gerais para cada processo

| PID | Tempo de Retorno | Preempções | Tempo Médio de Resposta | Numero de page fault |
|-----|------------------|------------|-----------------|-----------------|
| 1   | 28790.00 ms        | 0          | 0 ms        | 14               |
| 2   | 14940.00 ms         | 6          | 151 ms        | 21               |
| 3   | 13569.00 ms         | 0          | 224 ms        | 21               |
| 4   | 27390.00 ms        | 0          | 75 ms        |  21               |

#### Metricas de tempo em cada estado para cada processo

| PID | Tempo em execução  | Tempo pronto | Tempo bloqueado | Tempo morto |
|-----|------------------|------------|-----------------|-----------------|
| 1   | 766 ms        | 0 ms         | 28024 ms        | 0 ms        |
| 2   | 8976 ms         | 5302 ms          | 662 ms        | 12843 ms        |
| 3   | 3735 ms         | 8323 ms         | 1511 ms        | 14198 ms        |
| 4   | 5616 ms        | 11725 ms          | 10049 ms        | 360 ms        | 

#### Metricas de numero de vezes em cada estado para cada processo

| PID |EM_EXECUCAO  | PRONTO | BLOQUEADO | MORTO |
|-----|------------------|------------|-----------------|-----------------|
| 1   | 17 vezes        | 17 vezes         | 16 vezes        | 1 vezes      |
| 2   | 35 vezes         | 35 vezes          | 28 vezes        | 1 vezes        |
| 3   | 37 vezes         | 37 vezes          | 36 vezes        | 1 vezes       |
| 4   | 155 vezes        | 155 vezes          | 154 vezes        | 1 vezes        | 


---

### Segundo teste

- Tamanho da Página: 3
- Tamanho da Memória Secundária: 60000 (um valor consideravelmente grande)

---------------------------------
#### Metricas gerais

- **Número de processos criados**: 4  
- **Tempo total de execução**: 34631.00 ms  
- **Tempo total ocioso**: 14745.00 ms  
- **Número total de preempções**: 6

#### Interrupções por IRQ:
- **IRQ 0**: 1 vez  
- **IRQ 1**: 276 vezes  
- **IRQ 2**: 462 vezes  
- **IRQ 3**: 346 vezes  
- **IRQ 4**: 0 vezes  
- **IRQ 5**: 0 vezes  


#### Metricas gerais para cada processo

| PID | Tempo de Retorno | Preempções | Tempo Médio de Resposta | Numero de page fault |
|-----|------------------|------------|-----------------|-----------------|
| 1   | 34631.00 ms        | 0          | 1 ms        | 46               |
| 2   | 13783.00 ms         | 6          | 41 ms        | 77               |
| 3   | 14827.00 ms         | 0          | 95 ms        | 76               |
| 4   | 31358.00 ms        | 0          | 57 ms        |  77               |

#### Metricas de tempo em cada estado para cada processo

| PID | Tempo em execução  | Tempo pronto | Tempo bloqueado | Tempo morto |
|-----|------------------|------------|-----------------|-----------------|
| 1   | 885 ms        | 91 ms         | 33655 ms        | 0 ms        |
| 2   | 9200 ms         | 3636 ms          | 947 ms        | 18543 ms        |
| 3   | 3955 ms         | 8815 ms         | 2057 ms        | 17457 ms        |
| 4   | 5846 ms        | 12325 ms          | 13187 ms        | 900 ms        | 

#### Metricas de numero de vezes em cada estado para cada processo

| PID |EM_EXECUCAO  | PRONTO | BLOQUEADO | MORTO |
|-----|------------------|------------|-----------------|-----------------|
| 1   | 50 vezes        | 50 vezes         | 49 vezes        | 1 vezes      |
| 2   | 88 vezes         | 88 vezes          | 81 vezes        | 1 vezes        |
| 3   | 92 vezes         | 92 vezes          | 91 vezes        | 1 vezes       |
| 4   | 214 vezes        | 214 vezes          | 213 vezes        | 1 vezes        | 


---

### Terceiro teste

- Tamanho da Página: 3
- Tamanho da Memória Secundária: 6000 (um valor consideravelmente pequeno)

---------------------------------
#### Metricas gerais

- **Número de processos criados**: 4  
- **Tempo total de execução**: 34631.00 ms  
- **Tempo total ocioso**: 14745.00 ms  
- **Número total de preempções**: 6

#### Interrupções por IRQ:
- **IRQ 0**: 1 vez  
- **IRQ 1**: 276 vezes  
- **IRQ 2**: 462 vezes  
- **IRQ 3**: 346 vezes  
- **IRQ 4**: 0 vezes  
- **IRQ 5**: 0 vezes  


#### Metricas gerais para cada processo

| PID | Tempo de Retorno | Preempções | Tempo Médio de Resposta | Numero de page fault |
|-----|------------------|------------|-----------------|-----------------|
| 1   | 34631.00 ms        | 0          | 1 ms        | 46               |
| 2   | 13783.00 ms         | 6          | 41 ms        | 77               |
| 3   | 14827.00 ms         | 0          | 95 ms        | 76               |
| 4   | 31358.00 ms        | 0          | 57 ms        |  77               |

#### Metricas de tempo em cada estado para cada processo

| PID | Tempo em execução  | Tempo pronto | Tempo bloqueado | Tempo morto |
|-----|------------------|------------|-----------------|-----------------|
| 1   | 885 ms        | 91 ms         | 33655 ms        | 0 ms        |
| 2   | 9200 ms         | 3636 ms          | 947 ms        | 18543 ms        |
| 3   | 3955 ms         | 8815 ms         | 2057 ms        | 17457 ms        |
| 4   | 5846 ms        | 12325 ms          | 13187 ms        | 900 ms        | 

#### Metricas de numero de vezes em cada estado para cada processo

| PID |EM_EXECUCAO  | PRONTO | BLOQUEADO | MORTO |
|-----|------------------|------------|-----------------|-----------------|
| 1   | 50 vezes        | 50 vezes         | 49 vezes        | 1 vezes      |
| 2   | 88 vezes         | 88 vezes          | 81 vezes        | 1 vezes        |
| 3   | 92 vezes         | 92 vezes          | 91 vezes        | 1 vezes       |
| 4   | 214 vezes        | 214 vezes          | 213 vezes        | 1 vezes        | 


---



## Conclusão a Analise

### Tempo Total de Execução

O tempo total de execução dos processos variou significativamente entre os testes.
**Primeiro teste**:
    - O tempo total de execução foi 28.790 ms.

**Segundo teste**:
    -O tempo total de execução foi 34.631 ms.

**Terceiro teste** :
  	-  O tempo total de execução também foi 34.631 ms.

**Conclusão**:
- O tempo total de execução aumentou significativamente do primeiro para o segundo e terceiro testes.
- O aumento foi influenciado principalmente pela redução do tamanho da página, que elevou o número de page faults (falhas de página), aumentando o tempo de execução.
- Entre o segundo e o terceiro testes, mesmo com uma memória secundária menor no terceiro teste, o tempo de execução não mudou, indicando que a limitação da memória secundária não teve impacto significativo.

### Impacto do Tamanho da Página:
**Primeiro Teste (Página = 12)**:

- Menor tempo total de execução: 28.790 ms.
- Menor número de page faults (14–21 por processo).
- O tamanho maior da página permitiu uma alocação mais eficiente, reduzindo as falhas e interrupções.
  
**Segundo e Terceiro Testes (Página = 3)**:

- Tempo total de execução maior: 34.631 ms.
- Maior número de page faults (46–77 por processo).
- A redução no tamanho da página aumentou drasticamente o número de falhas de página, impactando a eficiência do sistema.
- Impacto da Memória Secundária:
- A redução da memória secundária no terceiro teste (de 60.000 para 6.000) não impactou o tempo total de execução, sugerindo que a quantidade de memória secundária ainda era suficiente para o comportamento do sistema neste cenário.

### Tempo Ocioso:
O tempo total ocioso aumentou significativamente nos testes com páginas menores:

**Primeiro Teste**: 9.697 ms.

**Segundo e Terceiro Testes**: 14.745 ms.

O aumento do tempo ocioso está associado ao maior tempo gasto em falhas de página e ao tempo bloqueado dos processos.

### Utilização da CPU:

Os processos passaram mais tempo em estado bloqueado no segundo e terceiro testes, como visto nas métricas:

Exemplo: Processo 1 teve 33.655 ms bloqueado no segundo e terceiro testes, contra 28.024 ms no primeiro teste.

### Conclusões Finais:
- Tamanho da Página tem um impacto significativo na eficiência do sistema:
- Páginas maiores (Teste 1) reduziram falhas de página e melhoraram o desempenho geral.
- Páginas menores (Testes 2 e 3) aumentaram falhas de página, interrupções e tempo bloqueado, prejudicando o desempenho.
- Tempo Ocioso cresce com a quantidade de falhas de página, o que afeta diretamente o tempo total de execução.

