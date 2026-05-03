#include <systemc.h>
#include "../src/po/ula.h"

int sc_main(int argc, char* argv[]) {
    sc_signal<sc_int<16>> t_a, t_b, t_res;
    sc_signal<sc_uint<4>> t_op;
    sc_signal<bool> t_z, t_n;

    ULA ula_test("ULA");
    ula_test.a(t_a); ula_test.b(t_b); ula_test.op(t_op);
    ula_test.result(t_res); ula_test.flag_zero(t_z); ula_test.flag_negative(t_n);

    // Testando SOMA: 10 + 5 = 15
    t_a.write(10); t_b.write(5); t_op.write(4);
    sc_start(1, SC_NS);
    cout << "ADD Test: " << (t_res.read() == 15 ? "PASS" : "FAIL") << " (Result: " << t_res.read() << ")" << endl;

    // Testando CMP: 5 - 10 (Negativo)
    t_a.write(5); t_b.write(10); t_op.write(6);
    sc_start(1, SC_NS);
    cout << "CMP Test (Flag Neg): " << (t_n.read() == true ? "PASS" : "FAIL") << endl;

    return 0;
}