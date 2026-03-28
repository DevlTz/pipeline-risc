#include <systemc.h>

// sc_uint -> unsigned int
// sc_int -> signed int
// sc_in -> sinal in
// sc_out -> sinal out
// bool

// Temos que ler e escrever sinais
// Para isso usamos read() e write()

// SC_MODULE -> Módulo (classe)
// SC_CTOR -> Construtor 
// SC_METHOD -> função a executar

// Temos que colocar os valores com 0b no começo para saber que é binário


SC_MODULE(ULA){
    sc_in<sc_int<16>> a;
    sc_in<sc_int<16>> b;
    sc_in<sc_uint<4>> op;

    sc_out<sc_int<16>> result;
    sc_out<bool> flag_zero;
    sc_out<bool> flag_negative;

    

    void process(){
        sc_int<16> value_a = a.read();
        sc_int<16> value_b = b.read();
        sc_uint<4> value_op = op.read();

        sc_int<16> res = 0;
        bool write_result = true;

        switch(value_op){
            // AND
            case 0: 
                res = value_a & value_b;
                break;
            // OR 
            case 1: 
                res = value_a | value_b;
                break;
            // XOR 
            case 2:
                res = value_a ^ value_b;
                break;
            // NOT
            case 3:
                res = ~value_a;
                break;
            // ADD
            case 4:
                res = value_a + value_b;
                break;
            // SUB
            case 5:
                res = value_a - value_b;
                break;
            // CMP
            case 6:
                res = value_a - value_b;
                write_result = false;
                break;
            };
            
        flag_zero.write(res == 0);
        flag_negative.write(res < 0);

        if (write_result){
            result.write(res);
        }
    }


    SC_CTOR(ULA){
        SC_METHOD(process);
        sensitive << a << b << op; // Toda vez que algum desses mudar de valor, vai executar o process
    }
};