#include "processador.h"

void PROCESSADOR::atualizar_pc() {
    if (reset.read()) pc.write(0);
    else if (!if_id_stall.read()) pc.write(pc_next.read());
}

// Lógica de cálculo do próximo endereço (PC+1 ou Salto)
void PROCESSADOR::calcular_next_pc() {
    pc_plus_1.write(pc.read() + 1);
    if (id_jump.read() || id_branch.read()) {
        pc_next.write(id_pc.read() + id_imm.read());
        branch_flush.write(true);
        branch_id_ex_flush.write(true);
    } else {
        pc_next.write(pc_plus_1.read());
        branch_flush.write(false);
        branch_id_ex_flush.write(false);
    }
}

void PROCESSADOR::selecionar_alu_input_b() {
    ex_alu_input_b.write(ex_alu_src.read() ? ex_imm.read() : ex_dado_rs2.read());
}

void PROCESSADOR::selecionar_wb_data() {
    wb_write_data.write(wb_mem_to_reg.read() ? wb_mem_data.read() : wb_alu_result.read());
}

void PROCESSADOR::converter_endereco_mem() {
    // Pega só os 8 bits menos significativos do resultado da ULA
    mem_endereco.write((sc_uint<8>)(mem_alu_result.read() & 0xFF));
}

void PROCESSADOR::combinar_flushes() {
    // OR lógico: flush ocorre se houver hazard OU salto
    if_id_flush.write(hazard_flush.read() || branch_flush.read());
    id_ex_flush.write(branch_id_ex_flush.read());
    // Nota: hazard de dados NÃO faz flush do ID/EX 
    // (ele apenas congela o IF/ID e insere bolha via stall)
}

void PROCESSADOR::debug_estado() {
    cout << "@" << sc_time_stamp()
         << " | PC=" << pc.read()
         << " | IF=0x" << hex << if_instrucao.read()
         << " | ID=0x" << id_instrucao.read() << dec
         << " | stall=" << if_id_stall.read()
         << " | flush=" << if_id_flush.read()
         << " | jump=" << id_jump.read()
         << " | branch=" << id_branch.read()
         << " | EX_res=" << ex_alu_result.read()
         << " | MEM_W=" << mem_mem_write.read()
         << endl;
}


