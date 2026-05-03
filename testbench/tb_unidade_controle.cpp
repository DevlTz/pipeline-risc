#include <systemc.h>
#include "../src/pc/unidade_controle.h"
#include <iostream>

using namespace std;

int sc_main(int argc, char* argv[]) {
    // Sinais para conectar ao módulo
    sc_signal<sc_uint<16>> t_instrucao;
    sc_signal<bool> t_flag_zero;
    sc_signal<bool> t_flag_neg;

    sc_signal<sc_uint<4>> t_opcode, t_rd, t_rs1, t_rs2, t_alu_op;
    sc_signal<sc_int<16>> t_imediato;
    sc_signal<bool> t_alu_src, t_mem_read, t_mem_write, t_reg_write, t_mem_to_reg, t_branch, t_jump;

    // Instanciação do módulo
    UNIDADE_CONTROLE uc("UC");
    uc.instrucao(t_instrucao);
    uc.flag_zero(t_flag_zero);
    uc.flag_neg(t_flag_neg);
    uc.opcode(t_opcode);
    uc.reg_destino(t_rd);
    uc.reg_fonte1(t_rs1);
    uc.reg_fonte2(t_rs2);
    uc.imediato(t_imediato);
    uc.alu_op(t_alu_op);
    uc.alu_src(t_alu_src);
    uc.mem_read(t_mem_read);
    uc.mem_write(t_mem_write);
    uc.reg_write(t_reg_write);
    uc.mem_to_reg(t_mem_to_reg);
    uc.branch(t_branch);
    uc.jump(t_jump);

    cout << "=== INICIANDO TESTE UNITARIO: UNIDADE DE CONTROLE ===\n" << endl;

    // --- CASO 1: ADD R1, R2, R3 (0x4123) ---
    // Opcode 4 = ADD
    t_instrucao.write(0x4123);
    t_flag_zero.write(false);
    t_flag_neg.write(false);
    sc_start(1, SC_NS); // Processa a lógica combinacional

    cout << "Instrucao: 0x4123 (ADD)" << endl;
    cout << "  ALU_OP esperado: 4 | Obtido: " << t_alu_op.read() << endl;
    cout << "  RegWrite esperado: 1 | Obtido: " << t_reg_write.read() << endl;
    cout << "  ALU_SRC esperado: 0 (Reg) | Obtido: " << t_alu_src.read() << endl;
    cout << "------------------------------------------" << endl;

    // --- CASO 2: LD R3, 0(R0) (0x7300) ---
    // Opcode 7 = LD
    t_instrucao.write(0x7300);
    sc_start(1, SC_NS);

    cout << "Instrucao: 0x7300 (LD)" << endl;
    cout << "  ALU_OP esperado: 4 (ADD p/ end.) | Obtido: " << t_alu_op.read() << endl;
    cout << "  MemRead esperado: 1 | Obtido: " << t_mem_read.read() << endl;
    cout << "  MemToReg esperado: 1 | Obtido: " << t_mem_to_reg.read() << endl;
    cout << "------------------------------------------" << endl;

    // --- CASO 3: JZ +5 (0xB005) com Flag Zero = 1 ---
    // Opcode B = JZ
    t_instrucao.write(0xB005);
    t_flag_zero.write(true); 
    sc_start(1, SC_NS);

    cout << "Instrucao: 0xB005 (JZ) com Flag_Zero=1" << endl;
    cout << "  Branch esperado: 1 | Obtido: " << t_branch.read() << endl;
    cout << "  Imediato esperado: 5 | Obtido: " << t_imediato.read() << endl;
    cout << "------------------------------------------" << endl;

    cout << "\n=== FIM DOS TESTES ===\n" << endl;

    return 0;
}