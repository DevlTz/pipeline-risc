#include <systemc.h>
#include "processador.h"

int sc_main(int argc, char* argv[]) {
    sc_signal<bool> clock;
    sc_signal<bool> reset;
    
    PROCESSADOR cpu("CPU");
    cpu.clock(clock);
    cpu.reset(reset);
    
    // ============================================
    // PROGRAMA: SOMA VETORIAL
    // ============================================
    sc_uint<16> programa[] = {
        0x710A,  // 0: LD R1, [R0+10]   ; R1 = endereço base
        0x720B,  // 1: LD R2, [R0+11]   ; R2 = N
        0x4300,  // 2: ADD R3, R0, R0   ; R3 = 0 (i)
        0x4400,  // 3: ADD R4, R0, R0   ; R4 = 0 (soma)
        0x5732,  // 4: SUB R7, R3, R2   ; R7 = i - N
        0xB006,  // 5: JZ +6            ; se i == N, sai
        0x7510,  // 6: LD R5, [R1+0]    ; R5 = vetor[i]
        0x4445,  // 7: ADD R4, R4, R5   ; soma += R5
        0x4111,  // 8: ADD R1, R1, 1    ; ponteiro++
        0x4331,  // 9: ADD R3, R3, 1    ; i++
        0x900A,  // 10: J -6            ; volta ao LOOP
        0x8404,  // 11: ST R4, [R0+4]   ; mem[4] = soma
        0x9000   // 12: J 0             ; parada (loop infinito)
    };
    int tam_programa = sizeof(programa) / sizeof(programa[0]);
    cpu.mem_instrucao->carregar_programa(programa, tam_programa);
    
    // ============================================
    // DADOS INICIAIS NA MEMÓRIA
    // ============================================
    sc_int<16> vetor[] = {3, 7, 2, 8, 5};  // soma = 25
    cpu.mem_dados->carregar_dados(vetor, 5, 20);  // posições 20..24
    
    sc_int<16> base = 20;
    sc_int<16> tamanho = 5;
    cpu.mem_dados->carregar_dados(&base, 1, 10);     // mem[10] = 20
    cpu.mem_dados->carregar_dados(&tamanho, 1, 11);  // mem[11] = 5
    
    // ============================================
    // EXECUÇÃO
    // ============================================
    cout << "\n=== INICIANDO SIMULAÇÃO: SOMA VETORIAL ===" << endl;
    cout << "Vetor: [3, 7, 2, 8, 5] | Soma esperada: 25\n" << endl;
    
    reset.write(true);
    clock.write(false);
    sc_start(5, SC_NS);
    reset.write(false);
    
    // Roda por ciclos suficientes (cada iteração ~7 ciclos × 5 elementos + setup ≈ 50 ciclos)
    for (int i = 0; i < 100; i++) {
        clock.write(true);
        sc_start(5, SC_NS);
        clock.write(false);
        sc_start(5, SC_NS);
    }
    
    // ============================================
    // RESULTADO
    // ============================================
    cout << "\n=== SIMULAÇÃO ENCERRADA ===" << endl;
    cpu.mem_dados->dump_memoria(0, 25);
    
    return 0;
}