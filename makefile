# makefile - Elifoot II Genesis
#
# Uso:
#   make SGDK=D:/Pessoal/Projetos/sgdk170    (Windows, path sem espacos)
#   make SGDK=/opt/sgdk170                    (Linux/Mac)
#
# Alvos:
#   make          -> build completo (gera dados + ROM)
#   make data     -> apenas regenera teams.bin e coaches.bin
#   make font     -> apenas regenera gfx/font_cp850.png
#   make clean    -> limpa out/ e ficheiros gerados pelo rescomp
#   make fullclean-> clean + remove data/*.bin e gfx/font_cp850.png
#
# SGDK deve apontar para o directorio raiz do SGDK 1.70.
# O makefile.gen compila todos os .c em src/ automaticamente.
# Output: out/rom.bin

ifndef SGDK
    $(error Variavel SGDK nao definida. Ex: make SGDK=/opt/sgdk170)
endif

PYTHON       := python3
TOOLS_DIR    := tools
ORIGINAL_DIR := original
DATA_DIR     := data
GFX_DIR      := gfx

DATA_BIN := $(DATA_DIR)/teams.bin $(DATA_DIR)/coaches.bin
FONT_PNG := $(GFX_DIR)/font_cp850.png

.PHONY: all data font clean fullclean

all: font data
	$(MAKE) -f $(SGDK)/makefile.gen

font: $(FONT_PNG)

$(FONT_PNG):
	@echo "Gerando fonte bitmap..."
	$(PYTHON) $(TOOLS_DIR)/gen_font.py $(FONT_PNG)

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
	@$(PYTHON) -c "import os; [os.remove(f) for f in ['res/resources.h','res/resources.s','res/resources.rs'] if os.path.exists(f)]"

fullclean: clean
	@$(PYTHON) -c "import os; [os.remove(f) for f in ['$(DATA_DIR)/teams.bin','$(DATA_DIR)/coaches.bin','$(FONT_PNG)'] if os.path.exists(f)]"
	@echo "Limpeza completa concluida."
