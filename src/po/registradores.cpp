#include <systemc.h>


// 16 Registradores

SC_MODULE(BANCO_REG){
    sc_in<sc_uint<4>> reg_read_1;
    sc_in<sc_uint<4>> reg_read_2;
    sc_in<sc_uint<4>> reg_write;
    sc_in<sc_int<16>> res;

    sc_in<bool> write;
    sc_in<bool> clock;

    sc_out<sc_int<16>> value_1;
    sc_out<sc_int<16>> value_2;

    sc_int<16> regs[16];

    void read_reg(){
        sc_uint<4> reg_1 = reg_read_1.read();
        sc_uint<4> reg_2 = reg_read_2.read();

        value_1.write(regs[reg_1]);
        value_2.write(regs[reg_2]);
    }

    void write_reg(){
        if(write.read()){
            sc_uint<4> reg = reg_write.read();
            sc_int<16> result = res.read();
            regs[reg] = result;
        }
    }

    SC_CTOR(BANCO_REG){
        SC_METHOD(read_reg);
        sensitive << reg_read_1 << reg_read_2;

        SC_METHOD(write_reg);
        sensitive << clock.pos();

        // inicialização
        for (int i = 0; i < 16; i++){
            regs[i] = 0;
        }
    }
};