#include "processador.h"

// Atualização do PC síncrona
void PROCESSADOR::atualizar_pc() {
    if (reset.read()) pc.write(0);
    else if (!if_id_stall.read()) pc.write(pc_next.read());
}

// Lógica de cálculo do próximo endereço (PC+1 ou Salto)
void PROCESSADOR::calcular_next_pc() {
    pc_plus_1.write(pc.read() + 1);
    if (id_jump.read() || id_branch.read()) {
        pc_next.write(id_pc.read() + id_imm.read());
        if_id_flush.write(true);
        id_ex_flush.write(true);
    } else {
        pc_next.write(pc_plus_1.read());
        if_id_flush.write(false);
        id_ex_flush.write(false);
    }
}

void PROCESSADOR::selecionar_alu_input_b() {
    ex_alu_input_b.write(ex_alu_src.read() ? ex_imm.read() : ex_dado_rs2.read());
}

void PROCESSADOR::selecionar_wb_data() {
    wb_write_data.write(wb_mem_to_reg.read() ? wb_mem_data.read() : wb_alu_result.read());
}

PROCESSADOR::SC_CTOR(PROCESSADOR) {
    // Instanciação dos módulos
    mem_instrucao = new MEMORIA_INSTRUCAO("MEM_INSTR");
    mem_dados = new MEMORIA_DADOS("MEM_DADOS");
    banco_reg = new BANCO_REG("BANCO_REG");
    ula = new ULA("ULA");
    controle = new UNIDADE_CONTROLE("CONTROLE");
    hazard_unit = new HAZARD_DETECTION("HAZARD");
    reg_if_id = new REG_IF_ID("REG_IF_ID");
    reg_id_ex = new REG_ID_EX("REG_ID_EX");
    reg_ex_mem = new REG_EX_MEM("REG_EX_MEM");
    reg_mem_wb = new REG_MEM_WB("REG_MEM_WB");

    // --- CONEXÕES (Bindings) ---
    mem_instrucao->endereco(pc);
    mem_instrucao->instrucao(if_instrucao);

    reg_if_id->clock(clock);
    reg_if_id->flush(if_id_flush);
    reg_if_id->stall(if_id_stall);
    reg_if_id->instrucao_in(if_instrucao);
    reg_if_id->pc_in(pc_plus_1);
    reg_if_id->instrucao_out(id_instrucao);
    reg_if_id->pc_out(id_pc);

    controle->instrucao(id_instrucao);
    controle->flag_zero(mem_flag_zero); // Forwarding de flags do estágio MEM
    controle->flag_neg(mem_flag_neg);
    // ... Conecte as demais portas conforme seu processador.h original ...

    banco_reg->clock(clock);
    banco_reg->reg_read_1(id_rs1);
    banco_reg->reg_read_2(id_rs2);
    banco_reg->reg_write(wb_rd);
    banco_reg->res(wb_write_data);
    banco_reg->write(wb_reg_write);
    banco_reg->value_1(id_dado_rs1);
    banco_reg->value_2(id_dado_rs2);

    ula->a(ex_dado_rs1);
    ula->b(ex_alu_input_b);
    ula->op(ex_alu_op);
    ula->result(ex_alu_result);
    ula->flag_zero(ex_flag_zero);
    ula->flag_negative(ex_flag_neg);

    // Conecte os sinais de Pipeline REGS (ID/EX, EX/MEM, MEM/WB) seguindo o padrão
    
    SC_METHOD(atualizar_pc); sensitive << clock.pos() << reset;
    SC_METHOD(calcular_next_pc); sensitive << pc << id_jump << id_branch << id_imm << id_pc;
    SC_METHOD(selecionar_alu_input_b); sensitive << ex_alu_src << ex_imm << ex_dado_rs2;
    SC_METHOD(selecionar_wb_data); sensitive << wb_mem_to_reg << wb_mem_data << wb_alu_result;
}

PROCESSADOR::~PROCESSADOR() {
    // Delete todas as instâncias (mem_instrucao, mem_dados, etc.)
}