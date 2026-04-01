# Elifoot II — Sega Genesis / Mega Drive Port

Port do jogo Elifoot II (MS-DOS, 1994, Turbo Pascal) para o Sega Genesis / Mega Drive
usando SGDK 1.70.

## Requisitos

- **SGDK 1.70** instalado (https://github.com/Stephane-D/SGDK)
- **Python 3.6+** com Pillow opcional (`pip install pillow`)
- Ficheiros originais `EQUIPAS.EF2` e `TREINAD.EF2` em `original/`

## Build

```bash
# 1. Gerar fonte bitmap (109 glifos 8×8px)
python3 tools/gen_font.py gfx/font_cp850.png

# 2. Converter dados originais para binários de ROM
python3 tools/pack_data.py original/EQUIPAS.EF2 original/TREINAD.EF2 data/

# 3. Compilar ROM
make SGDK=/caminho/para/sgdk170
# Saída: out/rom.bin
```

## Estrutura

```
elifoot2_genesis/
├── makefile                    ← invoca makefile.gen do SGDK
├── src/
│   ├── main.c                  ← entry point, init, loop principal
│   ├── engine/
│   │   ├── compat.c/h          ← memmove, strncmp, atol (ausentes em libmd.a)
│   │   ├── input.c/h           ← JOY_readJoypad, edge/hold/repeat detection
│   │   ├── render.c/h          ← VDP text engine, paletas CGA→Genesis
│   │   ├── rng.c/h             ← LCG 32-bit (parâmetros Turbo Pascal)
│   │   └── sram_io.c/h         ← save/load 3 slots com CRC-16/CCITT
│   ├── game/
│   │   ├── types.h             ← Player, Team, CupTie, SeasonRecord, ...
│   │   ├── data.c/h            ← leitura de ROM → RAM
│   │   ├── match.c/h           ← simulação de partidas
│   │   ├── league.c/h          ← round-robin de Berger
│   │   ├── cup.c/h             ← copa eliminatória em 2 mãos
│   │   ├── economy.c/h         ← salários, bilheteria, prémios
│   │   ├── transfer.c/h        ← leilão por salário
│   │   └── season.c/h          ← coordenação de temporada
│   └── screens/
│       ├── title.c/h           ← tela de título + seleção de equipa
│       ├── main_menu.c/h       ← menu principal pré-jornada
│       ├── squad.c/h           ← plantel, formação, auto-escalar
│       ├── finances.c/h        ← ordenados, receitas, estádio
│       ├── transfers.c/h       ← venda e leilão
│       ├── palmares.c/h        ← historial de temporadas
│       └── coaches.c/h         ← treinadores, chicotada psicológica
├── res/
│   ├── resources.res           ← declarações rescomp (TILESET + BIN)
│   └── resources.h             ← gerado pelo rescomp (placeholder incluído)
├── gfx/
│   └── font_cp850.png          ← fonte 8×872px, indexada 2 cores
├── data/
│   ├── teams.bin               ← 29 equipas + 464 jogadores (11894 bytes)
│   └── coaches.bin             ← 50 treinadores (1002 bytes)
├── tools/
│   ├── gen_font.py             ← gera font_cp850.png (sem deps. externas)
│   └── pack_data.py            ← converte EQUIPAS.EF2/TREINAD.EF2 → .bin
└── original/
    ├── EQUIPAS.EF2             ← dados originais das equipas
    └── TREINAD.EF2             ← dados originais dos treinadores
```

## Mapeamento de Controlos

| Botão | Acção |
|---|---|
| A | Confirmar / Seleccionar |
| B | Cancelar / Voltar |
| C | Acção secundária (auto-escalar, gravar) |
| X | Transferências |
| Y | Palmares / Mudar formação |
| Z | Treinadores |
| Start | Gravar / Carregar |
| D-pad | Navegar menus e listas |

## Funcionalidades

- 29 equipas e 464 jogadores dos dados originais
- Campeonato em 2 divisões com sistema round-robin de Berger
- Copa em eliminatórias de 2 mãos (5 fases + penáltis)
- Sistema económico: salários, bilheteria, prémios, transferências
- Treinadores com chicotada psicológica (+5 força, 3 jornadas)
- SRAM: 3 slots de save com CRC-16
- Promoção/rebaixamento de 3 equipas por divisão
- Tela de teste de paleta CGA→Genesis (botão C no título)

## Caveats SGDK 1.70

- `int` = 16 bits no m68k — todos os valores monetários usam `long`
- `JOY_init()` é a primeira chamada obrigatória em `main()`
- `SYS_doVBlankProcess()` apenas no loop principal, nunca em simulação
- `VDP_clearPlane(WINDOW, TRUE)` obrigatório em cada troca de tela
- `SRAM_enable()` / `SRAM_disable()` sempre em par
