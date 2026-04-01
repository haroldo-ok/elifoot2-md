# res/resources.res - Recursos da ROM do Elifoot II Genesis
#
# Processado pelo rescomp 3.39b (SGDK 1.70).
#
# ATENCAO: caminhos sao relativos a localizacao DESTE ficheiro (res/).
# Por isso os assets usam ../ para subir um nivel.
#
# PRE-REQUISITOS (executar a partir da raiz do projecto):
#   python3 tools/gen_font.py gfx/font_cp850.png
#   python3 tools/pack_data.py original/EQUIPAS.EF2 original/TREINAD.EF2 data/

# ---------------------------------------------------------------------
# Fonte de texto (TILESET)
# Caminho relativo a res/ -> sobe para a raiz com ../
# ---------------------------------------------------------------------
# TILESET font_tiles removed -- font loaded directly as C array in font_data.c

# ---------------------------------------------------------------------
# Dados binarios (BIN)
# teams.bin   : 11894 bytes (29 equipas x 464 jogadores)
# coaches.bin : 1002 bytes  (50 treinadores)
# ---------------------------------------------------------------------
BIN teams_data   "../data/teams.bin"   2 2 0 FAST
BIN coaches_data "../data/coaches.bin" 2 2 0 FAST
