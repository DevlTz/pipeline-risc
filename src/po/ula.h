#ifndef ULA_H
#define ULA_H

#include <systemc.h>

// Módulo responsável pelas operações matemáticas e lógicas
SC_MODULE(ULA) {
    sc_in<sc_int<16>> a;
    sc_in<sc_int<16>> b;
    sc_in<sc_uint<4>> op;

    sc_out<sc_int<16>> result;
    sc_out<bool> flag_zero;
    sc_out<bool> flag_negative;

    void process(); // Declaração do método de processamento

    SC_CTOR(ULA) {
        SC_METHOD(process);
        sensitive << a << b << op;
    }
};

#endif
