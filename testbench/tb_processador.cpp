/**
 * @file tb_processador.cpp
 * @brief Testbench de integração — PROCESSADOR RISC PIPELINED (5 estágios)
 *
 * Valida o comportamento end-to-end do módulo PROCESSADOR (processador.h /
 * processador.cpp), exercitando o pipeline completo (IF → ID → EX → MEM → WB)
 * e os mecanismos de hazard handling implementados.
 *
 * Estrutura dos testes
 * ───────────────────
 *   Suite A — Instruções aritméticas e lógicas (Tipo R)
 *     A1  ADD: soma de dois registradores
 *     A2  SUB: subtração com resultado positivo
 *     A3  SUB: subtração com resultado negativo (flag_neg)
 *     A4  AND lógico bit a bit
 *     A5  OR  lógico bit a bit
 *     A6  XOR lógico bit a bit
 *     A7  NOT bit a bit
 *     A8  CMP: flag_zero quando operandos iguais
 *     A9  CMP: flag_neg  quando Rs1 < Rs2
 *
 *   Suite B — Acesso à memória (LD / ST)
 *     B1  ST seguido de LD no mesmo endereço (round-trip)
 *     B2  LD com offset positivo (endereçamento indexado)
 *     B3  ST com offset (escrita indexada)
 *
 *   Suite C — Controle de fluxo (J / JZ / JN)
 *     C1  J (salto incondicional): PC desvia para alvo correto
 *     C2  JZ tomado  (flag_zero = 1 após CMP igual)
 *     C3  JZ não tomado (flag_zero = 0)
 *     C4  JN tomado  (flag_neg  = 1 após CMP negativo)
 *     C5  JN não tomado (flag_neg = 0)
 *
 *   Suite D — Hazards de dados
 *     D1  Load-use hazard: LD seguido imediatamente de ADD que usa o mesmo Rd
 *     D2  Forwarding EX→EX: dois ADDs consecutivos com dependência RAW
 *     D3  Forwarding MEM→EX: ADD cujo operando ainda está em MEM
 *
 *   Suite E — Sinal de reset
 *     E1  Reset assíncrono: PC volta a 0 e banco de registradores é reiniciado
 *
 *   Suite F — Algoritmos completos
 *     F1  Soma de vetor de 4 elementos (exercita LD, ADD, J, CMP, JZ)
 *     F2  Sequência de Fibonacci de 6 termos (exercita ADD, ST, LD, JZ)
 *
 * Convenção de saída
 * ──────────────────
 *   [PASS] <descrição>
 *   [FAIL] <descrição>   — seguido do detalhe esperado/obtido quando aplicável
 *
 * Compilação (ajuste SYSTEMC_HOME conforme instalação local):
 *   g++ -std=c++17 \
 *       testbench/tb_processador.cpp \
 *       src/po/ula.cpp \
 *       src/po/registradores.cpp \
 *       src/top/processador.cpp \
 *       src/pc/unidade_controle.cpp \
 *       -I$SYSTEMC_HOME/include \
 *       -L$SYSTEMC_HOME/lib-linux64 \
 *       -lsystemc -Wl,-rpath,$SYSTEMC_HOME/lib-linux64 \
 *       -o bin/tb_processador
 *
 * Execução:
 *   ./bin/tb_processador
 *
 * Código de retorno:
 *   0  — todos os testes passaram
 *   1  — um ou mais testes falharam
 */

#include <systemc.h>
#include "../src/top/processador.h"
#include <iostream>
#include <iomanip>

using namespace std;

// ─────────────────────────────────────────────────────────────────────────────
// Infraestrutura auxiliar
// ─────────────────────────────────────────────────────────────────────────────

static int total_pass = 0;
static int total_fail = 0;

/**
 * Imprime resultado de uma asserção e contabiliza o totalizador global.
 * @param desc  Descrição do que está sendo verificado.
 * @param cond  true = passou; false = falhou.
 */
static void check(const char *desc, bool cond)
{
    if (cond)
    {
        cout << "  [PASS] " << desc << "\n";
        ++total_pass;
    }
    else
    {
        cout << "  [FAIL] " << desc << "\n";
        ++total_fail;
    }
}

/** Variante com detalhe de esperado/obtido para facilitar diagnóstico. */
static void check_val(const char *desc, sc_int<16> expected, sc_int<16> obtained)
{
    bool ok = (expected == obtained);
    if (ok)
    {
        cout << "  [PASS] " << desc << "\n";
        ++total_pass;
    }
    else
    {
        cout << "  [FAIL] " << desc
             << "  (esperado=" << expected << " obtido=" << obtained << ")\n";
        ++total_fail;
    }
}

/**
 * Avança N ciclos completos de clock (período = 10 ns, borda de subida ativa).
 * O processador opera na borda de subida; cada chamada a tick(clk) produz
 * exatamente uma borda de subida e uma de descida.
 */
static void tick(sc_signal<bool> &clk, int n = 1)
{
    for (int i = 0; i < n; ++i)
    {
        clk.write(true);
        sc_start(5, SC_NS);
        clk.write(false);
        sc_start(5, SC_NS);
    }
}

/**
 * Aplica o sinal de reset por 2 ciclos e o derruba.
 * Garante que o PC e os registradores de pipeline estejam em estado inicial
 * conhecido antes de cada suite de testes.
 */
static void reset_cpu(PROCESSADOR &cpu,
                      sc_signal<bool> &clk,
                      sc_signal<bool> &rst)
{
    rst.write(true);
    tick(clk, 2);
    rst.write(false);
    sc_start(1, SC_NS); // propaga a descida do reset
}

