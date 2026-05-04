#include <systemc.h>
#include "processador.h"

int sc_main(int argc, char* argv[]) {
    sc_signal<bool> clock;
    sc_signal<bool> reset;
    
    PROCESSADOR cpu("CPU");
    cpu.clock(clock);
    cpu.reset(reset);
    
    // ============================================
    // PROGRAMA: FIBONACCI
    // ============================================
    sc_uint<16> programa[] = {
        0x710A,  // 0:  LD R1, [R0+10]   ; R1 = N
        0x420B,  // 1:  LD R2, [R0+11]   ; R2 = 1 (constante 1)
        0x4400,  // 2:  ADD R4, R0, R0   ; R4 = 0 (F(i-2))
        0x4502,  // 3:  ADD R5, R0, R2   ; R5 = 1 (F(i-1))
        0x8400,  // 4:  ST  R4, [R0+0]   ; mem[0] = F(0) = 0
        0x8501,  // 5:  ST  R5, [R0+1]   ; mem[1] = F(1) = 1
        0x4332,  // 6:  ADD R3, R3, 2    ; R3 = 2 (ponteiro)
        0x5731,  // 7:  SUB R7, R3, R1   ; R7 = R3 - N
        0xB007,  // 8:  JZ  +7           ; se R3 == N, sai
        0x4645,  // 9:  ADD R6, R4, R5   ; R6 = F(i-2) + F(i-1)
        0x8630,  // 10: ST  R6, [R3+0]   ; mem[R3] = R6
        0x4405,  // 11: ADD R4, R0, R5   ; R4 = R5
        0x4506,  // 12: ADD R5, R0, R6   ; R5 = R6
        0x4331,  // 13: ADD R3, R3, 1    ; R3++
        0x9009,  // 14: J   -7           ; volta ao teste
        0x9000   // 15: J   0            ; parada
    };
    int tam_programa = sizeof(programa) / sizeof(programa[0]);
    cpu.mem_instrucao->carregar_programa(programa, tam_programa);
    
    // ============================================
    // DADOS INICIAIS NA MEMÓRIA
    // ============================================
    sc_int<16> N = 8;
    sc_int<16> um = 1;
    cpu.mem_dados->carregar_dados(&N, 1, 10);   // mem[10] = 8 (N)
    cpu.mem_dados->carregar_dados(&um, 1, 11);  // mem[11] = 1 (constante)
    
    // ============================================
    // EXECUÇÃO
    // ============================================
    cout << "\n=== INICIANDO SIMULAÇÃO: FIBONACCI ===" << endl;
    cout << "Calculando os primeiros 8 termos.\n" << endl;
    
    reset.write(true);
    clock.write(false);
    sc_start(5, SC_NS);
    reset.write(false);
    
    // Aproximadamente 12 ciclos por iteração × 6 iterações + setup ≈ 100 ciclos
    for (int i = 0; i < 200; i++) {
        clock.write(true);
        sc_start(5, SC_NS);
        clock.write(false);
        sc_start(5, SC_NS);
    }
    
    // ============================================
    // RESULTADO
    // ============================================
    cout << "\n=== SIMULAÇÃO ENCERRADA ===" << endl;
    cout << "Sequência de Fibonacci esperada: 0, 1, 1, 2, 3, 5, 8, 13" << endl;
    cpu.mem_dados->dump_memoria(0, 11);
    
    return 0;
}