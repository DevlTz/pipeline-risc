#include "unidade_controle.h"

/**
 * PROCESSO DE DECODIFICAÇÃO
 * Responsável por transformar a instrução binária de 16 bits em sinais 
 * que a Parte Operativa consiga entender.
 */
void UNIDADE_CONTROLE::decodificar() {
    sc_uint<16> inst = instrucao.read();
    
    // Fatiamento da Instrução (Conforme definido na ISA do trabalho)
    // [15:12] Opcode | [11:8] Rd | [7:4] Rs1 | [3:0] Rs2/Imediato
    sc_uint<4> op = (inst >> 12) & 0xF; 
    sc_uint<4> rd = (inst >> 8) & 0xF;  
    sc_uint<4> rs1 = (inst >> 4) & 0xF; 
    sc_uint<4> rs2 = inst & 0xF;        

    // Extensão de Sinal: Converte o imediato de 4 bits para 16 bits
    // Mantém o sinal negativo se o bit mais significativo do campo rs2 for 1
    sc_int<16> imm;
    if (rs2 & 0x8) imm = 0xFFF0 | rs2; 
    else imm = rs2;

    // Atualiza as saídas básicas para os registradores
    opcode.write(op);
    reg_destino.write(rd);
    reg_fonte1.write(rs1);
    reg_fonte2.write(rs2);
    imediato.write(imm);
    
    // Inicialização de Sinais de Controle (Estado Padrão = Desligado)
    bool is_branch = false; bool is_jump = false;
    bool do_mem_read = false; bool do_mem_write = false;
    bool do_reg_write = false; bool use_imm = false;
    bool use_mem_data = false;
    sc_uint<4> alu_operation = op;

    // LOGICA DA UNIDADE DE CONTROLE
    switch (op.to_uint()) {
        // Instruções Aritméticas e Lógicas (Tipo R)
        case 0x0: case 0x1: case 0x2: case 0x3: case 0x4: case 0x5:
            do_reg_write = true;  // Habilita escrita no banco de registradores
            use_imm = false;      // Usa valor do registrador Rs2, não o imediato
            break;
            
        case 0x6: // CMP: Apenas subtrai para setar flags, não salva resultado
            do_reg_write = false;
            use_imm = false;
            break;
        
        case 0x7: // LD: Lê da memória para o registrador
            do_reg_write = true;
            do_mem_read = true;   // Ativa barramento de leitura da RAM
            use_imm = true;       // Endereço = Rs1 + Imediato
            use_mem_data = true;  // Mux seleciona dado da memória para o Write-Back
            alu_operation = 0x4;  // ULA faz ADD para calcular o endereço
            break;
        
        case 0x8: // ST: Escreve do registrador na memória
            do_mem_write = true;  // Ativa escrita na RAM
            use_imm = true;
            alu_operation = 0x4;  // ULA faz ADD para calcular o endereço
            break;
        
        case 0x9: // J: Salto Incondicional
            is_jump = true;
            use_imm = true;
            break;
        
        case 0xA: // JN: Desvia se a flag de Negativo da ULA for 1
            if (flag_neg.read()) is_branch = true;
            use_imm = true;
            break;
            
        case 0xB: // JZ: Desvia se a flag de Zero da ULA for 1
            if (flag_zero.read()) is_branch = true;
            use_imm = true;
            break;
    }

    // Envia os sinais de controle para os estágios EX, MEM e WB
    alu_op.write(alu_operation);
    alu_src.write(use_imm);
    mem_read.write(do_mem_read);
    mem_write.write(do_mem_write);
    reg_write.write(do_reg_write);
    mem_to_reg.write(use_mem_data);
    branch.write(is_branch);
    jump.write(is_jump);
}

/**
 * UNIDADE DE DETECÇÃO DE HAZARDS
 * Previne erros quando uma instrução precisa de um dado que ainda não foi escrito.
 */
void HAZARD_DETECTION::detectar() {
    bool precisa_stall = false;
    
    // Load-Use Hazard: Se a instrução anterior foi um LD e a atual usa o mesmo reg.
    // O pipeline precisa parar (Stall) pois o dado só chega da memória no fim do ciclo
    if (ex_mem_read.read()) {
        sc_uint<4> rd = ex_rd.read();
        sc_uint<4> rs1 = id_rs1.read();
        sc_uint<4> rs2 = id_rs2.read();
        
        if (rd != 0 && (rd == rs1 || rd == rs2)) {
            precisa_stall = true; // Gera a "bolha" no pipeline
        }
    }
    
    stall.write(precisa_stall);
    flush_if_id.write(precisa_stall);
}