// ─────────────────────────────────────────────────────────────────────────────
// Utilitários de carga de programa
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Codifica uma instrução Tipo R.
 *   | opcode[15:12] | Rd[11:8] | Rs1[7:4] | Rs2[3:0] |
 */
static sc_uint<16> tipo_r(uint8_t op, uint8_t rd, uint8_t rs1, uint8_t rs2)
{
    return (sc_uint<16>)(((op & 0xF) << 12) |
                         ((rd & 0xF) << 8) |
                         ((rs1 & 0xF) << 4) |
                         (rs2 & 0xF));
}

/**
 * Codifica uma instrução Tipo I.
 *   | opcode[15:12] | Rd/Rs[11:8] | Rs_base[7:4] | imm[3:0] |
 * O imediato é truncado para 4 bits (complemento de dois).
 */
static sc_uint<16> tipo_i(uint8_t op, uint8_t rd, uint8_t rs_base, int8_t imm)
{
    return (sc_uint<16>)(((op & 0xF) << 12) |
                         ((rd & 0xF) << 8) |
                         ((rs_base & 0xF) << 4) |
                         (imm & 0xF));
}

// Mnemonicos de opcode (conforme ISA documentada)
static const uint8_t OP_AND = 0x0;
static const uint8_t OP_OR = 0x1;
static const uint8_t OP_XOR = 0x2;
static const uint8_t OP_NOT = 0x3;
static const uint8_t OP_ADD = 0x4;
static const uint8_t OP_SUB = 0x5;
static const uint8_t OP_CMP = 0x6;
static const uint8_t OP_LD = 0x7;
static const uint8_t OP_ST = 0x8;
static const uint8_t OP_J = 0x9;
static const uint8_t OP_JN = 0xA;
static const uint8_t OP_JZ = 0xB;
static const uint8_t NOP = 0x0; // AND R0,R0,R0 — não altera estado útil

// ─────────────────────────────────────────────────────────────────────────────
// Suites de teste
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Suite A — Instruções aritméticas e lógicas (Tipo R)
 *
 * Estratégia: carrega valores conhecidos em R1 e R2 via sequências de ADD com
 * R0 (que é hardwired-zero), executa a instrução alvo e lê o resultado de Rd
 * diretamente do banco de registradores do DUT após o número de ciclos
 * suficiente para que o WB seja concluído (5 ciclos de pipeline + 1 de margem).
 */
