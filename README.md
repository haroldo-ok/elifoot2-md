# Elifoot II — port para Sega Mega Drive / Genesis

Port do **Elifoot II** (MS-DOS, 1994, Turbo Pascal) para o
**Sega Mega Drive / Genesis**, escrito em C com o **SGDK 1.70**.

O objectivo é reproduzir o jogo original o mais fielmente possível: as mesmas
32 equipas (lidas dos ficheiros originais `EQUIPAS.EF2` e `TREINAD.EF2`), as mesmas
regras de campeonato e taça, a economia, os leilões, os ecrãs em modo texto com as
cores CGA de cada clube e o jogo "ao vivo" com os 16 jogos da jornada no mesmo ecrã.

> A análise de engenharia reversa antiga (`documentation/elifoot2_analise.md`) tem
> erros conhecidos. A referência usada para este port é
> `documentation/original_observado.md`, que regista o que foi **visto a correr o
> executável original** no DOSBox, e separa o que foi confirmado do que é inferido.

---

## Índice

1. [Estado do port](#estado-do-port)
2. [Compilar](#compilar)
3. [Testar](#testar)
4. [Como jogar](#como-jogar)
5. [Regras e funcionalidades](#regras-e-funcionalidades)
6. [Arquitectura do código](#arquitectura-do-código)
7. [Dados e formatos](#dados-e-formatos)
8. [Notas técnicas e armadilhas](#notas-técnicas-e-armadilhas)
9. [Limitações conhecidas](#limitações-conhecidas)

---

## Estado do port

| Item | Estado |
|---|---|
| Compila com SGDK 1.70 | Sim — `out/rom.bin`, 256 KB |
| Arranca em emulador (Genesis Plus GX) | Sim — 320×224, ecrã de título e menus |
| Época completa (campeonato + taça + fim de época) | Sim, testado em modo autoplay |
| Várias épocas seguidas | Sim (testado com build de teste) |
| Gravar / carregar em SRAM | Sim, 3 posições com nome |
| Som / música | Não (o original só tinha bips do PC speaker) |
| Testado em hardware real | Não |

---

## Compilar

### Requisitos

- **SGDK 1.70** (https://github.com/Stephane-D/SGDK, tag `v1.70`)
- **Java** (o `rescomp` do SGDK é Java)
- **Python 3.6+** (para regenerar `data/*.bin`; os `.bin` já vêm no repositório)
- Os ficheiros originais `original/EQUIPAS.EF2` e `original/TREINAD.EF2` (incluídos)

### Windows (SGDK oficial)

```bat
make SGDK=D:/caminho/para/sgdk170
```

ou, sem regenerar dados, directamente:

```bat
make -f D:\caminho\para\sgdk170\makefile.gen
```

(`compile.bat`, `env.bat` e `run.bat` são atalhos do autor; ajuste os caminhos.)

### Linux

Com MarsDev ou a imagem docker do SGDK basta:

```sh
make SGDK=/opt/sgdk170
```

Com o compilador do Ubuntu/Debian (`gcc-m68k-linux-gnu`) é preciso **reconstruir a
`libmd.a`** do SGDK com o mesmo compilador, **sem LTO** e com deslocamentos de 16 bits
por omissão (ver [a armadilha do `bra.l`](#a-armadilha-do-bral-ecrã-preto-no-arranque)):

```sh
sudo apt install gcc-m68k-linux-gnu binutils-m68k-linux-gnu default-jre
# links m68k-elf-* -> m68k-linux-gnu-* em /opt/bin, e as/ld/nm/ar/objcopy em $GDK/bin
export GDK=/opt/sgdk170 PATH=/opt/bin:$GDK/bin:$PATH

# 1) No makelib.gen e no makefile.gen, trocar a linha "release: FLAGS..." por:
#      release: FLAGS_LIB= $(DEFAULT_FLAGS_LIB) -O2 -fomit-frame-pointer
#      release: FLAGS= $(DEFAULT_FLAGS) -O2 -fomit-frame-pointer
# 2) Reconstruir a biblioteca do zero (apagar os .o antigos é obrigatório):
cd $GDK && find src res -name '*.o' -delete && find src res -name '*.d' -delete
make -f makelib.gen PREFIX=m68k-elf- \
     EXTRA_FLAGS="-fno-store-merging -Wa,--disp-size-default-16" release

# 3) Compilar o jogo com a libgcc do SGDK (68000) e sem build-id:
cd /caminho/elifoot2-md
make -f $GDK/makefile.gen GDK=$GDK EXTRA_FLAGS=-fno-store-merging \
     "LIBGCC=$GDK/lib/libgcc.a -Wl,--build-id=none"
```

As ferramentas Z80 do SGDK (`sjasm`, `bintos`) vêm só em `.exe`; compile-as a partir
de `$GDK/tools/` antes do passo 2.

### Alvos do `makefile` do projecto

| Alvo | O que faz |
|---|---|
| `make` | gera a fonte e os dados (se faltarem) e compila a ROM |
| `make data` | regenera `data/teams.bin` e `data/coaches.bin` a partir de `original/` |
| `make font` | regenera `gfx/font_cp850.png` (só referência; a fonte em uso está em `src/engine/font_data.c`) |
| `make clean` | limpa `out/` e os ficheiros gerados pelo rescomp |
| `make fullclean` | `clean` + apaga `data/*.bin` e a fonte PNG |

Saída: **`out/rom.bin`**.

> Depois de alterar um `.h` (sobretudo `src/game/types.h`) faça `make clean`: o
> `makefile.gen` do SGDK 1.70 não segue bem as dependências dos cabeçalhos.

---

## Testar

### Num emulador

Qualquer emulador de Mega Drive serve (BlastEm, Genesis Plus GX / RetroArch, Kega
Fusion, PicoDrive). A ROM declara **SRAM com bateria** (32 KB em `$200000`, endereços
ímpares), por isso as gravações persistem entre sessões.

### Build de autoplay

```sh
make -f $GDK/makefile.gen EXTRA_FLAGS="-fno-store-merging -DELF_AUTOPLAY" ...
```

Com `-DELF_AUTOPLAY` o jogo joga-se sozinho: escolhe um treinador, responde a todas
as perguntas, aceita as tácticas sugeridas e avança jornada após jornada,
época após época. Serve para testar épocas inteiras num emulador sem ecrã
(headless). Não use esta build para jogar.

### Verificações feitas

- Arranque em Genesis Plus GX (libretro, sem ecrã): frames iluminados, com várias
  cores, a mudar ao longo do tempo e sem o ecrã de excepção do SGDK.
- Autoplay de 9000 frames (~2,5 min): leilões, jornadas ao vivo, classificações,
  taça, mudança de época.
- Gravar → reiniciar → carregar dá o mesmo estado (CRC confere).
- Cabeçalho da SRAM (`ELF2`, versão 3) escrito na SRAM do emulador.
- Verificação do binário: `m68k-elf-objdump -d out/rom.out | grep -cE ':\s+6[0-9a-f]ff '`
  deve dar **0** (nenhuma instrução `bra.l`/`bsr.l` exclusiva do 68020).

---

## Como jogar

### Início

1. Ecrã de título → **novo jogo** ou **carregar** uma das 3 gravações.
2. Ecrã dos treinadores: A acrescenta um treinador humano (até 4, *hot-seat*), com o
   nome escrito no **teclado no ecrã**; B retira o último; Start começa.
3. Cada treinador recebe uma equipa ao acaso da **4ª divisão**.

### Controlos

| Botão | Ecrã principal | Listas / perguntas | Teclado no ecrã |
|---|---|---|---|
| **A** | Escolher táctica (F1–F10) e jogar | Confirmar / Sim | Escrever a letra |
| **B** | — | Voltar / Não | Apagar |
| **C** | Seleccionar os melhores | Acção secundária | Maiúsculas/minúsculas |
| **Start** | Menu (o equivalente a Shift/Ctrl/Alt+F do original) | — | Terminar |
| **D-pad** | Percorrer o plantel | Navegar; ←/→ muda de página | Mover o cursor |

- **Ficha da táctica:** A troca dois jogadores (titular ↔ suplente), Start joga.
- **Jogo ao vivo:** A acelera; C alterna entre marcadores e público.
- **Intervalo:** A escolhe quem sai e quem entra (até 3 por jogo), B termina.

### Menu (Start no ecrã principal)

| Campeonato | Finanças | Diversos |
|---|---|---|
| Calendário | Vender | Gravar |
| Resultados | Ordenados | Estádio |
| Palmarés | Prémios | Seleccionar melhores |
| Marcadores | Transferências | Treinadores |
| Últimos vencedores | Receitas | Sair |
| Classificação | | |
| Próximas jornadas | | |
| Títulos das equipas | | |
| Títulos dos treinadores | | |
| Taça | | |

### Ciclo de uma jornada

1. **Ecrã principal** de cada treinador humano (plantel, dinheiro, ordenados,
   próximo adversário, casa/fora).
2. **Leilões** — "VENDA PELA MELHOR OFERTA DE ORDENADO": jogadores livres ou postos à
   venda; cada clube oferece um ordenado e o melhor leva o jogador.
3. **Jornada ao vivo** — os 16 jogos da jornada no mesmo ecrã, minuto a minuto, com
   os golos e os marcadores; intervalo com substituições.
4. **Resultados aplicados** — pontos, golos, bilheteira, ordenados, desgaste das
   bancadas, forma dos jogadores.
5. **Classificações** (ou ecrãs da taça, nas jornadas de taça) e chicotadas
   psicológicas da IA.
6. **Pedidos de aumento** dos jogadores dos clubes humanos.

---

## Regras e funcionalidades

Tudo o que está nesta secção foi observado no executável original, salvo onde diz
*(inferido)*.

### Competições

- **32 equipas em 4 divisões de 8**, com as cores do clube (N1 = texto, N2 = fundo,
  paleta CGA de 16 cores) em todos os ecrãs.
- **Campeonato** a 2 voltas: 14 jornadas, **vitória = 2 pontos**, empate = 1.
- **Sobem e descem 2 equipas** por divisão *(inferido)*.
- **Taça** com as 32 equipas: sorteio animado; 1ª e 2ª eliminatórias, quartos, meias e
  final, **a 2 mãos**, com prolongamento e penáltis; lista de apuradas/eliminadas.
- A época tem **24 "jornadas"**: as 14 do campeonato intercaladas com as 10 mãos da taça.
- Fim de época: campeões, subidas e descidas, prémios da temporada, palmarés,
  forma dos jogadores reposta, propostas de outros clubes aos humanos.

### Jogo

- **10 tácticas** F1–F10 (3-4-3, 4-3-3, 4-4-2, 4-5-1, 5-2-3, … 6-4-0); "seleccionar
  melhores" escolhe o onze mais forte para a táctica.
- A **força** de cada jogador varia com a forma (10 níveis); ataque e defesa da
  equipa saem dos titulares e da táctica.
- Simulação **minuto a minuto** com golos, marcadores, prolongamento e penáltis.
- Até **3 substituições** por jogo.

### Economia

- Dinheiro inicial: **320 000**.
- **Ordenados** pagos todas as jornadas.
- **Bilheteira** = público × preço do bilhete (10); **sócios**.
- **Bancadas** de 5000 lugares (até 16), que se degradam com o tempo
  (48 jornadas, *inferido*); construir uma custa **100 000**.
- **Prémios da época** (ecrã Prémios, Ctrl+F3 no original): campeão 1 000 000,
  2º 500 000, 3º 300 000, vencedor da taça 500 000, melhor ataque, melhor defesa e
  melhor marcador 200 000 cada.
- Receitas separadas de campeonato e taça.
- Clubes da IA endividados vendem jogadores primeiro.

### Mercado

- **Leilões** antes de cada jornada (oferta de ordenado).
- **Vender** jogadores (vão a leilão).
- **Pedidos de aumento**; alterar o ordenado com as respostas do original
  ("Nem pensar! Exijo um mínimo de…", "Então adeus.").
- Plantel entre **14 e 22** jogadores.

### Treinadores

- 50 treinadores da IA (de `TREINAD.EF2`) e até 4 humanos.
- **Chicotadas psicológicas**: a IA despede e troca de treinador quando os
  resultados são maus; um treinador novo dá um impulso temporário à equipa.
- Os humanos recebem **propostas de outros clubes** e podem ser despedidos por
  dívidas *(inferido)*.

### Ecrãs

Classificação geral e por divisão (casa/fora/total), calendário, resultados
(tabela cruzada), próximas jornadas, melhores marcadores, palmarés, últimos
vencedores, títulos das equipas e dos treinadores, prémios, prémios da temporada,
transferências, receitas, estádio e treinadores em jogo.

### Gravações

- **3 posições** com nome (até 12 caracteres) na SRAM, 8 KB cada, com **CRC-16**.
- A gravação guarda o estado completo (equipas, jogadores, calendário, taça,
  histórico, treinadores).

---

## Arquitectura do código

O código está dividido em três camadas. A camada `game/` não faz chamadas ao VDP:
só regras e estado.

```
src/
├── main.c                 ciclo da jornada e da época (ver "Ciclo de uma jornada")
├── boot/                  rom_head.c (cabeçalho, SRAM declarada) e sega.s (arranque)
├── engine/                camada de plataforma
│   ├── ui.c/h             texto 40x28 em qualquer par de cores CGA (tiles dinâmicos),
│   │                      caixas, títulos, perguntas S/N, esperas, modo autoplay
│   ├── input.c/h          comando com repetição automática e detecção de flancos
│   ├── sram_io.c/h        gravar/carregar directamente na SRAM, CRC-16/CCITT
│   ├── rng.c/h            gerador pseudo-aleatório (semente do VCounter e do vtimer)
│   ├── font_data.c/h      fonte 8x8: 96 ASCII + caracteres de caixa + bloco
│   └── compat.c/h         utilitários de texto (sprintf reduzido, etc.)
├── game/                  regras do jogo (sem ecrã)
│   ├── types.h            constantes e estruturas (Team, Player, Coach, CupTie, ...)
│   ├── data.c/h           estado global, carga dos dados da ROM, plantéis em cache
│   ├── tactics.c/h        F1..F10, escalação, ataque/defesa
│   ├── match.c/h          jogo minuto a minuto, prolongamento, penáltis
│   ├── season.c/h         calendário de 24 jornadas, campeonato, taça, subidas/descidas
│   ├── economy.c/h        ordenados, bilheteira, sócios, bancadas, prémios
│   ├── market.c/h         leilões, transferências, vendas, pedidos de aumento
│   ├── coaches.c/h        treinadores da IA e humanos, despedimentos, propostas
│   └── history.c/h        palmarés, últimos vencedores, prémios da época
└── screens/               um ficheiro por grupo de ecrãs
    ├── start.c            título, novo jogo / carregar
    ├── managers_screen.c  número e nomes dos treinadores
    ├── kbd.c              teclado no ecrã e introdução de números
    ├── main_screen.c      ecrã principal da equipa e menu Start
    ├── tactic.c           ficha da táctica e substituições
    ├── live.c             jornada ao vivo (16 jogos)
    ├── league_screens.c   classificações, calendário, resultados, marcadores
    ├── cup_screens.c      sorteio e eliminatórias da taça
    ├── finance_screens.c  vender, ordenados, prémios, transferências, receitas, estádio
    ├── history_screens.c  palmarés, vencedores, títulos
    ├── events.c           leilões, pedidos de aumento, chicotadas, propostas
    └── screens.h

data/teams.bin             32 equipas x 16 jogadores (gerado por tools/pack_data.py)
data/coaches.bin           50 treinadores (gerado por tools/pack_data.py)
res/resources.res          inclui os dois .bin na ROM (BIN)
tools/pack_data.py         EQUIPAS.EF2 + TREINAD.EF2 -> data/*.bin
tools/gen_font.py          gera gfx/font_cp850.png (referência da fonte)
original/                  ficheiros de dados do jogo original
documentation/             notas sobre o original, plano do port, docs do SGDK 1.70
```

### Modo de texto

O ecrã é uma grelha de **40×28 caracteres**. Como o original usa qualquer par
(texto, fundo) das 16 cores CGA, `ui.c` gera em VRAM, a pedido, uma cópia de cada
glifo para cada par de cores usado (tiles dinâmicos com cache), em vez de gastar uma
paleta por cor. A **PAL0** guarda as 16 cores CGA.

---

## Dados e formatos

### `EQUIPAS.EF2` → `data/teams.bin`

`tools/pack_data.py` lê o ficheiro original (texto CP850/Latin-1, com códigos de
posição `GR/DF/MD/AV` e nacionalidade de 3 letras), descodifica os acentos byte a
byte e escreve, em big-endian:

```
u16 número de equipas (32)
u16 jogadores por equipa (16)
32 x {
    equipa (26 bytes): nome[20], cor do texto, cor do fundo, nacionalidade[3], pad
    16 x jogador (24 bytes): nome[16], posição, nacionalidade[3], pad[4]
}
```

A força, o ordenado e a divisão inicial são calculados em `data.c` ao começar um
jogo novo.

### `TREINAD.EF2` → `data/coaches.bin`

`u16` número de treinadores (50) e, para cada um, o nome (20 bytes, NUL).

### Gravação em SRAM (versão 3)

```
$0000  "ELF2"                     assinatura
$0004  u16 versão (0x0003)
$0010  posição 1 (8 KB)   \
$2010  posição 2 (8 KB)    > cada uma: u16 CRC-16/CCITT, u16 tamanho, dados
$4010  posição 3 (8 KB)   /
```

Os dados incluem o nome da gravação, a época, a jornada, os treinadores, o estado de
todas as equipas e jogadores, os resultados, a taça e o histórico. Os nomes das
equipas e dos jogadores não são gravados: vêm da ROM, e cada jogador mantém o seu
índice original. A SRAM é de 8 bits nos endereços ímpares; `sram_io.c` lê e escreve
directamente com `SRAM_readByte`/`SRAM_writeByte`, sem buffer em RAM. Uma posição
com CRC errado aparece como vazia.

---

## Notas técnicas e armadilhas

### A armadilha do `bra.l` (ecrã preto no arranque)

Com o `gcc-m68k-linux-gnu` do Ubuntu, se a `libmd.a` for compilada com LTO, o
código da biblioteca só é gerado no link final e é montado **sem** `-mcpu=68000`. O
assembler escolhe então `bra.l` (opcode `60FF`), uma instrução que **só existe no
68020**. No 68000 esse opcode é lido como `bra.s` com deslocamento `-1`: salta para
um endereço ímpar, o CPU entra num ciclo de excepções antes de configurar o VDP e
a ROM fica com o **ecrã preto** — apesar de compilar sem erros e sem avisos.
Acontecia até com o *hello world* do SGDK.

Solução: compilar a biblioteca e o jogo **sem LTO** e com
`-Wa,--disp-size-default-16`, e confirmar que não há nenhum `60FF` no binário
(comando na secção [Testar](#testar)).

### Outras

- **`-fno-store-merging`**: o GCC 13 junta duas escritas `u8` seguidas numa escrita
  de 16 bits, que pode cair num endereço ímpar (erro de endereço no 68000).
- **libgcc do SGDK**: a `-lgcc` do Debian usa instruções do 68020 em `__divsi3`;
  ligar sempre com `$GDK/lib/libgcc.a`.
- **`int` tem 32 bits** com o GCC do SGDK, mas o código usa sempre `u8`/`u16`/`long`
  explícitos; valores de dinheiro são `long`.
- Estruturas de `types.h` têm os campos `u16`/`long` em offsets pares (o 68000 não
  lê palavras em endereços ímpares).
- `JOY_init()` é a primeira chamada de `main()`; `ui_init()` força 320×224
  (não chamar `VDP_init()` outra vez).
- `SYS_doVBlankProcess()` só nos ciclos de ecrã, nunca dentro da simulação.
- Os plantéis são guardados em cache (`data_squad`, `data_total_salaries`) e
  invalidados por `data_rosters_changed()` em cada transferência ou carga; sem isso
  o jogo ficava lento ao percorrer os 512 jogadores.
- `SRAM_enable()` / `SRAM_disable()` sempre em par.

---

## Limitações conhecidas

- Algumas fórmulas do original (força exacta de cada jogo, público, valores dos
  prémios, ritmo de degradação das bancadas, número de subidas) são **aproximações**
  do que se observou; estão marcadas como *inferido* em `types.h` e em
  `documentation/original_observado.md`.
- O cabeçalho da ROM declara o fim em `$0FFFFF`, mas o ficheiro tem 256 KB
  (`$03FFFF`). A maioria dos emuladores ignora isto; em hardware real convém
  corrigir o campo em `src/boot/rom_head.c`.
- Sem som.
- Não foi testado em hardware real.

---

## Créditos

- **Elifoot II** © 1994, do seu autor original. Os ficheiros em `original/` pertencem ao
  jogo original e são usados apenas para gerar os dados do port.
- **SGDK** © Stephane Dallongeville.
- Port: haroldo-ok (https://github.com/haroldo-ok/elifoot2-md).
