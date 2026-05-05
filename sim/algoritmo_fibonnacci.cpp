#include <systemc.h>
#include "processador.h"

int sc_main(int argc, char* argv[]) {
    sc_signal<bool> clock;
    sc_signal<bool> reset;
    
    PROCESSADOR cpu("CPU");
    cpu.clock(clock);
    cpu.reset(reset);
    
    sc_uint<16> programa[] = {
        0x7101,  // 0:  LD  R1, [R0+1]   -> R1 = 1 (F1)
        0x4000,  // 1:  NOP              -> aguarda LD inicial
        0x4000,  // 2:  NOP
        0x4000,  // 3:  NOP
        0x8000,  // 4:  ST  R0, [R0+0]   -> MEM[0] = F0
        0x8001,  // 5:  ST  R1, [R0+1]   -> MEM[1] = F1

        0x4201,  // 6:  ADD R2, R0, R1   -> F2 = F0 + F1
        0x4000,  // 7:  NOP
        0x4000,  // 8:  NOP
        0x4000,  // 9:  NOP
        0x8002,  // 10: ST  R2, [R0+2]

        0x4312,  // 11: ADD R3, R1, R2   -> F3
        0x4000,  // 12: NOP
        0x4000,  // 13: NOP
        0x4000,  // 14: NOP
        0x8003,  // 15: ST  R3, [R0+3]

        0x4423,  // 16: ADD R4, R2, R3   -> F4
        0x4000,  // 17: NOP
        0x4000,  // 18: NOP
        0x4000,  // 19: NOP
        0x8004,  // 20: ST  R4, [R0+4]

        0x4534,  // 21: ADD R5, R3, R4   -> F5
        0x4000,  // 22: NOP
        0x4000,  // 23: NOP
        0x4000,  // 24: NOP
        0x8005,  // 25: ST  R5, [R0+5]

        0x4645,  // 26: ADD R6, R4, R5   -> F6
        0x4000,  // 27: NOP
        0x4000,  // 28: NOP
        0x4000,  // 29: NOP
        0x8006,  // 30: ST  R6, [R0+6]

        0x4756,  // 31: ADD R7, R5, R6   -> F7
        0x4000,  // 32: NOP
        0x4000,  // 33: NOP
        0x4000,  // 34: NOP
        0x8007,  // 35: ST  R7, [R0+7]
        0x9000   // 36: J   0            -> parada simples
    };
    int tam_programa = sizeof(programa) / sizeof(programa[0]);
    cpu.mem_instrucao->carregar_programa(programa, tam_programa);
    
    sc_int<16> um = 1;
    cpu.mem_dados->carregar_dados(&um, 1, 1);   // MEM[1] tambem e F(1)
    
    // ============================================
    // EXECUÇÃO
    // ============================================
    cout << "\n=== INICIANDO SIMULAÇÃO: FIBONACCI ===" << endl;
    cout << "Calculando os primeiros 8 termos.\n" << endl;
    
    reset.write(true);
    clock.write(false);
    sc_start(5, SC_NS);
    reset.write(false);
    
    for (int i = 0; i < 300; i++) {
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
    cpu.mem_dados->dump_memoria(0, 7);
    
    return 0;
}
