<div align="center">
  <img align="right" alt="logo imd" height="60" src="docs/assets/dimap.png">
  <img align="left" alt="logo ufrn" height="60" src="docs/assets/ufrn-logo.png">
  <br/><br/><br/>

# Processador RISC Pipelined em SystemC

### DIM0129 — Organização de Computadores

**UFRN — Departamento de Informática e Matemática Aplicada**

</div>

---

> 📄 Para mais detalhes, acesse o [Relatório](docs/relatorio.pdf).

Este repositório contém a implementação de um processador baseado na arquitetura **RISC** com organização em **Pipeline de 5 estágios**, desenvolvido em **SystemC**. O sistema é composto por uma **Parte Operativa (PO)** e uma **Parte de Controle (PC)**, integradas para executar um conjunto de 12 instruções.

**Autores:**
- Kauã do Vale Ferreira
- Luisa Ferreira de Souza Santos
- Ryan David dos Santos Silvestre

**Professor:** Marcio Eduardo Kreutz

**Disciplina:** DIM0129 — Organização de Computadores · UFRN · 2026.1

---

## Índice

- [Resumo e Objetivos](#-resumo-e-objetivos)
- [Decisões de Projeto](#️-decisões-de-projeto)
- [Arquitetura do Processador](#️-arquitetura-do-processador)
- [Conjunto de Instruções (ISA)](#-conjunto-de-instruções-isa)
- [Estrutura do Repositório](#-estrutura-do-repositório)
- [Módulos Implementados](#-módulos-implementados)
- [Algoritmos Simulados](#-algoritmos-simulados)
- [Como Compilar e Executar](#️-como-compilar-e-executar)
- [Resultados](#-resultados)

---

## Resumo e Objetivos

O objetivo principal é desenvolver um modelo funcional de processador capaz de executar algoritmos clássicos por meio de um pipeline de processamento. A arquitetura foca em:

1. **Modularidade** — Cada componente da Parte Operativa (ULA, Banco de Registradores, Memória) é implementado e testado individualmente antes da integração.
2. **Eficiência** — Execução de instruções em estágios paralelos para maximizar o throughput e reduzir o tempo de execução.
3. **Tratamento de Conflitos (Hazards)** — Implementação de soluções para dependências de dados (*data hazards*) e de controle (*control hazards*), garantindo a integridade da execução.

---

## Decisões de Projeto

| # | Decisão | Escolha Adotada |
|:---:|:---|:---|
| 2.1 | Tamanho da palavra | **16 bits** |
| 2.2 | Formato da instrução | **Tipo R** (reg-reg) e **Tipo I** (imediato/memória) |
| 2.3 | Modos de endereçamento | Registrador, Imediato, Direto (memória) |
| 2.4 | Banco de registradores | **8 registradores** de uso geral (R0–R7) |
| 2.5 | Tamanho das memórias | Instrução: **256 palavras** · Dados: **256 palavras** |
| 2.6 | Barramentos da PO | Barramento único com canais dedicados leitura/escrita |
| 2.7 | Organização do pipeline | **5 estágios**: IF → ID → EX → MEM → WB |

---

## Arquitetura do Processador

O processador segue o modelo clássico de pipelines RISC com **5 estágios**:

```
┌──────┐   ┌──────┐   ┌──────┐   ┌──────┐   ┌──────┐
│  IF  │──▶│  ID  │──▶│  EX  │──▶│ MEM  │──▶│  WB  │
│Busca │   │Decod.│   │Execut│   │Memór.│   │Escrit│
└──────┘   └──────┘   └──────┘   └──────┘   └──────┘
    IF/ID        ID/EX        EX/MEM       MEM/WB
  (reg. pipe) (reg. pipe)  (reg. pipe)  (reg. pipe)
```

| Estágio | Nome | Descrição |
|:---:|:---|:---|
| **IF** | Instruction Fetch | Busca a instrução na memória de instruções via PC |
| **ID** | Instruction Decode | Decodifica a instrução e lê os registradores-fonte |
| **EX** | Execute | Executa a operação na ULA ou calcula o endereço |
| **MEM** | Memory Access | Leitura (LD) ou escrita (ST) na memória de dados |
| **WB** | Write Back | Escreve o resultado no banco de registradores |

### Tratamento de Hazards

- **Data Hazards:** Forwarding/bypassing entre estágios EX→EX e MEM→EX; stall para casos load-use
- **Control Hazards:** Flush do pipeline em saltos (J, JN, JZ); branch resolution no estágio ID

---

## Conjunto de Instruções (ISA)

### Instruções Lógicas

| Mnemônico | Operação | Descrição |
|:---|:---|:---|
| `AND Rd, Rs1, Rs2` | Rd ← Rs1 AND Rs2 | E lógico bit a bit |
| `OR  Rd, Rs1, Rs2` | Rd ← Rs1 OR Rs2  | OU lógico bit a bit |
| `XOR Rd, Rs1, Rs2` | Rd ← Rs1 XOR Rs2 | OU exclusivo bit a bit |
| `NOT Rd, Rs1`      | Rd ← NOT Rs1     | Inversão bit a bit |

### Instruções Aritméticas

| Mnemônico | Operação | Descrição |
|:---|:---|:---|
| `ADD Rd, Rs1, Rs2` | Rd ← Rs1 + Rs2 | Soma inteira |
| `SUB Rd, Rs1, Rs2` | Rd ← Rs1 - Rs2 | Subtração inteira |
| `CMP Rs1, Rs2`     | flags ← Rs1 - Rs2 | Comparação (atualiza flags N e Z) |

### Instruções de Acesso à Memória

| Mnemônico | Operação | Descrição |
|:---|:---|:---|
| `LD  Rd, imm(Rs)` | Rd ← MEM[Rs + imm] | Leitura da memória de dados |
| `ST  Rs, imm(Rb)` | MEM[Rb + imm] ← Rs | Escrita na memória de dados |

### Instruções de Controle de Fluxo

| Mnemônico | Condição | Descrição |
|:---|:---|:---|
| `J  imm` | Sempre | Salto incondicional para PC + imm |
| `JN imm` | flag N = 1 | Salta se resultado negativo |
| `JZ imm` | flag Z = 1 | Salta se resultado zero |

---

## Estrutura do Repositório

```
pipeline-risc-systemc/
│
├── docs/
│   ├── assets/                  # Logos e imagens
│   ├── notes/                   # Relatório completo e anotações do trabalho
│   └── diagramas/               # Diagramas de bloco PO+PC
│
├── src/
│   ├── po/                      
│   │   ├── ula.h / ula.cpp        
│   │   ├── banco_registradores.h / .cpp  
│   │   ├── memoria.h / .cpp        
│   │   └── pipeline_regs.h          
│   │
│   ├── pc/                      # Parte de Controle
│   │   └── unidade_controle.h / .cpp    # Unidade de Controle
│   │
│   └── top/
│       └── processador.h / .cpp   # Integração PO + PC
│
├── sim/                         # Programas de teste (algoritmos)
│   ├── algo1_bubble_sort.cpp
│   ├── algo2_fibonacci.cpp
│   └── algo3_soma_vetor.cpp
│
├── tb/                          # Testbenches individuais
│   ├── tb_ula.cpp
│   ├── tb_banco_registradores.cpp
│   ├── tb_memoria.cpp
│   ├── tb_unidade_controle.cpp
│   └── tb_processador.cpp
│
├── Makefile
└── README.md
```

---

## Módulos Implementados

| Módulo | Arquivo | Função | Status |
|:---|:---|:---|:---:|
| **ULA** | `src/po/alu` | Operações AND, OR, XOR, NOT, ADD, SUB, CMP 
| **Banco de Registradores** | `src/po/register_bank` | 8 registradores R0–R7, leitura dual-port 
| **Memória de Instrução** | `src/po/memory` | ROM com 256 posições de 16 bits | 🔧 Em desenvolvimento
| **Memória de Dados** | `src/po/memory` | RAM com 256 posições de 16 bits | 🔧 Em desenvolvimento 
| **Registradores de Pipeline** | `src/po/pipeline_regs` | IF/ID · ID/EX · EX/MEM · MEM/WB 
| **Unidade de Controle** | `src/pc/control_unit` | Decodificação de opcode e sinais de controle
| **Processador (Top)** | `src/top/processor` | Integração completa PO + PC 

---

## Algoritmos Simulados

Os seguintes algoritmos serão usados para validar a arquitetura:

| # | Algoritmo | Instruções Exercitadas | Arquivo |
|:---:|:---|:---|:---|
| 1 | **Bubble Sort** | ADD, SUB, CMP, JN, JZ, LD, ST | `sim/algo1_bubble_sort.cpp` |
| 2 | **Sequência de Fibonacci** | ADD, CMP, JZ, LD, ST | `sim/algo2_fibonacci.cpp` |
| 3 | **Soma de Vetor** | ADD, LD, CMP, JZ, J | `sim/algo3_soma_vetor.cpp` |

---

## Como Compilar e Executar

### Pré-requisitos

- SystemC 2.3.x instalado
- GCC / G++ com suporte a C++17
- `make`

### Compilação

```bash
# Clone o repositório
git clone https://github.com/DevlTz/pipeline-risc.git
cd pipeline-risc

# Compile o projeto completo
make

# Ou compile apenas um módulo específico (ex: ULA)
make tb_alu
```

### Execução

```bash
# Roda a simulação completa com o algoritmo 1 (Bubble Sort)
./sim/algo1_bubble_sort

# Roda o testbench da ULA
./tb/tb_alu
```

### Limpeza

```bash
make clean
```

---

## Resultados

Os resultados das simulações (diagramas de forma de onda e análises de desempenho) estão disponíveis no [relatório completo](docs/relatorio.pdf).

### Análise de Desempenho (Pipeline)

| Situação | CPI Esperado |
|:---|:---:|
| Sem hazards (ideal) | 1,0 |
| Com data hazards (forwarding) | ~1,2 |
| Com load-use hazard (1 stall) | ~1,4 |
| Com branch (flush 2 inst.) | ~1,6 |

---

<div align="center">
  <sub>Desenvolvido para a disciplina DIM0129 — UFRN · 2026.1</sub>
</div>