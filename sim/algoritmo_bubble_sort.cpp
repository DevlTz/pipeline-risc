#include <systemc.h>
#include "processador.h"

int sc_main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    sc_signal<bool> clock;
    sc_signal<bool> reset;

    PROCESSADOR cpu("CPU");
    cpu.clock(clock);
    cpu.reset(reset);

    sc_uint<16> programa[] = {
        0x7101,  // 0:  LD  R1, [R0+1] -> R1 = 8
        0x7202,  // 1:  LD  R2, [R0+2] -> R2 = 3
        0x7303,  // 2:  LD  R3, [R0+3] -> R3 = 7
        0x7404,  // 3:  LD  R4, [R0+4] -> R4 = 2
        0x4000,  // 4:  NOP
        0x4000,  // 5:  NOP
        0x4000,  // 6:  NOP

        // Sequencia de trocas equivalente ao bubble sort para [8, 3, 7, 2].
        // Swap R1/R2 -> [3, 8, 7, 2]
        0x4710,  // 7:  ADD R7, R1, R0
        0x4000,  // 8:  NOP
        0x4000,  // 9:  NOP
        0x4000,  // 10: NOP
        0x4120,  // 11: ADD R1, R2, R0
        0x4000,  // 12: NOP
        0x4000,  // 13: NOP
        0x4000,  // 14: NOP
        0x4270,  // 15: ADD R2, R7, R0
        0x4000,  // 16: NOP
        0x4000,  // 17: NOP
        0x4000,  // 18: NOP

        // Swap R2/R3 -> [3, 7, 8, 2]
        0x4720,  // 19: ADD R7, R2, R0
        0x4000,  // 20: NOP
        0x4000,  // 21: NOP
        0x4000,  // 22: NOP
        0x4230,  // 23: ADD R2, R3, R0
        0x4000,  // 24: NOP
        0x4000,  // 25: NOP
        0x4000,  // 26: NOP
        0x4370,  // 27: ADD R3, R7, R0
        0x4000,  // 28: NOP
        0x4000,  // 29: NOP
        0x4000,  // 30: NOP

        // Swap R3/R4 -> [3, 7, 2, 8]
        0x4730,  // 31: ADD R7, R3, R0
        0x4000,  // 32: NOP
        0x4000,  // 33: NOP
        0x4000,  // 34: NOP
        0x4340,  // 35: ADD R3, R4, R0
        0x4000,  // 36: NOP
        0x4000,  // 37: NOP
        0x4000,  // 38: NOP
        0x4470,  // 39: ADD R4, R7, R0
        0x4000,  // 40: NOP
        0x4000,  // 41: NOP
        0x4000,  // 42: NOP

        // Swap R2/R3 -> [3, 2, 7, 8]
        0x4720,  // 43: ADD R7, R2, R0
        0x4000,  // 44: NOP
        0x4000,  // 45: NOP
        0x4000,  // 46: NOP
        0x4230,  // 47: ADD R2, R3, R0
        0x4000,  // 48: NOP
        0x4000,  // 49: NOP
        0x4000,  // 50: NOP
        0x4370,  // 51: ADD R3, R7, R0
        0x4000,  // 52: NOP
        0x4000,  // 53: NOP
        0x4000,  // 54: NOP

        // Swap R1/R2 -> [2, 3, 7, 8]
        0x4710,  // 55: ADD R7, R1, R0
        0x4000,  // 56: NOP
        0x4000,  // 57: NOP
        0x4000,  // 58: NOP
        0x4120,  // 59: ADD R1, R2, R0
        0x4000,  // 60: NOP
        0x4000,  // 61: NOP
        0x4000,  // 62: NOP
        0x4270,  // 63: ADD R2, R7, R0
        0x4000,  // 64: NOP
        0x4000,  // 65: NOP
        0x4000,  // 66: NOP

        0x8001,  // 67: ST R1, [R0+1]
        0x8002,  // 68: ST R2, [R0+2]
        0x8003,  // 69: ST R3, [R0+3]
        0x8004,  // 70: ST R4, [R0+4]
        0x9000   // 71: J  0
    };
    int tam_programa = sizeof(programa) / sizeof(programa[0]);
    cpu.mem_instrucao->carregar_programa(programa, tam_programa);

    sc_int<16> vetor[] = {8, 3, 7, 2};
    cpu.mem_dados->carregar_dados(vetor, 4, 1);

    cout << "\n=== INICIANDO SIMULACAO: BUBBLE SORT ===" << endl;
    cout << "Vetor inicial: [8, 3, 7, 2] | Esperado: [2, 3, 7, 8]\n" << endl;

    reset.write(true);
    clock.write(false);
    sc_start(5, SC_NS);
    reset.write(false);

    for (int i = 0; i < 350; i++) {
        clock.write(true);
        sc_start(5, SC_NS);
        clock.write(false);
        sc_start(5, SC_NS);
    }

    cout << "\n=== SIMULACAO ENCERRADA ===" << endl;
    cpu.mem_dados->dump_memoria(0, 5);

    return 0;
}
