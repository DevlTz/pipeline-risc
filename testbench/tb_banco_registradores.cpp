/**
 * @file tb_banco_registradores.cpp
 * @brief Testbench unitário — BANCO DE REGISTRADORES
 *
 * Executa 7 testes independentes que cobrem todos os comportamentos
 * especificados para o BANCO_REG (registradores.h / registradores.cpp).
 *
 * Testes:
 *   T1 — Escrita síncrona e leitura combinacional básica
 *   T2 — Valores negativos (complemento de dois)
 *   T3 — Leitura dual-port simultânea em dois registradores distintos
 *   T4 — Proteção do R0 (hardwired zero; escrita deve ser ignorada)
 *   T5 — write=0 mantém estado anterior inalterado
 *   T6 — Cobertura completa de R1-R7 (write + readback)
 *   T7 — Limites aritméticos de 16 bits (32767 e -32768)
 *
 * Compilação (ajuste SYSTEMC_HOME conforme instalação local):
 *   g++ -std=c++17 testbench/tb_banco_registradores.cpp \
 *       src/po/registradores.cpp \
 *       -I$SYSTEMC_HOME/include -L$SYSTEMC_HOME/lib-linux64 \
 *       -lsystemc -Wl,-rpath,$SYSTEMC_HOME/lib-linux64 \
 *       -o bin/tb_banco_reg
 *
 * Execução:
 *   ./bin/tb_banco_reg
 */

#include <systemc.h>
#include "../src/po/registradores.h"
#include <iostream>
using namespace std;

// ─── helpers ─────────────────────────────────────────────────────────────────

/** Gera N ciclos completos de clock (período = 10 ns). */
static void tick(sc_signal<bool> &clk, int n = 1)
{
    for (int i = 0; i < n; ++i)
    {
        clk.write(false);
        sc_start(5, SC_NS);
        clk.write(true);
        sc_start(5, SC_NS);
    }
}

static int total_pass = 0, total_fail = 0;

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

// ─── sc_main ─────────────────────────────────────────────────────────────────

int sc_main(int, char **)
{

    // ── Declaração de sinais ──────────────────────────────────────────────
    sc_signal<bool> clk;
    sc_signal<sc_uint<4>> t_read1, t_read2, t_wreg;
    sc_signal<sc_int<16>> t_res, t_val1, t_val2;
    sc_signal<bool> t_write;

    // ── DUT ──────────────────────────────────────────────────────────────
    BANCO_REG dut("BANCO_REG");
    dut.clock(clk);
    dut.reg_read_1(t_read1);
    dut.reg_read_2(t_read2);
    dut.reg_write(t_wreg);
    dut.res(t_res);
    dut.write(t_write);
    dut.value_1(t_val1);
    dut.value_2(t_val2);

    // Estado inicial estável
    clk.write(false);
    t_write.write(false);
    t_read1.write(0);
    t_read2.write(0);
    t_wreg.write(0);
    t_res.write(0);
    sc_start(SC_ZERO_TIME);

    cout << "\n=== TB: BANCO DE REGISTRADORES ===\n";

    // ── T1: Escrita síncrona e leitura combinacional ──────────────────────
    cout << "\n[T1] Escrita básica — R1 = 42\n";
    {
        t_write.write(true);
        t_wreg.write(1);
        t_res.write(42);
        t_read1.write(1);
        tick(clk);          // borda de subida grava o valor em regs[1]
        sc_start(1, SC_NS); // propaga leitura combinacional (read_reg)
        check("R1 == 42", t_val1.read() == 42);
    }

    // ── T2: Valores negativos ─────────────────────────────────────────────
    cout << "\n[T2] Valor negativo — R2 = -10\n";
    {
        t_wreg.write(2);
        t_res.write(-10);
        t_read2.write(2);
        tick(clk);
        sc_start(1, SC_NS);
        check("R2 == -10", t_val2.read() == -10);
    }

    // ── T3: Leitura dual-port ─────────────────────────────────────────────
    // Verifica que as duas portas de leitura funcionam simultaneamente
    // sem interferir uma na outra.
    cout << "\n[T3] Dual-port — R1 e R2 simultâneos\n";
    {
        t_write.write(false);
        t_read1.write(1);
        t_read2.write(2);
        sc_start(1, SC_NS);
        check("val1 == 42", t_val1.read() == 42);
        check("val2 == -10", t_val2.read() == -10);
    }

    // ── T4: Proteção R0 (hardwired zero) ──────────────────────────────────
    // R0 deve permanecer 0 mesmo que a lógica de escrita seja ativada.
    cout << "\n[T4] Protecao R0 — tentativa de escrever 999 em R0\n";
    {
        t_write.write(true);
        t_wreg.write(0);
        t_res.write(999);
        t_read1.write(0);
        tick(clk);
        sc_start(1, SC_NS);
        check("R0 == 0 (hardwired zero)", t_val1.read() == 0);
    }

    // ── T5: write=0 não altera banco ──────────────────────────────────────
    // Com write desabilitado, mesmo que reg_write e res sejam setados,
    // o banco não deve mudar.
    cout << "\n[T5] Escrita inibida — R1 deve manter 42 (write=0)\n";
    {
        t_write.write(false);
        t_wreg.write(1);
        t_res.write(9999);
        t_read1.write(1);
        tick(clk);
        sc_start(1, SC_NS);
        check("R1 ainda == 42", t_val1.read() == 42);
    }

    // ── T6: Cobertura completa R1-R7 ──────────────────────────────────────
    // Escreve valor = índice * 10 em cada registrador e faz readback.
    cout << "\n[T6] Cobertura total — R1-R7 = indice x 10\n";
    {
        t_write.write(true);
        for (int i = 1; i <= 7; ++i)
        {
            t_wreg.write(i);
            t_res.write(i * 10);
            tick(clk);
        }
        t_write.write(false);

        bool all_ok = true;
        for (int i = 1; i <= 7; ++i)
        {
            t_read1.write(i);
            sc_start(1, SC_NS);
            if (t_val1.read() != i * 10)
            {
                cout << "    Erro R" << i << ": esperado=" << (i * 10)
                     << " obtido=" << t_val1.read() << "\n";
                all_ok = false;
            }
        }
        check("R1-R7 todos corretos", all_ok);
    }

    // ── T7: Limites de 16 bits ────────────────────────────────────────────
    cout << "\n[T7] Limites aritmeticos — R5=32767, R6=-32768\n";
    {
        t_write.write(true);
        t_wreg.write(5);
        t_res.write(32767);
        tick(clk);
        t_wreg.write(6);
        t_res.write(-32768);
        tick(clk);
        t_write.write(false);
        t_read1.write(5);
        t_read2.write(6);
        sc_start(1, SC_NS);
        check("R5 ==  32767", t_val1.read() == 32767);
        check("R6 == -32768", t_val2.read() == -32768);
    }

    // ── Resumo ────────────────────────────────────────────────────────────
    cout << "\n===========================================\n";
    cout << "RESULTADO: " << total_pass << " PASS | " << total_fail << " FAIL\n";
    cout << "===========================================\n\n";

    return (total_fail > 0) ? 1 : 0;
}