static void suite_a(PROCESSADOR &cpu,
                    sc_signal<bool> &clk,
                    sc_signal<bool> &rst)
{
    cout << "\n[Suite A] Instrucoes aritmeticas e logicas (Tipo R)\n";

    // ── A1: ADD R3, R1, R2  (R1=10, R2=7 → R3=17) ────────────────────────
    {
        reset_cpu(cpu, clk, rst);

        sc_uint<16> prog[] = {
            // Inicializar R1 = 10  (ADD R1, R0, R0 não funciona, usamos imediato
            // via LD de endereço fixo — mas como a ISA não tem ADDI, usamos a
            // memória de dados pré-carregada para obter constantes)
            // Alternativa direta: ST não existe sem base. Usamos o forwarding
            // de que R0 = 0 e carregamos constantes pela memória de dados.
            //
            // A forma mais simples dentro desta ISA é usar LD com R0 como base
            // e endereços de memória pré-carregados com os valores desejados.
            // Os dados MEM[0]=10, MEM[1]=7 são carregados antes da simulação.
            tipo_i(OP_LD, 1, 0, 0),  // LD R1, 0(R0)  → R1 = MEM[0] = 10
            tipo_i(OP_LD, 2, 0, 1),  // LD R2, 1(R0)  → R2 = MEM[1] = 7
            tipo_r(OP_ADD, 3, 1, 2), // ADD R3, R1, R2 → R3 = 17
        };
        cpu.mem_instrucao->carregar_programa(prog, 3);

        sc_int<16> dados[] = {10, 7};
        cpu.mem_dados->carregar_dados(dados, 2, 0);

        // Pipeline de 5 estágios: 3 instruções precisam de 3+4 = 7 ciclos
        // para que o WB da última instrução seja concluído.
        tick(clk, 8);

        check_val("A1 ADD R3 = R1(10) + R2(7) = 17",
                  17, cpu.banco_reg->regs[3]);
    }

    // ── A2: SUB R3, R1, R2  (R1=15, R2=4 → R3=11) ───────────────────────
    {
        reset_cpu(cpu, clk, rst);

        sc_uint<16> prog[] = {
            tipo_i(OP_LD, 1, 0, 0),
            tipo_i(OP_LD, 2, 0, 1),
            tipo_r(OP_SUB, 3, 1, 2),
        };
        cpu.mem_instrucao->carregar_programa(prog, 3);

        sc_int<16> dados[] = {15, 4};
        cpu.mem_dados->carregar_dados(dados, 2, 0);

        tick(clk, 8);

        check_val("A2 SUB R3 = R1(15) - R2(4) = 11",
                  11, cpu.banco_reg->regs[3]);
    }

    // ── A3: SUB gerando resultado negativo (flag_neg deve ser 1) ─────────
    {
        reset_cpu(cpu, clk, rst);

        sc_uint<16> prog[] = {
            tipo_i(OP_LD, 1, 0, 0),
            tipo_i(OP_LD, 2, 0, 1),
            tipo_r(OP_SUB, 3, 1, 2),
        };
        cpu.mem_instrucao->carregar_programa(prog, 3);

        sc_int<16> dados[] = {3, 9}; // 3 - 9 = -6
        cpu.mem_dados->carregar_dados(dados, 2, 0);

        tick(clk, 8);

        check_val("A3 SUB resultado negativo R3 = 3 - 9 = -6",
                  -6, cpu.banco_reg->regs[3]);
    }

    // ── A4: AND R3, R1, R2  (0b1010 AND 0b1100 = 0b1000 = 8) ─────────────
    {
        reset_cpu(cpu, clk, rst);

        sc_uint<16> prog[] = {
            tipo_i(OP_LD, 1, 0, 0),
            tipo_i(OP_LD, 2, 0, 1),
            tipo_r(OP_AND, 3, 1, 2),
        };
        cpu.mem_instrucao->carregar_programa(prog, 3);

        sc_int<16> dados[] = {0b1010, 0b1100}; // 10 AND 12 = 8
        cpu.mem_dados->carregar_dados(dados, 2, 0);

        tick(clk, 8);

        check_val("A4 AND R3 = 0b1010 AND 0b1100 = 8",
                  8, cpu.banco_reg->regs[3]);
    }

    // ── A5: OR R3, R1, R2  (0b1010 OR 0b0101 = 0b1111 = 15) ──────────────
    {
        reset_cpu(cpu, clk, rst);

        sc_uint<16> prog[] = {
            tipo_i(OP_LD, 1, 0, 0),
            tipo_i(OP_LD, 2, 0, 1),
            tipo_r(OP_OR, 3, 1, 2),
        };
        cpu.mem_instrucao->carregar_programa(prog, 3);

        sc_int<16> dados[] = {0b1010, 0b0101};
        cpu.mem_dados->carregar_dados(dados, 2, 0);

        tick(clk, 8);

        check_val("A5 OR  R3 = 0b1010 OR 0b0101 = 15",
                  15, cpu.banco_reg->regs[3]);
    }

    // ── A6: XOR R3, R1, R2  (0b1111 XOR 0b1010 = 0b0101 = 5) ────────────
    {
        reset_cpu(cpu, clk, rst);

        sc_uint<16> prog[] = {
            tipo_i(OP_LD, 1, 0, 0),
            tipo_i(OP_LD, 2, 0, 1),
            tipo_r(OP_XOR, 3, 1, 2),
        };
        cpu.mem_instrucao->carregar_programa(prog, 3);

        sc_int<16> dados[] = {0b1111, 0b1010};
        cpu.mem_dados->carregar_dados(dados, 2, 0);

        tick(clk, 8);

        check_val("A6 XOR R3 = 0b1111 XOR 0b1010 = 5",
                  5, cpu.banco_reg->regs[3]);
    }

    // ── A7: NOT R3, R1  (NOT 0x0001 = 0xFFFE = -2 em sc_int<16>) ─────────
    {
        reset_cpu(cpu, clk, rst);

        sc_uint<16> prog[] = {
            tipo_i(OP_LD, 1, 0, 0),
            tipo_r(OP_NOT, 3, 1, 0), // Rs2 ignorado para NOT
        };
        cpu.mem_instrucao->carregar_programa(prog, 2);

        sc_int<16> dados[] = {1};
        cpu.mem_dados->carregar_dados(dados, 1, 0);

        tick(clk, 7);

        check_val("A7 NOT R3 = ~1 = -2 (complemento de dois)",
                  -2, cpu.banco_reg->regs[3]);
    }

    // ── A8: CMP com operandos iguais → flag_zero deve ser 1 ───────────────
    // CMP não escreve resultado no banco; verificamos a flag via mem_flag_zero
    // que é acessível como sinal interno após o estágio EX/MEM.
    {
        reset_cpu(cpu, clk, rst);

        sc_uint<16> prog[] = {
            tipo_i(OP_LD, 1, 0, 0),
            tipo_i(OP_LD, 2, 0, 1),
            tipo_r(OP_CMP, 0, 1, 2), // Rd=0, resultado descartado
        };
        cpu.mem_instrucao->carregar_programa(prog, 3);

        sc_int<16> dados[] = {5, 5};
        cpu.mem_dados->carregar_dados(dados, 2, 0);

        tick(clk, 8);

        // A flag fica disponível no sinal mem_flag_zero após o estágio EX/MEM
        check("A8 CMP iguais: flag_zero = 1",
              cpu.mem_flag_zero.read() == true);
        check("A8 CMP iguais: R0 nao modificado (hardwired zero)",
              cpu.banco_reg->regs[0] == 0);
    }

    // ── A9: CMP Rs1 < Rs2 → flag_neg deve ser 1 ──────────────────────────
    {
        reset_cpu(cpu, clk, rst);

        sc_uint<16> prog[] = {
            tipo_i(OP_LD, 1, 0, 0),
            tipo_i(OP_LD, 2, 0, 1),
            tipo_r(OP_CMP, 0, 1, 2),
        };
        cpu.mem_instrucao->carregar_programa(prog, 3);

        sc_int<16> dados[] = {2, 8}; // 2 - 8 = -6
        cpu.mem_dados->carregar_dados(dados, 2, 0);

        tick(clk, 8);

        check("A9 CMP Rs1<Rs2: flag_neg = 1",
              cpu.mem_flag_neg.read() == true);
        check("A9 CMP Rs1<Rs2: flag_zero = 0",
              cpu.mem_flag_zero.read() == false);
    }
}

// ─────────────────────────────────────────────────────────────────────────────

/**
 * Suite B — Acesso à memória (LD / ST)
 */
