#ifndef PROCESSADOR_H
#define PROCESSADOR_H

#include <systemc.h>
#include "../po/memoria.h"
#include "../po/pipeline_regs.h"
#include "../po/ula.h"
#include "../po/registradores.h"
#include "../pc/unidade_controle.h"

SC_MODULE(PROCESSADOR) {
    sc_in<bool> clock;
    sc_in<bool> reset;
    
    // ======== SINAIS INTERNOS ========
    sc_signal<sc_uint<8>> pc, pc_next, pc_plus_1, pc_branch;
    
    // IF Stage
    sc_signal<sc_uint<16>> if_instrucao;
    
    // IF/ID Pipeline
    sc_signal<sc_uint<16>> id_instrucao;
    sc_signal<sc_uint<8>> id_pc;
    sc_signal<bool> if_id_flush;          // sinal final que vai ao registrador IF/ID
    sc_signal<bool> if_id_stall;
    sc_signal<bool> hazard_flush;         // flush vindo da unidade de hazard
    sc_signal<bool> branch_flush;         // flush vindo do salto
    
    // ID Stage - Sinais da Unidade de Controle
    sc_signal<sc_uint<4>> id_opcode, id_rd, id_rs1, id_rs2, id_alu_op;
    sc_signal<sc_int<16>> id_imm, id_dado_rs1, id_dado_rs2;
    sc_signal<bool> id_alu_src, id_mem_read, id_mem_write, id_reg_write, id_mem_to_reg, id_branch, id_jump;
    
    // ID/EX Pipeline
    sc_signal<sc_int<16>> ex_dado_rs1, ex_dado_rs2, ex_imm;
    sc_signal<sc_uint<4>> ex_rd, ex_alu_op;
    sc_signal<sc_uint<8>> ex_pc;
    sc_signal<bool> ex_alu_src, ex_mem_read, ex_mem_write, ex_reg_write, ex_mem_to_reg;
    sc_signal<bool> id_ex_flush;          // sinal final que vai ao registrador ID/EX
    sc_signal<bool> branch_id_ex_flush;   // flush ID/EX vindo do salto
    
    // EX Stage - ULA
    sc_signal<sc_int<16>> ex_alu_input_b, ex_alu_result;
    sc_signal<bool> ex_flag_zero, ex_flag_neg;
    
    // EX/MEM Pipeline
    sc_signal<sc_int<16>> mem_alu_result, mem_dado_rs2;
    sc_signal<sc_uint<8>> mem_endereco;   // endereço de 8 bits para memória de dados
    sc_signal<sc_uint<4>> mem_rd;
    sc_signal<bool> mem_flag_zero, mem_flag_neg, mem_mem_read, mem_mem_write, mem_reg_write, mem_mem_to_reg;
    
    // MEM Stage - Memória de Dados
    sc_signal<sc_int<16>> mem_dado_lido;
    
    // MEM/WB Pipeline
    sc_signal<sc_int<16>> wb_alu_result, wb_mem_data;
    sc_signal<sc_uint<4>> wb_rd;
    sc_signal<bool> wb_reg_write, wb_mem_to_reg;
    
    sc_signal<sc_int<16>> wb_write_data;
    
    // ======== INSTÂNCIAS ========
    MEMORIA_INSTRUCAO *mem_instrucao;
    MEMORIA_DADOS *mem_dados;
    BANCO_REG *banco_reg;
    ULA *ula;
    UNIDADE_CONTROLE *controle;
    HAZARD_DETECTION *hazard_unit;
    
    REG_IF_ID *reg_if_id;
    REG_ID_EX *reg_id_ex;
    REG_EX_MEM *reg_ex_mem;
    REG_MEM_WB *reg_mem_wb;
    
    // ======== MÉTODOS ========
    void atualizar_pc();
    void calcular_next_pc();
    void selecionar_alu_input_b();
    void selecionar_wb_data();
    void converter_endereco_mem();
    void combinar_flushes();
    void debug_estado();

    
    SC_HAS_PROCESS(PROCESSADOR);
    PROCESSADOR(sc_module_name name);
    ~PROCESSADOR();
};

#endif