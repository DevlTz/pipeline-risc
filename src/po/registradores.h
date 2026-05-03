#ifndef REGISTRADORES_H
#define REGISTRADORES_H

#include <systemc.h>

// Gerencia os 16 registradores de uso geral do processador
SC_MODULE(BANCO_REG) {
    sc_in<sc_uint<4>> reg_read_1;
    sc_in<sc_uint<4>> reg_read_2;
    sc_in<sc_uint<4>> reg_write;
    sc_in<sc_int<16>> res;

    sc_in<bool> write;
    sc_in<bool> clock;

    sc_out<sc_int<16>> value_1;
    sc_out<sc_int<16>> value_2;

    sc_int<16> regs[16];

    void read_reg();  // Leitura combinacional
    void write_reg(); // Escrita síncrona

    SC_CTOR(BANCO_REG) {
        SC_METHOD(read_reg);
        sensitive << reg_read_1 << reg_read_2;

        SC_METHOD(write_reg);
        sensitive << clock.pos(); // Escrita na borda de subida

        for (int i = 0; i < 16; i++) {
            regs[i] = 0;
        }
    }
};

#endif