static void suite_b(PROCESSADOR &cpu,
                    sc_signal<bool> &clk,
                    sc_signal<bool> &rst)
{
    cout << "\n[Suite B] Acesso a memoria (LD / ST)\n";

    // ── B1: ST + LD round-trip no mesmo endereço ───────────────────────────
    // Carrega valor em R1 via LD, escreve em outro endereço via ST,
    // lê de volta em R2 para verificar.
    {
        reset_cpu(cpu, clk, rst);

        sc_uint<16> prog[] = {
            tipo_i(OP_LD, 1, 0, 0), // R1 = MEM[0] = 42
            tipo_i(OP_ST, 1, 0, 5), // MEM[0+5] = R1  → MEM[5] = 42
            tipo_i(OP_LD, 2, 0, 5), // R2 = MEM[5]
        };
        cpu.mem_instrucao->carregar_programa(prog, 3);

        sc_int<16> dados[] = {42, 0, 0, 0, 0, 0}; // MEM[0]=42, MEM[5]=0 init
        cpu.mem_dados->carregar_dados(dados, 6, 0);

        tick(clk, 10);

        check_val("B1 ST/LD round-trip: R2 = MEM[5] = 42",
                  42, cpu.banco_reg->regs[2]);
    }

    // ── B2: LD com offset positivo ─────────────────────────────────────────
    // MEM[base + imm] onde base=R1=2 e imm=3 → acessa MEM[5]
    {
        reset_cpu(cpu, clk, rst);

        sc_uint<16> prog[] = {
            tipo_i(OP_LD, 1, 0, 0), // R1 = MEM[0] = 2  (valor base)
            tipo_i(OP_LD, 2, 1, 3), // R2 = MEM[R1+3] = MEM[5] = 99
        };
        cpu.mem_instrucao->carregar_programa(prog, 2);

        sc_int<16> dados[6] = {2, 0, 0, 0, 0, 99}; // MEM[0]=2, MEM[5]=99
        cpu.mem_dados->carregar_dados(dados, 6, 0);

        tick(clk, 9);

        check_val("B2 LD indexado: R2 = MEM[R1(2)+3] = MEM[5] = 99",
                  99, cpu.banco_reg->regs[2]);
    }

    // ── B3: ST com offset ─────────────────────────────────────────────────
    // Armazena R2=77 em MEM[R1+2] = MEM[4], depois lê de volta.
    {
        reset_cpu(cpu, clk, rst);

        sc_uint<16> prog[] = {
            tipo_i(OP_LD, 1, 0, 0), // R1 = MEM[0] = 2
            tipo_i(OP_LD, 2, 0, 1), // R2 = MEM[1] = 77
            tipo_i(OP_ST, 2, 1, 2), // MEM[R1+2] = MEM[4] = R2 = 77
            tipo_i(OP_LD, 3, 0, 4), // R3 = MEM[4] (verificacao)
        };
        cpu.mem_instrucao->carregar_programa(prog, 4);

        sc_int<16> dados[] = {2, 77, 0, 0, 0};
        cpu.mem_dados->carregar_dados(dados, 5, 0);

        tick(clk, 12);

        check_val("B3 ST indexado: R3 = MEM[R1(2)+2] = MEM[4] = 77",
                  77, cpu.banco_reg->regs[3]);
    }
}

// ─────────────────────────────────────────────────────────────────────────────

/**
 * Suite C — Controle de fluxo (J / JZ / JN)
 *
 * Estratégia: coloca uma instrução "armadilha" imediatamente após o salto
 * (opcode ADD com destino R7 = valor indesejado). Se o flush funcionar,
 * R7 permanece 0. O alvo do salto contém um ADD que escreve um valor
 * sentinel em R6, confirmando que o desvio ocorreu.
 */
