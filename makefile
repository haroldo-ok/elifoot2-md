# makefile — Elifoot II Genesis
#
# Uso:
#   make          → build completo
#   make data     → apenas regenera teams.bin e coaches.bin
#   make clean    → limpa out/ e ficheiros gerados pelo rescomp
#   make fullclean→ clean + remove data/*.bin
#
# Pré-requisito: definir variável SGDK apontando para o diretório
# de instalação do SGDK 1.70, ou exportar no ambiente:
#   export SGDK=/opt/sgdk170
#
# O makefile.gen do SGDK compila automaticamente todos os .c em src/
# (incluindo subdirectórios engine/, game/, screens/) e linka com
# libmd.a. Output: out/rom.bin
#
# -DSGDK_GCC é definido automaticamente pelo makefile.gen — usar
# em guards #ifndef SGDK_GCC apenas para declarações que conflitem
# com genesis.h (ex: forward-declares de strlen com tipos errados).
# As funções em compat.c NÃO usam esse guard.

# Caminho para o SGDK 1.70 (ajustar conforme instalação local)
ifndef SGDK
    $(error Variável SGDK não definida. Ex: make SGDK=/opt/sgdk170)
endif

# Pré-processamento de dados: gera teams.bin e coaches.bin
ORIGINAL_DIR := original
DATA_DIR     := data
TOOLS_DIR    := tools
PYTHON       := python3

DATA_BIN := $(DATA_DIR)/teams.bin $(DATA_DIR)/coaches.bin

.PHONY: all data clean fullclean

# Alvo padrão: garante dados gerados e invoca makefile.gen do SGDK
all: data
	$(MAKE) -f $(SGDK)/makefile.gen

# Gera binários de dados a partir dos arquivos EF2 originais
data: $(DATA_BIN)

$(DATA_BIN): $(ORIGINAL_DIR)/EQUIPAS.EF2 $(ORIGINAL_DIR)/TREINAD.EF2 \
             $(TOOLS_DIR)/pack_data.py
	@echo "Gerando dados de ROM..."
	$(PYTHON) $(TOOLS_DIR)/pack_data.py \
	    $(ORIGINAL_DIR)/EQUIPAS.EF2 \
	    $(ORIGINAL_DIR)/TREINAD.EF2 \
	    $(DATA_DIR)

clean:
	$(MAKE) -f $(SGDK)/makefile.gen clean
	@rm -f res/resources.h res/resources.s

fullclean: clean
	@rm -f $(DATA_BIN)
	@echo "Limpeza completa concluída."