PROCESSADOR::PROCESSADOR(sc_module_name name) : sc_module(name) {
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
    controle->flag_zero(mem_flag_zero);
    controle->flag_neg(mem_flag_neg);
    controle->opcode(id_opcode);
    controle->reg_destino(id_rd);
    controle->reg_fonte1(id_rs1);
    controle->reg_fonte2(id_rs2);
    controle->imediato(id_imm);
    controle->alu_op(id_alu_op);
    controle->alu_src(id_alu_src);
    controle->mem_read(id_mem_read);
    controle->mem_write(id_mem_write);
    controle->reg_write(id_reg_write);
    controle->mem_to_reg(id_mem_to_reg);
    controle->branch(id_branch);
    controle->jump(id_jump);

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

    // Hazard Detection Unit
    hazard_unit->id_rs1(id_rs1);
    hazard_unit->id_rs2(id_rs2);
    hazard_unit->ex_rd(ex_rd);
    hazard_unit->ex_mem_read(ex_mem_read);
    hazard_unit->stall(if_id_stall);
    hazard_unit->flush_if_id(hazard_flush);

    // ID/EX Pipeline Register
    reg_id_ex->clock(clock);
    reg_id_ex->flush(id_ex_flush);
    reg_id_ex->dado_rs1_in(id_dado_rs1);
    reg_id_ex->dado_rs2_in(id_dado_rs2);
    reg_id_ex->imediato_in(id_imm);
    reg_id_ex->reg_destino_in(id_rd);
    reg_id_ex->pc_in(id_pc);
    reg_id_ex->alu_op_in(id_alu_op);
    reg_id_ex->alu_src_in(id_alu_src);
    reg_id_ex->mem_read_in(id_mem_read);
    reg_id_ex->mem_write_in(id_mem_write);
    reg_id_ex->reg_write_in(id_reg_write);
    reg_id_ex->mem_to_reg_in(id_mem_to_reg);
    reg_id_ex->dado_rs1_out(ex_dado_rs1);
    reg_id_ex->dado_rs2_out(ex_dado_rs2);
    reg_id_ex->imediato_out(ex_imm);
    reg_id_ex->reg_destino_out(ex_rd);
    reg_id_ex->pc_out(ex_pc);
    reg_id_ex->alu_op_out(ex_alu_op);
    reg_id_ex->alu_src_out(ex_alu_src);
    reg_id_ex->mem_read_out(ex_mem_read);
    reg_id_ex->mem_write_out(ex_mem_write);
    reg_id_ex->reg_write_out(ex_reg_write);
    reg_id_ex->mem_to_reg_out(ex_mem_to_reg);

    // EX/MEM Pipeline Register
    reg_ex_mem->clock(clock);
    reg_ex_mem->resultado_alu_in(ex_alu_result);
    reg_ex_mem->dado_rs2_in(ex_dado_rs2);
    reg_ex_mem->reg_destino_in(ex_rd);
    reg_ex_mem->flag_zero_in(ex_flag_zero);
    reg_ex_mem->flag_neg_in(ex_flag_neg);
    reg_ex_mem->mem_read_in(ex_mem_read);
    reg_ex_mem->mem_write_in(ex_mem_write);
    reg_ex_mem->reg_write_in(ex_reg_write);
    reg_ex_mem->mem_to_reg_in(ex_mem_to_reg);
    reg_ex_mem->resultado_alu_out(mem_alu_result);
    reg_ex_mem->dado_rs2_out(mem_dado_rs2);
    reg_ex_mem->reg_destino_out(mem_rd);
    reg_ex_mem->flag_zero_out(mem_flag_zero);
    reg_ex_mem->flag_neg_out(mem_flag_neg);
    reg_ex_mem->mem_read_out(mem_mem_read);
    reg_ex_mem->mem_write_out(mem_mem_write);
    reg_ex_mem->reg_write_out(mem_reg_write);
    reg_ex_mem->mem_to_reg_out(mem_mem_to_reg);

    // Memória de Dados
    mem_dados->clock(clock);
    mem_dados->endereco(mem_endereco);
    mem_dados->dado_entrada(mem_dado_rs2);
    mem_dados->mem_read(mem_mem_read);
    mem_dados->mem_write(mem_mem_write);
    mem_dados->dado_saida(mem_dado_lido);

    // MEM/WB Pipeline Register
    reg_mem_wb->clock(clock);
    reg_mem_wb->resultado_alu_in(mem_alu_result);
    reg_mem_wb->dado_memoria_in(mem_dado_lido);
    reg_mem_wb->reg_destino_in(mem_rd);
    reg_mem_wb->reg_write_in(mem_reg_write);
    reg_mem_wb->mem_to_reg_in(mem_mem_to_reg);
    reg_mem_wb->resultado_alu_out(wb_alu_result);
    reg_mem_wb->dado_memoria_out(wb_mem_data);
    reg_mem_wb->reg_destino_out(wb_rd);
    reg_mem_wb->reg_write_out(wb_reg_write);
    reg_mem_wb->mem_to_reg_out(wb_mem_to_reg);
    

    SC_METHOD(atualizar_pc); sensitive << clock.pos() << reset;
    SC_METHOD(calcular_next_pc); sensitive << pc << id_jump << id_branch << id_imm << id_pc;
    SC_METHOD(selecionar_alu_input_b); sensitive << ex_alu_src << ex_imm << ex_dado_rs2;
    SC_METHOD(selecionar_wb_data); sensitive << wb_mem_to_reg << wb_mem_data << wb_alu_result;
    SC_METHOD(converter_endereco_mem); sensitive << mem_alu_result;
    SC_METHOD(combinar_flushes); sensitive << hazard_flush << branch_flush << branch_id_ex_flush;
    SC_METHOD(debug_estado); sensitive << clock.pos();

}

PROCESSADOR::~PROCESSADOR() {
    delete mem_instrucao;
    delete mem_dados;
    delete banco_reg;
    delete ula;
    delete controle;
    delete hazard_unit;
    delete reg_if_id;
    delete reg_id_ex;
    delete reg_ex_mem;
    delete reg_mem_wb;
}