#ifndef PIPELINE_REGS_H
#define PIPELINE_REGS_H

#include <systemc.h>

// ============================================================
// REGISTRADOR IF/ID (Instruction Fetch / Instruction Decode)
// ============================================================
SC_MODULE(REG_IF_ID) {
    // Entradas
    sc_in<bool> clock;
    sc_in<bool> flush;                    // Limpar registrador (para saltos)
    sc_in<bool> stall;                    // Congelar registrador (para hazards)
    sc_in<sc_uint<16>> instrucao_in;      // Instrução vinda da memória
    sc_in<sc_uint<8>> pc_in;              // PC + 1
    
    // Saídas
    sc_out<sc_uint<16>> instrucao_out;
    sc_out<sc_uint<8>> pc_out;
    
    void processo() {
        if (flush.read()) {
            // Inserir bolha (NOP)
            instrucao_out.write(0x0000);
            pc_out.write(0);
        } else if (!stall.read()) {
            // Propagar valores
            instrucao_out.write(instrucao_in.read());
            pc_out.write(pc_in.read());
        }
        // Se stall == 1, mantém valores atuais (não atualiza)
    }
    
    SC_CTOR(REG_IF_ID) {
        SC_METHOD(processo);
        sensitive << clock.pos();
    }
};


// ============================================================
// REGISTRADOR ID/EX (Instruction Decode / Execute)
// ============================================================
SC_MODULE(REG_ID_EX) {
    // Entradas
    sc_in<bool> clock;
    sc_in<bool> flush;
    
    // Dados
    sc_in<sc_int<16>> dado_rs1_in;
    sc_in<sc_int<16>> dado_rs2_in;
    sc_in<sc_int<16>> imediato_in;
    sc_in<sc_uint<4>> reg_destino_in;
    sc_in<sc_uint<8>> pc_in;
    
    // Sinais de controle
    sc_in<sc_uint<4>> alu_op_in;
    sc_in<bool> alu_src_in;              // 0 = reg, 1 = imediato
    sc_in<bool> mem_read_in;
    sc_in<bool> mem_write_in;
    sc_in<bool> reg_write_in;
    sc_in<bool> mem_to_reg_in;           // 0 = ALU, 1 = memoria
    
    // Saídas
    sc_out<sc_int<16>> dado_rs1_out;
    sc_out<sc_int<16>> dado_rs2_out;
    sc_out<sc_int<16>> imediato_out;
    sc_out<sc_uint<4>> reg_destino_out;
    sc_out<sc_uint<8>> pc_out;
    
    sc_out<sc_uint<4>> alu_op_out;
    sc_out<bool> alu_src_out;
    sc_out<bool> mem_read_out;
    sc_out<bool> mem_write_out;
    sc_out<bool> reg_write_out;
    sc_out<bool> mem_to_reg_out;
    
    void processo() {
        if (flush.read()) {
            // Bolha - zerar sinais de controle
            dado_rs1_out.write(0);
            dado_rs2_out.write(0);
            imediato_out.write(0);
            reg_destino_out.write(0);
            pc_out.write(0);
            alu_op_out.write(0);
            alu_src_out.write(0);
            mem_read_out.write(0);
            mem_write_out.write(0);
            reg_write_out.write(0);
            mem_to_reg_out.write(0);
        } else {
            // Propagar
            dado_rs1_out.write(dado_rs1_in.read());
            dado_rs2_out.write(dado_rs2_in.read());
            imediato_out.write(imediato_in.read());
            reg_destino_out.write(reg_destino_in.read());
            pc_out.write(pc_in.read());
            alu_op_out.write(alu_op_in.read());
            alu_src_out.write(alu_src_in.read());
            mem_read_out.write(mem_read_in.read());
            mem_write_out.write(mem_write_in.read());
            reg_write_out.write(reg_write_in.read());
            mem_to_reg_out.write(mem_to_reg_in.read());
        }
    }
    
    SC_CTOR(REG_ID_EX) {
        SC_METHOD(processo);
        sensitive << clock.pos();
    }
};


// ============================================================
// REGISTRADOR EX/MEM (Execute / Memory)
// ============================================================
SC_MODULE(REG_EX_MEM) {
    // Entradas
    sc_in<bool> clock;
    
    sc_in<sc_int<16>> resultado_alu_in;
    sc_in<sc_int<16>> dado_rs2_in;       // Para ST (escrita na memória)
    sc_in<sc_uint<4>> reg_destino_in;
    sc_in<bool> flag_zero_in;
    sc_in<bool> flag_neg_in;
    
    // Sinais de controle
    sc_in<bool> mem_read_in;
    sc_in<bool> mem_write_in;
    sc_in<bool> reg_write_in;
    sc_in<bool> mem_to_reg_in;
    
    // Saídas
    sc_out<sc_int<16>> resultado_alu_out;
    sc_out<sc_int<16>> dado_rs2_out;
    sc_out<sc_uint<4>> reg_destino_out;
    sc_out<bool> flag_zero_out;
    sc_out<bool> flag_neg_out;
    
    sc_out<bool> mem_read_out;
    sc_out<bool> mem_write_out;
    sc_out<bool> reg_write_out;
    sc_out<bool> mem_to_reg_out;
    
    void processo() {
        resultado_alu_out.write(resultado_alu_in.read());
        dado_rs2_out.write(dado_rs2_in.read());
        reg_destino_out.write(reg_destino_in.read());
        flag_zero_out.write(flag_zero_in.read());
        flag_neg_out.write(flag_neg_in.read());
        mem_read_out.write(mem_read_in.read());
        mem_write_out.write(mem_write_in.read());
        reg_write_out.write(reg_write_in.read());
        mem_to_reg_out.write(mem_to_reg_in.read());
    }
    
    SC_CTOR(REG_EX_MEM) {
        SC_METHOD(processo);
        sensitive << clock.pos();
    }
};


// ============================================================
// REGISTRADOR MEM/WB (Memory / Write Back)
// ============================================================
SC_MODULE(REG_MEM_WB) {
    // Entradas
    sc_in<bool> clock;
    
    sc_in<sc_int<16>> resultado_alu_in;
    sc_in<sc_int<16>> dado_memoria_in;
    sc_in<sc_uint<4>> reg_destino_in;
    
    // Sinais de controle
    sc_in<bool> reg_write_in;
    sc_in<bool> mem_to_reg_in;
    
    // Saídas
    sc_out<sc_int<16>> resultado_alu_out;
    sc_out<sc_int<16>> dado_memoria_out;
    sc_out<sc_uint<4>> reg_destino_out;
    
    sc_out<bool> reg_write_out;
    sc_out<bool> mem_to_reg_out;
    
    void processo() {
        resultado_alu_out.write(resultado_alu_in.read());
        dado_memoria_out.write(dado_memoria_in.read());
        reg_destino_out.write(reg_destino_in.read());
        reg_write_out.write(reg_write_in.read());
        mem_to_reg_out.write(mem_to_reg_in.read());
    }
    
    SC_CTOR(REG_MEM_WB) {
        SC_METHOD(processo);
        sensitive << clock.pos();
    }
};

#endif // PIPELINE_REGS_H