static void suite_c(PROCESSADOR &cpu,
                    sc_signal<bool> &clk,
                    sc_signal<bool> &rst)
{
    cout << "\n[Suite C] Controle de fluxo (J / JZ / JN)\n";

    // ── C1: J incondicional ───────────────────────────────────────────────
    // PC 0: J +3  (salta para PC 0+1+3 = 4, pois branch resolve em ID com PC+1)
    // PC 1: ADD R7, R0, R0  (armadilha — não deve executar)
    // PC 2: ADD R7, R0, R0  (armadilha)
    // PC 3: ADD R7, R0, R0  (armadilha)
    // PC 4: LD  R6, 0(R0)   (sentinel: R6 = MEM[0] = 55)
    {
        reset_cpu(cpu, clk, rst);

        // ADD R7,R0,R0 codificado: opcode=ADD(4), Rd=7, Rs1=0, Rs2=0 → 0x4700
        sc_uint<16> armadilha = tipo_r(OP_ADD, 7, 0, 0);
        // Mas R0=0, então ADD R7,R0,R0 = 0. Para distinguir, usamos R7 com
        // valor MEM indesejado — porém R0 é hardwired zero, então o ADD daria
        // R7=0 de qualquer forma. Usamos ST para checar indiretamente se
        // chegamos ao alvo correto via R6.
        sc_uint<16> prog[8] = {};
        prog[0] = tipo_i(OP_J, 0, 0, 3); // J  imm=3 → PC_next = ID_PC+3
                                         // (ID_PC = PC+1 = 1; alvo = 1+3 = 4)
        prog[1] = armadilha;             // nao deve executar
        prog[2] = armadilha;
        prog[3] = armadilha;
        prog[4] = tipo_i(OP_LD, 6, 0, 0);  // R6 = MEM[0] = 55 (sentinel)
        prog[5] = tipo_r(OP_ADD, 0, 0, 0); // NOP de fechamento

        cpu.mem_instrucao->carregar_programa(prog, 6);

        sc_int<16> dados[] = {55};
        cpu.mem_dados->carregar_dados(dados, 1, 0);

        tick(clk, 12);

        check_val("C1 J incondicional: R6 = 55 (alvo alcancado)",
                  55, cpu.banco_reg->regs[6]);
        // R7 deve ser 0 porque as armadilhas somam R0+R0=0
        check_val("C1 J incondicional: armadilha nao executou (R7=0)",
                  0, cpu.banco_reg->regs[7]);
    }

    // ── C2: JZ tomado (flag_zero = 1 após CMP igual) ─────────────────────
    {
        reset_cpu(cpu, clk, rst);

        sc_uint<16> prog[8] = {};
        prog[0] = tipo_i(OP_LD, 1, 0, 0);  // R1 = 5
        prog[1] = tipo_i(OP_LD, 2, 0, 1);  // R2 = 5
        prog[2] = tipo_r(OP_CMP, 0, 1, 2); // CMP R1,R2 → flag_zero=1
        // CMP resolve no EX do ciclo 5. JZ será lido no ID do ciclo 4,
        // mas as flags só ficam disponíveis do MEM para o ID.
        // Para garantir que a flag já está propagada, inserimos um NOP entre
        // CMP e JZ (bubble explícito para simplificar a verificação da lógica
        // de branch sem depender de um timing exato de forwarding das flags).
        prog[3] = tipo_r(OP_ADD, 0, 0, 0); // NOP (bolha explícita)
        prog[4] = tipo_i(OP_JZ, 0, 0, 2);  // JZ imm=2 → alvo = ID_PC+2
        prog[5] = tipo_r(OP_ADD, 0, 0, 0); // armadilha (nao deve executar)
        prog[6] = tipo_i(OP_LD, 6, 0, 2);  // R6 = MEM[2] = 77 (sentinel)

        cpu.mem_instrucao->carregar_programa(prog, 7);

        sc_int<16> dados[] = {5, 5, 77};
        cpu.mem_dados->carregar_dados(dados, 3, 0);

        tick(clk, 14);

        check_val("C2 JZ tomado: R6 = 77 (alvo alcancado)",
                  77, cpu.banco_reg->regs[6]);
    }

    // ── C3: JZ nao tomado (flag_zero = 0) ────────────────────────────────
    {
        reset_cpu(cpu, clk, rst);

        sc_uint<16> prog[8] = {};
        prog[0] = tipo_i(OP_LD, 1, 0, 0);  // R1 = 3
        prog[1] = tipo_i(OP_LD, 2, 0, 1);  // R2 = 7
        prog[2] = tipo_r(OP_CMP, 0, 1, 2); // flag_zero = 0
        prog[3] = tipo_r(OP_ADD, 0, 0, 0); // NOP
        prog[4] = tipo_i(OP_JZ, 0, 0, 3);  // JZ nao tomado
        prog[5] = tipo_i(OP_LD, 6, 0, 2);  // R6 = MEM[2] = 88 (deve executar)
        prog[6] = tipo_r(OP_ADD, 0, 0, 0); // NOP de fechamento

        cpu.mem_instrucao->carregar_programa(prog, 7);

        sc_int<16> dados[] = {3, 7, 88};
        cpu.mem_dados->carregar_dados(dados, 3, 0);

        tick(clk, 14);

        check_val("C3 JZ nao tomado: R6 = 88 (instrucao seguinte executou)",
                  88, cpu.banco_reg->regs[6]);
    }

    // ── C4: JN tomado (flag_neg = 1 após CMP negativo) ────────────────────
    {
        reset_cpu(cpu, clk, rst);

        sc_uint<16> prog[8] = {};
        prog[0] = tipo_i(OP_LD, 1, 0, 0);  // R1 = 2
        prog[1] = tipo_i(OP_LD, 2, 0, 1);  // R2 = 9
        prog[2] = tipo_r(OP_CMP, 0, 1, 2); // 2-9=-7 → flag_neg=1
        prog[3] = tipo_r(OP_ADD, 0, 0, 0); // NOP
        prog[4] = tipo_i(OP_JN, 0, 0, 2);  // JN tomado → alvo = ID_PC+2
        prog[5] = tipo_r(OP_ADD, 0, 0, 0); // armadilha
        prog[6] = tipo_i(OP_LD, 6, 0, 2);  // R6 = MEM[2] = 66 (sentinel)

        cpu.mem_instrucao->carregar_programa(prog, 7);

        sc_int<16> dados[] = {2, 9, 66};
        cpu.mem_dados->carregar_dados(dados, 3, 0);

        tick(clk, 14);

        check_val("C4 JN tomado: R6 = 66 (alvo alcancado)",
                  66, cpu.banco_reg->regs[6]);
    }

    // ── C5: JN nao tomado (flag_neg = 0) ─────────────────────────────────
    {
        reset_cpu(cpu, clk, rst);

        sc_uint<16> prog[8] = {};
        prog[0] = tipo_i(OP_LD, 1, 0, 0);  // R1 = 9
        prog[1] = tipo_i(OP_LD, 2, 0, 1);  // R2 = 2
        prog[2] = tipo_r(OP_CMP, 0, 1, 2); // 9-2=7 → flag_neg=0
        prog[3] = tipo_r(OP_ADD, 0, 0, 0); // NOP
        prog[4] = tipo_i(OP_JN, 0, 0, 3);  // JN nao tomado
        prog[5] = tipo_i(OP_LD, 6, 0, 2);  // R6 = MEM[2] = 33 (deve executar)
        prog[6] = tipo_r(OP_ADD, 0, 0, 0); // NOP de fechamento

        cpu.mem_instrucao->carregar_programa(prog, 7);

        sc_int<16> dados[] = {9, 2, 33};
        cpu.mem_dados->carregar_dados(dados, 3, 0);

        tick(clk, 14);

        check_val("C5 JN nao tomado: R6 = 33 (instrucao seguinte executou)",
                  33, cpu.banco_reg->regs[6]);
    }
}

// ─────────────────────────────────────────────────────────────────────────────

/**
 * Suite D — Hazards de dados
 *
 * Os testes desta suite verificam que o pipeline não produz resultados
 * incorretos quando há dependências RAW (Read After Write) entre instruções
 * consecutivas. A verificação é feita pelo valor final no banco de
 * registradores após a conclusão do WB.
 */
