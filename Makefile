# ==============================================================================
# Makefile — Processador RISC Pipelined em SystemC
# DIM0129 — Organização de Computadores · UFRN · 2026.1
# ==============================================================================
# Uso:
#   make tb_ula                  Compila e executa o testbench da ULA
#   make tb_banco_reg            Compila e executa o testbench do Banco de Registradores
#   make tb_controle             Compila e executa o testbench da Unidade de Controle
#   make tb_processador          Compila e executa o testbench de integração completo
#   make sim_soma                Compila e executa a simulação: Soma de Vetor
#   make sim_fibonacci           Compila e executa a simulação: Fibonacci
#   make sim_bubble_sort         Compila e executa a simulação: Bubble Sort
#   make all                     Compila todos os alvos (sem executar)
#   make clean                   Remove binários gerados
# ==============================================================================

# --- Configuração do SystemC ---------------------------------------------------
# Sobrescreva via linha de comando se necessário:
#   make tb_ula SYSTEMC_HOME=/caminho/para/systemc
SYSTEMC_HOME ?= /usr/local/systemc

# --- Compilador e flags --------------------------------------------------------
CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra \
            -I$(SYSTEMC_HOME)/include \
            -I./src \
            -I./src/po \
            -I./src/pc \
            -I./src/top
LDFLAGS  := -L$(SYSTEMC_HOME)/lib-linux64 \
            -lsystemc \
            -Wl,-rpath,$(SYSTEMC_HOME)/lib-linux64

# --- Diretórios ---------------------------------------------------------------
BIN_DIR := bin
SRC_PO  := src/po
SRC_PC  := src/pc
SRC_TOP := src/top
TB_DIR  := testbench
SIM_DIR := sim

# --- Fontes compartilhadas (compiladas junto com cada alvo) -------------------
SRCS_COMMON := $(SRC_PO)/ula.cpp \
               $(SRC_PO)/registradores.cpp \
               $(SRC_PC)/unidade_controle.cpp \
               $(SRC_TOP)/processador.cpp

# ==============================================================================
# Alvos principais
# ==============================================================================

.PHONY: all tb_ula tb_banco_reg tb_controle tb_processador \
        sim_soma sim_fibonacci sim_bubble_sort clean help

all: $(BIN_DIR)/tb_ula \
     $(BIN_DIR)/tb_banco_reg \
     $(BIN_DIR)/tb_controle \
     $(BIN_DIR)/tb_processador \
     $(BIN_DIR)/sim_soma \
     $(BIN_DIR)/sim_fibonacci \
     $(BIN_DIR)/sim_bubble_sort

# --- Cria o diretório de saída se necessário ----------------------------------
$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

# ==============================================================================
# Testbenches unitários
# ==============================================================================

# --- ULA ----------------------------------------------------------------------
$(BIN_DIR)/tb_ula: $(TB_DIR)/tb_ula.cpp $(SRC_PO)/ula.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

tb_ula: $(BIN_DIR)/tb_ula
	@echo ""
	@echo ">>> Executando: Testbench da ULA <<<"
	@echo "------------------------------------------------------------"
	@./$(BIN_DIR)/tb_ula

# --- Banco de Registradores ---------------------------------------------------
$(BIN_DIR)/tb_banco_reg: $(TB_DIR)/tb_banco_registradores.cpp $(SRC_PO)/registradores.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

tb_banco_reg: $(BIN_DIR)/tb_banco_reg
	@echo ""
	@echo ">>> Executando: Testbench do Banco de Registradores <<<"
	@echo "------------------------------------------------------------"
	@./$(BIN_DIR)/tb_banco_reg

# --- Unidade de Controle ------------------------------------------------------
$(BIN_DIR)/tb_controle: $(TB_DIR)/tb_unidade_controle.cpp $(SRC_PC)/unidade_controle.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

tb_controle: $(BIN_DIR)/tb_controle
	@echo ""
	@echo ">>> Executando: Testbench da Unidade de Controle <<<"
	@echo "------------------------------------------------------------"
	@./$(BIN_DIR)/tb_controle

# --- Processador (integração completa) ----------------------------------------
$(BIN_DIR)/tb_processador: $(TB_DIR)/tb_processador.cpp $(SRCS_COMMON) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

tb_processador: $(BIN_DIR)/tb_processador
	@echo ""
	@echo ">>> Executando: Testbench de Integração do Processador <<<"
	@echo "------------------------------------------------------------"
	@./$(BIN_DIR)/tb_processador

# ==============================================================================
# Simulações de algoritmos
# ==============================================================================

# --- Soma de Vetor ------------------------------------------------------------
$(BIN_DIR)/sim_soma: $(SIM_DIR)/algoritmo_soma_vetor.cpp $(SRCS_COMMON) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

sim_soma: $(BIN_DIR)/sim_soma
	@echo ""
	@echo ">>> Executando: Simulação — Soma de Vetor <<<"
	@echo "------------------------------------------------------------"
	@./$(BIN_DIR)/sim_soma

# --- Fibonacci ----------------------------------------------------------------
$(BIN_DIR)/sim_fibonacci: $(SIM_DIR)/algoritmo_fibonnacci.cpp $(SRCS_COMMON) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

sim_fibonacci: $(BIN_DIR)/sim_fibonacci
	@echo ""
	@echo ">>> Executando: Simulação — Fibonacci <<<"
	@echo "------------------------------------------------------------"
	@./$(BIN_DIR)/sim_fibonacci

# --- Bubble Sort --------------------------------------------------------------
$(BIN_DIR)/sim_bubble_sort: $(SIM_DIR)/algoritmo_bubble_sort.cpp $(SRCS_COMMON) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

sim_bubble_sort: $(BIN_DIR)/sim_bubble_sort
	@echo ""
	@echo ">>> Executando: Simulação — Bubble Sort <<<"
	@echo "------------------------------------------------------------"
	@./$(BIN_DIR)/sim_bubble_sort

# ==============================================================================
# Utilitários
# ==============================================================================

clean:
	@echo "Removendo binários..."
	@rm -rf $(BIN_DIR)
	@echo "Feito."

help:
	@echo ""
	@echo "Alvos disponíveis:"
	@echo "  make tb_ula          Compila e executa o testbench da ULA"
	@echo "  make tb_banco_reg    Compila e executa o testbench do Banco de Registradores"
	@echo "  make tb_controle     Compila e executa o testbench da Unidade de Controle"
	@echo "  make tb_processador  Compila e executa o testbench de integração completo"
	@echo "  make sim_soma        Compila e executa a simulação Soma de Vetor"
	@echo "  make sim_fibonacci   Compila e executa a simulação Fibonacci"
	@echo "  make sim_bubble_sort Compila e executa a simulação Bubble Sort"
	@echo "  make all             Compila todos os alvos (sem executar)"
	@echo "  make clean           Remove todos os binários gerados"
	@echo ""
	@echo "Variável de ambiente:"
	@echo "  SYSTEMC_HOME         Caminho para a instalação do SystemC"
	@echo "                       Padrão atual: $(SYSTEMC_HOME)"
	@echo "  Exemplo: make tb_ula SYSTEMC_HOME=/opt/systemc-2.3.4"
	@echo ""
