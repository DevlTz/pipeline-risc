#include <systemc.h>
#include "processador.h"

int sc_main(int argc, char* argv[]) {
    sc_signal<bool> clock;
    sc_signal<bool> reset;
    
    PROCESSADOR cpu("CPU");
    cpu.clock(clock);
    cpu.reset(reset);
    
    sc_uint<16> programa[] = {
        0x7105,  // 0:  LD  R1, [R0+5]    -> R1 = base do vetor (endereco 10)
        0x7607,  // 1:  LD  R6, [R0+7]    -> R6 = 1 (incremento)
        0x4400,  // 2:  ADD R4, R0, R0    -> R4 = 0 (acumulador)
        0x4000,  // 3:  NOP               -> aguarda LDs iniciais
        0x4000,  // 4:  NOP
        0x4000,  // 5:  NOP

        0x7510,  // 6:  LD  R5, [R1+0]    -> vetor[0]
        0x4445,  // 7:  ADD R4, R4, R5
        0x4116,  // 8:  ADD R1, R1, R6
        0x4000,  // 9:  NOP               -> aguarda R1/R4
        0x4000,  // 10: NOP
        0x4000,  // 11: NOP

        0x7510,  // 12: LD  R5, [R1+0]    -> vetor[1]
        0x4445,  // 13: ADD R4, R4, R5
        0x4116,  // 14: ADD R1, R1, R6
        0x4000,  // 15: NOP
        0x4000,  // 16: NOP
        0x4000,  // 17: NOP

        0x7510,  // 18: LD  R5, [R1+0]    -> vetor[2]
        0x4445,  // 19: ADD R4, R4, R5
        0x4116,  // 20: ADD R1, R1, R6
        0x4000,  // 21: NOP
        0x4000,  // 22: NOP
        0x4000,  // 23: NOP

        0x7510,  // 24: LD  R5, [R1+0]    -> vetor[3]
        0x4445,  // 25: ADD R4, R4, R5
        0x4116,  // 26: ADD R1, R1, R6
        0x4000,  // 27: NOP
        0x4000,  // 28: NOP
        0x4000,  // 29: NOP

        0x7510,  // 30: LD  R5, [R1+0]    -> vetor[4]
        0x4445,  // 31: ADD R4, R4, R5
        0x4000,  // 32: NOP               -> aguarda acumulador antes do ST
        0x4000,  // 33: NOP
        0x4000,  // 34: NOP
        0x8404,  // 35: ST  R4, [R0+4]    -> salva resultado em MEM[4]
        0x9000   // 36: J   0             -> parada simples
    };
    int tam_programa = sizeof(programa) / sizeof(programa[0]);
    cpu.mem_instrucao->carregar_programa(programa, tam_programa);

    sc_int<16> base = 10;
    sc_int<16> um = 1;
    sc_int<16> vetor[] = {3, 7, 2, 8, 5};

    cpu.mem_dados->carregar_dados(&base, 1, 5);
    cpu.mem_dados->carregar_dados(&um, 1, 7);
    cpu.mem_dados->carregar_dados(vetor, 5, 10);
    
    cout << "\n=== INICIANDO SIMULAÇÃO: SOMA VETORIAL ===" << endl;
    cout << "Vetor: [3, 7, 2, 8, 5] | Soma esperada: 25\n" << endl;
    
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
    
    cout << "\n=== SIMULAÇÃO ENCERRADA ===" << endl;
    cpu.mem_dados->dump_memoria(0, 15);
    
    return 0;
}
