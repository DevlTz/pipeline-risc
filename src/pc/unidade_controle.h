#ifndef UNIDADE_CONTROLE_H
#define UNIDADE_CONTROLE_H

#include <systemc.h>

/*
 * FORMATO DAS INSTRUÇÕES (16 bits):
 * 
 * TIPO R (Registrador-Registrador): AND, OR, XOR, NOT, ADD, SUB, CMP
 * | opcode (4) | Rd (4) | Rs1 (4) | Rs2 (4) |
 * 
 * TIPO I (Imediato/Memória): LD, ST, J, JN, JZ
 * | opcode (4) | Rd/Rs (4) | Rs_base (4) | imediato (4) |
 * 
 * OPCODES:
 * 0000 (0x0) - AND
 * 0001 (0x1) - OR
 * 0010 (0x2) - XOR
 * 0011 (0x3) - NOT
 * 0100 (0x4) - ADD
 * 0101 (0x5) - SUB
 * 0110 (0x6) - CMP
 * 0111 (0x7) - LD
 * 1000 (0x8) - ST
 * 1001 (0x9) - J
 * 1010 (0xA) - JN
 * 1011 (0xB) - JZ
 */

SC_MODULE(UNIDADE_CONTROLE) {
    // Entradas
    sc_in<sc_uint<16>> instrucao;
    sc_in<bool> flag_zero;
    sc_in<bool> flag_neg;
    
    // Saídas - Campos decodificados
    sc_out<sc_uint<4>> opcode;
    sc_out<sc_uint<4>> reg_destino;
    sc_out<sc_uint<4>> reg_fonte1;
    sc_out<sc_uint<4>> reg_fonte2;
    sc_out<sc_int<16>> imediato;
    
    // Saídas - Sinais de controle
    sc_out<sc_uint<4>> alu_op;
    sc_out<bool> alu_src;
    sc_out<bool> mem_read;
    sc_out<bool> mem_write;
    sc_out<bool> reg_write;
    sc_out<bool> mem_to_reg;
    sc_out<bool> branch;
    sc_out<bool> jump;
    
    void decodificar();

    SC_CTOR(UNIDADE_CONTROLE) {
        SC_METHOD(decodificar);
        sensitive << instrucao << flag_zero << flag_neg;
    }
};

SC_MODULE(HAZARD_DETECTION) {
    // Entradas
    sc_in<sc_uint<4>> id_rs1;
    sc_in<sc_uint<4>> id_rs2;
    sc_in<sc_uint<4>> ex_rd;
    sc_in<bool> ex_mem_read;
    
    // Saídas
    sc_out<bool> stall;
    sc_out<bool> flush_if_id;
    
    void detectar();

    SC_CTOR(HAZARD_DETECTION) {
        SC_METHOD(detectar);
        sensitive << id_rs1 << id_rs2 << ex_rd << ex_mem_read;
    }
};

#endif