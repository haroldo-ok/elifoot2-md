# Elifoot II — comportamento observado no executável original

Observado a correr `elifoot.exe` (documentation/elifoot2.zip) no DOSBox, em
novembro de 2026. Vale mais do que `elifoot2_analise.md` onde divergirem.
Os pontos marcados **(inferido)** não foram vistos directamente.

## Dados

- `EQUIPAS.EF2` tem **36 equipas**; o jogo usa as **32 primeiras**
  (a análise dizia 29).
- Os dois números depois do nome **não** são pote/orçamento: são as **cores do
  clube** em CGA. N1 = cor do texto, N2 = cor do fundo (só 0–7 no fundo;
  14 aparece castanho, 15 cinzento). Ex.: Flamengo 0/4 preto sobre vermelho,
  Palmeiras 2/15 verde sobre cinzento, AMERICANO 15/4 branco sobre vermelho.
  Os nomes dos clubes aparecem sempre nas suas cores.
- 4 divisões de 8 equipas pela ordem do ficheiro (1–8 = 1ª divisão, …).
- Força dos jogadores depende da divisão: 4ª ≈ 10–15, 1ª/2ª ≈ 29–38.
  Ordenados ≈ força × 100 (4ª: 900–1800).
- Todas as equipas começam com 320000 de dinheiro (visto na 4ª divisão)
  e preço dos bilhetes 10. A equipa da 4ª tinha 30000 lugares e 13250 sócios.

## Início

1. Ecrã gráfico "ELIFOOT II", depois `Quer continuar um jogo?`.
2. Lista `NOME / EQUIPA`: escreve-se o nome de cada treinador humano
   (vários jogadores em hot-seat); cada um recebe uma equipa ao acaso
   da 4ª divisão. Linha vazia termina.
3. `SORTEIO DOS JOGOS DA TACA`: as 32 equipas em 4 colunas de 8; os pares são
   sorteados um a um (animado) para a caixa de cima.

## Ecrã principal (um por treinador humano)

- Esquerda: plantel agrupado por posição (G/D/M/A), com nome, força,
  ordenado, golos; `*` ao lado de jogador que teve aumento.
- Direita em cima: clube, treinador, `Nº lugar  N pontos`,
  `N lugares  N sócios`.
- Direita ao meio: painel de ajuda que alterna TACTICAS (F1–F10),
  CAMPEONATO (Shift+F1–F9), FINANÇAS (Ctrl+F1–F5), DIVERSOS (Alt+F1–F10).
- Em baixo à esquerda: próximo adversário nas cores dele, `CASA/FORA`,
  divisão (se for de outra), lugar e pontos, `Nª JORNADA` ou
  `1ª ELIMINATORIA (1ª MAO)`.
- Em baixo à direita: `Dinheiro`, `Ordenados`, `Preço dos bilhetes`.
- Para jogar escolhe-se uma táctica F1–F10: o jogo escala os melhores por
  posição, mostra a ficha 1–11 e os SUPLENTES 12–16; Enter joga.

## Jornada

- Antes do jogo: leilões `VENDA PELA MELHOR OFERTA DE ORDENADO` — um jogador
  de outro clube (força, posição, equipa, PREÇO, ORDENADO MÍNIMO) é posto em
  leilão; cada treinador humano escreve uma oferta de ordenado; mostra
  `TRANSFERIDO PARA O X / NOVO ORDENADO`. Vários por jornada.
  Preço ≈ força × 3000; ordenado mínimo ≈ força × 70.
- Pedidos de aumento: `X, Avançado de força 15, pede que lhe seja aumentado o
  ordenado para 1600. Caso não aceite o jogador pode decidir pôr-se em leilão
  pelo preço de 18000. Aceita o aumento (S/N)?`
- Jogo ao vivo: as 4 divisões (4 jogos cada) no mesmo ecrã, casa/fora com
  resultado, último marcador e minuto nas cores do clube dele, público à
  direita; barra de tempo; minuto em baixo.
- Ao intervalo abre, para cada humano, `JOGADORES EM CAMPO / JOGADORES NO
  BANCO`, `F1 Substituir`, `Esc Fim`.
- Depois: `CLASSIFICAÇÃO Nª JORNADA` com as 4 divisões no mesmo ecrã
  (V E D, golos M:S, P). **Vitória = 2 pontos.**
- Ordenados são pagos **todas as jornadas** (320000 → 297300 com 22700 de
  ordenados). Receita em casa = público × preço do bilhete (15183 × 10).
- A força de cada jogador varia de jornada para jornada (forma), ±1–4.
- Sócios variam ligeiramente a cada jornada.
- Capacidade do estádio pode cair (30000 → 25000): as bancadas degradam-se.

## Taça

- 32 equipas, 1ª eliminatória, 2ª eliminatória, quartos, meias, final, cada
  fase em 2 mãos; jogos da taça são "jornadas" próprias intercaladas com o
  campeonato (a 1ª mão veio depois da 2ª jornada).
- Ecrã de jogo da taça: as 16 eliminatórias ao vivo, `1ª ELIMINATORIA (1ª MAO)`.
- Strings: PROLONGAMENTO, `Nº PENALTY (CASA)/(FORA) EXECUTADO`
  (o `º` é ordinal, não uma borda), EQUIPAS APURADAS (com a divisão),
  EQUIPAS ELIMINADAS, FINAL DA TACA, VENCEDOR DA TACA.

## Menus vistos

- Shift+F1 Calendário: 14 jornadas da equipa e do próximo adversário
  (adversário, C/F, resultado).
- Shift+F2 Resultados: tabela cruzada 8×8 da divisão.
- Shift+F3 Palmarés: equipa e próximo adversário (`4ª DIVISAO: 1 época`).
- Shift+F4 Melhores marcadores: nome, clube, golos, nas cores do clube.
- Shift+F6 Classificação da divisão com CASA / FORA / TOTAL (J V E D G, P).
- Shift+F7 Próximas jornadas: próximas 5 jornadas, jogos da equipa e do
  adversário seguinte.
- Ctrl+F2 Alterar o ordenado: escolhe o jogador na lista do plantel.
- Ctrl+F3 Prémios: vencedor do campeonato 1000000, 2º 500000, 3º 300000,
  vencedor da taça 500000, melhor ataque 200000, melhor defesa 200000,
  melhor marcador 200000.
- Alt+F2 Estádio: gráfico de barras das bancadas (1..N) e
  `Deseja construir uma bancada (preço: 100000) ?` — cada bancada = 5000
  lugares; a nova barra é a mais comprida (vida útil **(inferido)**).
- Alt+F4 `TREINADORES EM JOGO`: lista dos **treinadores humanos**;
  F1 Anular treinador, F2 Novo treinador (pede Nº e Nome). Não é sobre os
  treinadores da IA.
- Ctrl+F1/F4/F5 não foram testados (são atalhos do DOSBox); seguem as strings.

## Inferências usadas no port

- Ordem jornadas/taça: L1 L2 T1 L3 T2 … L11 T10 L12 L13 L14 (24 jornadas).
- Promoção/despromoção: 2 equipas por divisão.
- Prolongamento de 30 min e penáltis na 2ª mão se o agregado empatar
  (sem regra de golos fora).
- Proposta "X: quer ir treinar o Y? (s/n)" é feita aos treinadores humanos.
- Chicotadas psicológicas: clubes da IA despedem treinadores de TREINAD.EF2.
- Vida das bancadas, evolução dos sócios e fórmula do público.