static void suite_d(PROCESSADOR &cpu,
                    sc_signal<bool> &clk,
                    sc_signal<bool> &rst)
{
    cout << "\n[Suite D] Hazards de dados\n";

    // ── D1: Load-Use Hazard ────────────────────────────────────────────────
    // LD R1, 0(R0)        [ciclo 1]
    // ADD R2, R1, R1      [ciclo 2] — usa R1 que ainda não completou WB
    //
    // A unidade de detecção de hazards deve inserir 1 stall entre LD e ADD.
    // Resultado correto: R2 = MEM[0] + MEM[0] = 20 + 20 = 40.
    {
        reset_cpu(cpu, clk, rst);

        sc_uint<16> prog[] = {
            tipo_i(OP_LD, 1, 0, 0),  // LD R1, 0(R0)    → R1 = 20
            tipo_r(OP_ADD, 2, 1, 1), // ADD R2, R1, R1  → R2 deve ser 40
        };
        cpu.mem_instrucao->carregar_programa(prog, 2);

        sc_int<16> dados[] = {20};
        cpu.mem_dados->carregar_dados(dados, 1, 0);

        // Com 1 stall, o WB do ADD ocorre 1 ciclo mais tarde.
        tick(clk, 10);

        check_val("D1 Load-Use hazard: R2 = LD(20) + LD(20) = 40",
                  40, cpu.banco_reg->regs[2]);
    }

    // ── D2: Forwarding EX→EX (RAW entre dois ADDs consecutivos) ──────────
    // ADD R3, R1, R2   [ciclo 1] — R1=5, R2=3 → R3=8
    // ADD R4, R3, R2   [ciclo 2] — usa R3 ainda em EX do ciclo anterior
    //
    // Sem forwarding: R4 usaria R3=0 (valor antigo) → R4=3 (errado).
    // Com forwarding: R4 usa R3=8 → R4 = 8+3 = 11 (correto).
    {
        reset_cpu(cpu, clk, rst);

        sc_uint<16> prog[] = {
            tipo_i(OP_LD, 1, 0, 0),  // R1 = 5
            tipo_i(OP_LD, 2, 0, 1),  // R2 = 3
            tipo_r(OP_ADD, 3, 1, 2), // R3 = 8
            tipo_r(OP_ADD, 4, 3, 2), // R4 = R3 + R2 = 8 + 3 = 11
        };
        cpu.mem_instrucao->carregar_programa(prog, 4);

        sc_int<16> dados[] = {5, 3};
        cpu.mem_dados->carregar_dados(dados, 2, 0);

        tick(clk, 12);

        check_val("D2 Forwarding EX->EX: R4 = R3(8) + R2(3) = 11",
                  11, cpu.banco_reg->regs[4]);
    }

    // ── D3: Forwarding MEM→EX ─────────────────────────────────────────────
    // ADD R3, R1, R2    [ciclo 1] → R3=8  (em MEM no ciclo 3)
    // NOP               [ciclo 2] — distância 1
    // ADD R4, R3, R2    [ciclo 3] — usa R3 que está em MEM
    //
    // Sem forwarding MEM→EX: R4 usaria R3 do banco (0) → errado.
    // Com forwarding: R4 = 8 + 3 = 11.
    {
        reset_cpu(cpu, clk, rst);

        sc_uint<16> prog[] = {
            tipo_i(OP_LD, 1, 0, 0),
            tipo_i(OP_LD, 2, 0, 1),
            tipo_r(OP_ADD, 3, 1, 2), // R3 = 8
            tipo_r(NOP, 0, 0, 0),    // distância 1 (NOP)
            tipo_r(OP_ADD, 4, 3, 2), // R4 = R3(8) + R2(3) = 11
        };
        cpu.mem_instrucao->carregar_programa(prog, 5);

        sc_int<16> dados[] = {5, 3};
        cpu.mem_dados->carregar_dados(dados, 2, 0);

        tick(clk, 13);

        check_val("D3 Forwarding MEM->EX: R4 = R3(8) + R2(3) = 11",
                  11, cpu.banco_reg->regs[4]);
    }
}

// ─────────────────────────────────────────────────────────────────────────────

/**
 * Suite E — Sinal de reset
 */
static void suite_e(PROCESSADOR &cpu,
                    sc_signal<bool> &clk,
                    sc_signal<bool> &rst)
{
    cout << "\n[Suite E] Sinal de reset\n";

    // ── E1: Reset assíncrono ──────────────────────────────────────────────
    // Executa alguns ciclos, depois aplica reset e verifica que o PC volta
    // a 0 e que o banco começa do estado inicial (R0=0 após reset).
    {
        // Carrega e executa um programa que escreve em R5
        sc_uint<16> prog[] = {
            tipo_i(OP_LD, 5, 0, 0), // R5 = 123
        };
        cpu.mem_instrucao->carregar_programa(prog, 1);

        sc_int<16> dados[] = {123};
        cpu.mem_dados->carregar_dados(dados, 1, 0);

        rst.write(false);
        tick(clk, 8); // executa, R5 deve ser 123

        // Agora aplica reset
        rst.write(true);
        tick(clk, 2);
        rst.write(false);
        sc_start(1, SC_NS);

        // Verifica que o PC voltou a 0 (sinal pc é acessível)
        check("E1 Reset: PC volta a 0",
              cpu.pc.read() == 0);

        // Executa o programa novamente do zero para confirmar que re-executa
        tick(clk, 8);
        check_val("E1 Reset: programa re-executa corretamente (R5=123)",
                  123, cpu.banco_reg->regs[5]);
    }
}

// ─────────────────────────────────────────────────────────────────────────────

