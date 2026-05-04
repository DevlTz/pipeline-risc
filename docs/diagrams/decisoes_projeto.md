# Justificativa de Projeto: Processador RISC Pipeline

Este documento detalha as decisões de projeto tomadas na implementação do processador em SystemC, atendendo aos requisitos da disciplina de Organização de Computadores.

## 1. Arquitetura de Propósito Geral

Diferente de uma Parte Operativa (PO) dedicada, que é projetada para um único algoritmo, este projeto utiliza o modelo **PO/PC Programável (Propósito Geral)**.
- **Flexibilidade:** O hardware é genérico e o comportamento é definido pelo software (código de máquina) carregado na Memória de Instruções.
- **Componentes:** Inclui Banco de Registradores, ULA de propósito geral e Memória de Dados.

## 2. Organização do Pipeline (5 Estágios)

O processador foi organizado em um pipeline clássico de 5 estágios para maximizar o *throughput* (vazão) de instruções:
1. **IF (Instruction Fetch):** Busca da instrução baseada no PC.
2. **ID (Instruction Decode):** Decodificação e leitura do Banco de Registradores.
3. **EX (Execute):** Execução de operações aritméticas/lógicas na ULA.
4. **MEM (Memory):** Acesso à memória de dados (instruções LD/ST).
5. **WB (Write Back):** Escrita do resultado final no Banco de Registradores.

## 3. Decisões de Projeto (ISA e Hardware)

### 3.1 Tamanho da Palavra (16 bits)

Foi adotada uma palavra de **16 bits**. Essa escolha equilibra a simplicidade educacional com a capacidade de endereçamento necessária para os algoritmos propostos, como a busca do maior valor em vetores.

### 3.2 Conjunto de Instruções (ISA RISC)

A ISA segue a filosofia RISC com instruções de tamanho fixo, facilitando a decodificação em um único ciclo. As instruções implementadas incluem:
- **Aritméticas/Lógicas:** ADD, SUB, AND, OR, XOR, NOT, CMP.
- **Memória:** LD (Load), ST (Store).
- **Controle de Fluxo:** J (Jump), JN (Jump if Negative), JZ (Jump if Zero).

### 3.3 Tratamento de Hazards

Para garantir a integridade dos dados no pipeline, foi implementada uma **Unidade de Detecção de Hazards**.
- **Load-Use Hazard:** Quando uma instrução depende do resultado de um `LD` que ainda está no estágio de memória, a unidade insere um *Stall* (bolha) no pipeline para evitar o uso de dados obsoletos.
- **Controle:** Saltos condicionais resultam em um *Flush* dos estágios iniciais para evitar a execução de instruções do caminho errado.

## 4. Análise de Desempenho

O desempenho é medido em ciclos de relógio. Embora o pipeline idealmente execute uma instrução por ciclo (CPI = 1), a presença de *stalls* e *flushes* aumenta o tempo total de execução. O uso de registradores de interface (IF/ID, ID/EX, etc.) é crucial para manter os sinais estáveis entre as transições de estado do sistema.