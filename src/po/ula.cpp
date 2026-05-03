#include "ula.h"

void ULA::process() {
    sc_int<16> value_a = a.read();
    sc_int<16> value_b = b.read();
    sc_uint<4> value_op = op.read();

    sc_int<16> res = 0;
    bool write_result = true;

    // Implementação das operações baseada nos opcodes da ISA
    switch(value_op.to_uint()){
        case 0: res = value_a & value_b; break; // AND
        case 1: res = value_a | value_b; break; // OR
        case 2: res = value_a ^ value_b; break; // XOR
        case 3: res = ~value_a; break;          // NOT
        case 4: res = value_a + value_b; break; // ADD
        case 5: res = value_a - value_b; break; // SUB
        case 6: 
            res = value_a - value_b;            // CMP (Comparação)
            write_result = false;               // CMP não escreve no registrador
            break;
    }
        
    flag_zero.write(res == 0);
    flag_negative.write(res < 0);

    if (write_result) {
        result.write(res);
    }
}