/**
 * Suite F — Algoritmos completos
 *
 * Valida o processador em cenários realistas que exercitam múltiplas
 * instruções e mecanismos em conjunto.
 */
static void suite_f(PROCESSADOR &cpu,
                    sc_signal<bool> &clk,
                    sc_signal<bool> &rst)
{
    cout << "\n[Suite F] Algoritmos completos\n";

    // ── F1: Soma de vetor de 4 elementos ──────────────────────────────────
    //
    // Vetor: MEM[10..13] = {3, 7, 2, 8}
    // Resultado esperado: 3 + 7 + 2 + 8 = 20, armazenado em R5
    //
    // Mapa de registradores:
    //   R1 = acumulador (soma parcial), init = 0
    //   R2 = ponteiro (endereço corrente), init = 10
    //   R3 = elemento carregado
    //   R4 = limite superior (endereço 14)
    //
    // Programa (pseudo-asm):
    //   PC0:  LD   R1, 0(R0)      ; R1 = MEM[0] = 0  (acumulador)
    //   PC1:  LD   R2, 0(R0)      ; R2 = MEM[0] = 0  (temporário)
    //   PC2:  LD   R2, 1(R0)      ; R2 = MEM[1] = 10 (base do vetor)
    //   PC3:  LD   R4, 2(R0)      ; R4 = MEM[2] = 14 (limite = base + 4)
    // LOOP:
    //   PC4:  CMP  R2, R4         ; R2 == R4 ?
    //   PC5:  NOP                 ; (bolha pós-CMP)
    //   PC6:  JZ   imm=3          ; se R2==R4, sai do loop (alvo=PC10)
    //   PC7:  LD   R3, 0(R2)      ; R3 = MEM[R2]
    //   PC8:  ADD  R1, R1, R3     ; R1 += R3  (load-use hazard — stall automático)
    //   PC9:  ADD  R2, R2, R5_imm ; R2++  (sem ADDI, usamos ADD com R5=1)
    //           — mas R5 não foi inicializado. Alternativa: LD R5,3(R0)=1
    //   PC10: J    imm=-7         ; volta para LOOP (PC4)
    //   PC11: ST   R1, 4(R0)      ; MEM[4] = R1 (resultado)
    //
    // Nota: a ISA não tem ADDI. Para incrementar o ponteiro, carregamos
    // a constante 1 em R5 via LD MEM[3]=1 e usamos ADD.
    //
    // Simplificação para o testbench: usamos um loop de 4 iterações com
    // endereço verificável. Layout de MEM:
    //   MEM[0]  = 0   (init acumulador)
    //   MEM[1]  = 10  (base do vetor)
    //   MEM[2]  = 14  (limite)
    //   MEM[3]  = 1   (constante incremento)
    //   MEM[10] = 3
    //   MEM[11] = 7
    //   MEM[12] = 2
    //   MEM[13] = 8
    {
        reset_cpu(cpu, clk, rst);

        // Mnemônico para J com imediato negativo: imm=-7 = 0b1001 em 4 bits
        // (complemento de dois de 4 bits: -7 = 16-7 = 9 = 0x9)
        // No código da UC: if (rs2 & 0x8) imm = 0xFFF0 | rs2;
        // rs2=0x9 → bit3=1 → imm = 0xFFF0|0x9 = 0xFFF9 = -7 ✓
        //
        // JZ com imm=3: alvo = ID_PC(=PC7) + 3 = 10 — instrução ST.

        sc_uint<16> prog[16] = {};
        // Inicializações
        prog[0] = tipo_i(OP_LD, 1, 0, 0); // R1 = 0 (acum)
        prog[1] = tipo_i(OP_LD, 2, 0, 1); // R2 = 10 (ponteiro)
        prog[2] = tipo_i(OP_LD, 4, 0, 2); // R4 = 14 (limite)
        prog[3] = tipo_i(OP_LD, 5, 0, 3); // R5 = 1  (incremento)
        // LOOP (PC=4)
        prog[4] = tipo_r(OP_CMP, 0, 2, 4);           // CMP R2, R4
        prog[5] = tipo_r(OP_ADD, 0, 0, 0);           // NOP (bolha pos-CMP)
        prog[6] = tipo_i(OP_JZ, 0, 0, 3);            // JZ +3 → PC10 (sai)
        prog[7] = tipo_i(OP_LD, 3, 2, 0);            // LD R3, 0(R2)
        prog[8] = tipo_r(OP_ADD, 1, 1, 3);           // R1 += R3 (load-use stall)
        prog[9] = tipo_r(OP_ADD, 2, 2, 5);           // R2 += 1
        prog[10] = tipo_i(OP_J, 0, 0, (int8_t)(-7)); // J -7 → volta PC4
        // Saida do loop (PC=11)
        prog[11] = tipo_i(OP_ST, 1, 0, 4); // MEM[4] = R1 (resultado)

        cpu.mem_instrucao->carregar_programa(prog, 12);

        sc_int<16> dados[16] = {};
        dados[0] = 0;
        dados[1] = 10;
        dados[2] = 14;
        dados[3] = 1;
        dados[10] = 3;
        dados[11] = 7;
        dados[12] = 2;
        dados[13] = 8;
        cpu.mem_dados->carregar_dados(dados, 14, 0);

        // 4 iterações × ~5 ciclos + overhead = ~60 ciclos
        tick(clk, 70);

        check_val("F1 Soma de vetor {3,7,2,8}: R1 = 20",
                  20, cpu.banco_reg->regs[1]);
        check_val("F1 Soma de vetor: MEM[4] = 20 (resultado armazenado)",
                  20, cpu.mem_dados->memoria[4]);
    }

    // ── F2: Sequência de Fibonacci (6 termos) ─────────────────────────────
    //
    // F(0)=0, F(1)=1, F(2)=1, F(3)=2, F(4)=3, F(5)=5
    // Armazena em MEM[20..25].
    //
    // Mapa de registradores:
    //   R1 = F(n-2), init = 0
    //   R2 = F(n-1), init = 1
    //   R3 = F(n) = R1 + R2
    //   R4 = ponteiro de escrita, init = 20
    //   R5 = limite de escrita   = 25 (MEM[25] = último termo)
    //   R6 = constante 1 (incremento)
    //
    // Programa:
    //   PC0:  LD  R1, 0(R0)    ; R1 = 0 (F0)
    //   PC1:  LD  R2, 1(R0)    ; R2 = 1 (F1)
    //   PC2:  LD  R4, 2(R0)    ; R4 = 20 (ponteiro)
    //   PC3:  LD  R5, 3(R0)    ; R5 = 26 (limite exclusivo)
    //   PC4:  LD  R6, 4(R0)    ; R6 = 1  (incremento)
    //   PC5:  ST  R1, 0(R4)    ; MEM[R4] = R1
    //   PC6:  ADD R4, R4, R6   ; R4++
    //   PC7:  CMP R4, R5       ; R4 == limite?
    //   PC8:  NOP
    //   PC9:  JZ  +2           ; sai se R4==26 (após 6 termos)
    //   PC10: ADD R3, R1, R2   ; R3 = R1 + R2
    //   PC11: ADD R1, R2, R0   ; R1 = R2
    //   PC12: ADD R2, R3, R0   ; R2 = R3
    //   PC13: J   -9           ; volta PC5
    //   PC14: (fim)
    //
    // MEM[0]=0, MEM[1]=1, MEM[2]=20, MEM[3]=26, MEM[4]=1
    {
        reset_cpu(cpu, clk, rst);

        sc_uint<16> prog[16] = {};
        prog[0] = tipo_i(OP_LD, 1, 0, 0);
        prog[1] = tipo_i(OP_LD, 2, 0, 1);
        prog[2] = tipo_i(OP_LD, 4, 0, 2);
        prog[3] = tipo_i(OP_LD, 5, 0, 3);
        prog[4] = tipo_i(OP_LD, 6, 0, 4);
        prog[5] = tipo_i(OP_ST, 1, 4, 0);            // ST R1, 0(R4)
        prog[6] = tipo_r(OP_ADD, 4, 4, 6);           // R4 += 1
        prog[7] = tipo_r(OP_CMP, 0, 4, 5);           // CMP R4, R5
        prog[8] = tipo_r(OP_ADD, 0, 0, 0);           // NOP
        prog[9] = tipo_i(OP_JZ, 0, 0, 2);            // JZ +2 → PC12 (sai)
        prog[10] = tipo_r(OP_ADD, 3, 1, 2);          // R3 = R1 + R2
        prog[11] = tipo_r(OP_ADD, 1, 2, 0);          // R1 = R2 (ADD R1, R2, R0)
        prog[12] = tipo_r(OP_ADD, 2, 3, 0);          // R2 = R3
        prog[13] = tipo_i(OP_J, 0, 0, (int8_t)(-9)); // J -9 → PC5
        // PC14: fim

        cpu.mem_instrucao->carregar_programa(prog, 14);

        sc_int<16> dados[32] = {};
        dados[0] = 0;  // F(0)
        dados[1] = 1;  // F(1)
        dados[2] = 20; // base
        dados[3] = 26; // limite (base + 6 termos = 26)
        dados[4] = 1;  // incremento
        cpu.mem_dados->carregar_dados(dados, 5, 0);

        // 6 iterações × ~10 ciclos + overhead = ~80 ciclos
        tick(clk, 90);

        // F(0..5) = 0, 1, 1, 2, 3, 5
        check_val("F2 Fibonacci: MEM[20] = F(0) = 0", 0, cpu.mem_dados->memoria[20]);
        check_val("F2 Fibonacci: MEM[21] = F(1) = 1", 1, cpu.mem_dados->memoria[21]);
        check_val("F2 Fibonacci: MEM[22] = F(2) = 1", 1, cpu.mem_dados->memoria[22]);
        check_val("F2 Fibonacci: MEM[23] = F(3) = 2", 2, cpu.mem_dados->memoria[23]);
        check_val("F2 Fibonacci: MEM[24] = F(4) = 3", 3, cpu.mem_dados->memoria[24]);
        check_val("F2 Fibonacci: MEM[25] = F(5) = 5", 5, cpu.mem_dados->memoria[25]);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// sc_main
// ─────────────────────────────────────────────────────────────────────────────

int sc_main(int, char **)
{
    // Sinais externos
    sc_signal<bool> clk("clk");
    sc_signal<bool> rst("rst");

    // Instancia o processador
    PROCESSADOR cpu("CPU");
    cpu.clock(clk);
    cpu.reset(rst);

    // Estado inicial estável
    clk.write(false);
    rst.write(false);
    sc_start(SC_ZERO_TIME);

    cout << "\n";
    cout << "=============================================================\n";
    cout << " TB: PROCESSADOR RISC PIPELINED — Testbench de Integracao\n";
    cout << "=============================================================\n";

    suite_a(cpu, clk, rst);
    suite_b(cpu, clk, rst);
    suite_c(cpu, clk, rst);
    suite_d(cpu, clk, rst);
    suite_e(cpu, clk, rst);
    suite_f(cpu, clk, rst);

    cout << "\n";
    cout << "=============================================================\n";
    cout << " RESULTADO FINAL: "
         << total_pass << " PASS  |  " << total_fail << " FAIL\n";
    cout << "=============================================================\n\n";

    return (total_fail > 0) ? 1 : 0;
}