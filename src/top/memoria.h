#ifndef MEMORIA_H
#define MEMORIA_H

#include <systemc.h>

// ============================================================
// MEMÓRIA DE INSTRUÇÕES
// ============================================================
SC_MODULE(MEMORIA_INSTRUCAO) {
    // Portas
    sc_in<sc_uint<8>> endereco;           // Endereço de leitura (PC)
    sc_out<sc_uint<16>> instrucao;        // Instrução de 16 bits
    
    // Memória interna (256 palavras de 16 bits)
    sc_uint<16> memoria[256];
    
    // Processo de leitura
    void leitura() {
        sc_uint<8> addr = endereco.read();
        instrucao.write(memoria[addr]);
    }
    
    // Método para carregar programa na memória
    void carregar_programa(sc_uint<16> *programa, int tamanho) {
        for (int i = 0; i < tamanho && i < 256; i++) {
            memoria[i] = programa[i];
        }
    }
    
    // Construtor
    SC_CTOR(MEMORIA_INSTRUCAO) {
        SC_METHOD(leitura);
        sensitive << endereco;
        
        // Inicializar memória com NOPs (0x0000)
        for (int i = 0; i < 256; i++) {
            memoria[i] = 0x0000;
        }
    }
};


// ============================================================
// MEMÓRIA DE DADOS (RAM - Random Access Memory)
// ============================================================
SC_MODULE(MEMORIA_DADOS) {
    // Portas
    sc_in<sc_uint<8>> endereco;           // Endereço
    sc_in<sc_int<16>> dado_entrada;       // Dado para escrever
    sc_in<bool> mem_read;                 // Sinal de leitura
    sc_in<bool> mem_write;                // Sinal de escrita
    sc_in<bool> clock;                    // Clock
    
    sc_out<sc_int<16>> dado_saida;        // Dado lido
    
    // Memória interna (256 palavras de 16 bits)
    sc_int<16> memoria[256];
    
    // Processo de leitura (combinacional)
    void leitura() {
        if (mem_read.read()) {
            sc_uint<8> addr = endereco.read();
            dado_saida.write(memoria[addr]);
        } else {
            dado_saida.write(0);
        }
    }
    
    // Processo de escrita (síncrono com clock)
    void escrita() {
        if (mem_write.read()) {
            sc_uint<8> addr = endereco.read();
            sc_int<16> dado = dado_entrada.read();
            memoria[addr] = dado;
            
            cout << "@" << sc_time_stamp() 
                 << " [MEM] Escrita: MEM[" << addr.to_uint() 
                 << "] = " << dado << endl;
        }
    }
    
    // Método para inicializar dados na memória
    void carregar_dados(sc_int<16> *dados, int tamanho, int offset = 0) {
        for (int i = 0; i < tamanho && (i + offset) < 256; i++) {
            memoria[i + offset] = dados[i];
        }
    }
    
    // Método para imprimir conteúdo da memória (debug)
    void dump_memoria(int inicio, int fim) {
        cout << "\n=== DUMP MEMÓRIA DE DADOS ===" << endl;
        for (int i = inicio; i <= fim && i < 256; i++) {
            cout << "MEM[" << i << "] = " << memoria[i] << endl;
        }
        cout << "============================\n" << endl;
    }
    
    // Construtor
    SC_CTOR(MEMORIA_DADOS) {
        SC_METHOD(leitura);
        sensitive << endereco << mem_read;
        
        SC_METHOD(escrita);
        sensitive << clock.pos();
        
        // Inicializar memória com zeros
        for (int i = 0; i < 256; i++) {
            memoria[i] = 0;
        }
    }
};

#endif // MEMORIA_H
