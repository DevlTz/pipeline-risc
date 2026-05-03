#include "registradores.h"

void BANCO_REG::read_reg() {
    sc_uint<4> reg_1 = reg_read_1.read();
    sc_uint<4> reg_2 = reg_read_2.read();

    value_1.write(regs[reg_1]);
    value_2.write(regs[reg_2]);
}

void BANCO_REG::write_reg() {
    if(write.read()){
        sc_uint<4> reg = reg_write.read();
        sc_int<16> result = res.read();
        
        // Proteção opcional: evitar escrita no R0 se ele for constante zero
        if (reg != 0) {
            regs[reg] = result;
        }
    }
}