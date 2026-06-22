# Flappy Capivara — Especificação do Jogo Completo

**Data:** 2026-06-21
**Disciplina:** Computação Gráfica — UFPI (Prof. Dr. Laurindo de Sousa Britto Neto)
**Tema:** Flappy Bird em 3D, com a capivara no lugar do pássaro.

---

## 1. Objetivo e escopo

Transformar a base atual (janela GLUT, câmera 3D, capivara texturizada, asas) em um
**jogo completo e jogável**, mantendo o código **simples e explicável linha a linha**
(requisito da dupla para a apresentação). Não inventar mecânicas além do necessário.

O jogo é **3D na renderização** com **jogabilidade em 2D** no plano XY, conforme o
requisito mínimo da disciplina.

---

## 2. Mapeamento dos requisitos da disciplina

| Requisito (instrucoes.md) | Como é atendido |
|---|---|
| Jogo 3D, jogabilidade ≥ 2D em GLUT | Render 3D, gameplay no plano XY (já temos a base) |
| Detecção de colisões | **Duas técnicas**: AABB (capivara × canos/chão) e esfera-esfera (capivara × inimigo) |
| Algoritmo de IA | **FSM reativa** do inimigo: `VAGANDO` ↔ `PERSEGUINDO` (exemplo da apostila 6.4) |
| Iluminação | `GL_LIGHT0` já ativo |
| Sombreamento | Normais suaves (`GenSmoothNormals`), `GL_SMOOTH` |
| Textura | Capivara texturizada via PNG/stb_image (já temos) |
| Visibilidade | Z-buffer (já) + back-face culling (`GL_CULL_FACE`) |
| Bibliotecas complementares | Assimp (modelo .obj) + stb_image (textura). **Sem áudio** (decisão da dupla) |

---

## 3. Decisões de design (já acordadas)

- **Áudio:** nenhum. Mantém o build limpo; áudio é apenas bônus na apostila.
- **IA:** um inimigo que **mata no contato** (game over), integrando a IA à jogabilidade.
- **Asas:** manter o modelo `wings.obj`, **corrigindo** o bater (dividir em metade
  esquerda/direita girando em sentidos opostos). A batida é **disparada no ato do pulo**
  (animação one-shot), não contínua.
- **Canos:** feitos de **primitivas** (`glutSolidCube` escalado), conforme a apostila
  sugere para obstáculos. Demonstra uma técnica de modelagem diferente da capivara (.obj).
- **Arquitetura:** **um único `main.cpp`** organizado em seções com cabeçalhos claros.

---

## 4. Arquitetura

Arquivo único `main.cpp`, dividido em seções:

1. Includes e constantes
2. Estado global (struct do jogo, capivara, canos, inimigo)
3. Carregamento de modelos/texturas (já existe)
4. Física e controle
5. Canos (atualização + desenho)
6. Colisão (AABB + esfera)
7. IA do inimigo (FSM)
8. Asas (split + batida)
9. Renderização (display) e HUD
10. Callbacks GLUT (display, reshape, teclado, mouse, idle) e `main`

Funções "puras" isoladas para serem fáceis de explicar e conferir:
`passoFisica`, `sobreposicaoAABB`, `distanciaEsferas`, `atualizarInimigo`.

---

## 5. Estado global

```c
enum EstadoJogo { INICIO, JOGANDO, GAMEOVER };
EstadoJogo estado = INICIO;

float capivaraY;          // posição vertical (X é fixo)
float velocidadeY;        // velocidade vertical (gravidade/pulo)
float tempoDoUltimoPulo;  // instante do último pulo (para a batida de asa)
int   pontuacao;

struct Cano { float x; float centroBrecha; bool contado; };
Cano canos[NUM_CANOS];    // ~4 pares reutilizados

enum EstadoIA { VAGANDO, PERSEGUINDO };
struct Inimigo {
    float x, y;
    float vx, vy;         // direção atual (no estado VAGANDO)
    EstadoIA estadoIA;
    float tempoProxSorteio; // quando re-sortear a direção randômica
};
Inimigo inimigo;
```

---

## 6. Subsistemas

### 6.1 Estados do jogo (FSM principal)

- `INICIO` — desenha a cena parada + texto "ESPAÇO para começar".
- `JOGANDO` — física rodando; pular, canos, IA, colisões, placar.
- `GAMEOVER` — cena congelada + "Pontuação: N" + "ESPAÇO para reiniciar".

Transições: **ESPAÇO** ou **clique esquerdo**.
`INICIO → JOGANDO` (reseta o jogo e dá o primeiro pulo),
`JOGANDO → GAMEOVER` (em qualquer colisão),
`GAMEOVER → INICIO` (reinicia o estado e volta à tela inicial) **ou** direto para
`JOGANDO` — escolher o mais simples na implementação (preferência: voltar para `JOGANDO`
já reiniciado, evitando uma tela a mais).

### 6.2 Física e controle

No `idle`, com `dt` real (diferença de `GLUT_ELAPSED_TIME` entre quadros), apenas quando
`estado == JOGANDO`:

```
velocidadeY += GRAVIDADE * dt;     // GRAVIDADE < 0 (puxa para baixo)
capivaraY   += velocidadeY * dt;
```

Pular (input):
```
velocidadeY      = IMPULSO_PULO;   // velocidade para cima instantânea
tempoDoUltimoPulo = agora;         // dispara a batida de asa (ver 6.6)
```

Polimento barato: inclinar a capivara conforme `velocidadeY` (nariz para cima ao subir,
para baixo ao cair), com clamp. ~3 linhas no desenho.

### 6.3 Canos (obstáculos)

Vetor de `NUM_CANOS` (~4) pares **reutilizados**. Cada par tem `x` e `centroBrecha`
(altura do centro da abertura). A altura da brecha é uma constante `ALTURA_BRECHA`.

A cada quadro (JOGANDO):
- `cano.x -= VELOCIDADE_CANO * dt;`
- Se `cano.x < LIMITE_ESQUERDO`: recicla para a direita
  (`cano.x += NUM_CANOS * ESPACO_ENTRE_CANOS`) e sorteia novo `centroBrecha`.
- Pontuação: quando `cano.x` cruza o X da capivara e `!cano.contado`, `pontuacao++` e
  `cano.contado = true` (reseta ao reciclar).

Desenho: cada par = dois `glutSolidCube` escalados (alto e estreito), verdes, um acima e
um abaixo da brecha. Opcional: uma "boca" (cubo levemente mais largo) na ponta de cada
cano, junto da brecha, para o visual clássico de Flappy.

### 6.4 Detecção de colisão (duas técnicas)

**AABB — capivara × canos e chão:**
A capivara é aproximada por uma caixa (a partir do seu bounding box). Cada cano é uma
caixa. Há colisão se as caixas se sobrepõem em **X e Y** simultaneamente.
Chão: `capivaraY - meiaAlturaCapivara <= 0`. (Teto: clamp, não mata — ver abaixo.)

```
bool sobreposicaoAABB(ax0,ax1,ay0,ay1, bx0,bx1,by0,by1):
    return ax0 <= bx1 && ax1 >= bx0 && ay0 <= by1 && ay1 >= by0;
```

**Esfera-esfera — capivara × inimigo** (fórmula da apostila 6.3):
```
D = sqrt((x1-x0)^2 + (y1-y0)^2 + (z1-z0)^2)   // z=0 no plano de jogo
colisão  ⇔  D <= raioCapivara + raioInimigo
```

Qualquer colisão (cano, chão ou inimigo) → `estado = GAMEOVER`.
**Teto:** apenas limita (`capivaraY` não passa de `ALTURA_TETO`), não mata — comportamento
clássico do Flappy.

### 6.5 IA do inimigo — FSM reativa (apostila 6.4)

Um inimigo (uma **abelha/inseto** desenhada com `glutSolidSphere` amarela — primitiva).
Dois estados, avaliados a cada quadro pela **mesma fórmula de distância de esfera**:

- **`VAGANDO`** — a cada `INTERVALO_SORTEIO` segundos, sorteia uma nova direção
  aleatória (`vx`, `vy`) e deriva por ela; ao tocar nos limites da área de jogo, inverte a
  direção. Movimento lento.
- **`PERSEGUINDO`** — calcula o vetor (capivara − inimigo), normaliza e move o inimigo nessa
  direção com `VELOCIDADE_PERSEGUICAO`.

Transição (reativa):
```
D = distanciaEsferas(inimigo, capivara)
if (D <= RAIO_PERCEPCAO)  estadoIA = PERSEGUINDO;
else                      estadoIA = VAGANDO;
```

Encostar na capivara (esfera-esfera, seção 6.4) → game over. O inimigo permanece na área de
jogo (não rola com os canos), funcionando como ameaça persistente.

### 6.6 Asas — correção geométrica + batida no pulo

**Correção do "gangorra":** ao carregar `wings.obj`, separar os triângulos em
**metade esquerda (x ≤ centro)** e **metade direita (x > centro)**. No desenho, girar cada
metade em **sentidos opostos** em torno do eixo da raiz (eixo Z do modelo), pelo mesmo
ângulo — assim as duas pontas sobem e descem **juntas** (bater simétrico).

**Batida disparada pelo pulo (one-shot):** o ângulo de batida depende do tempo desde o
último pulo:
```
t = agora - tempoDoUltimoPulo
if (t < DURACAO_BATIDA):
    progresso  = t / DURACAO_BATIDA           // 0 → 1
    anguloAsa  = sin(progresso * PI) * AMPLITUDE_BATIDA   // um arco: sobe e volta
else:
    anguloAsa  = 0                            // repouso
```

Depois posiciona o par de asas nas costas da capivara (translação/rotação como já feito).

### 6.7 Renderização e recursos OpenGL

- Mantém iluminação (`GL_LIGHT0`), sombreamento suave, textura da capivara, z-buffer.
- Adiciona **back-face culling** (`GL_CULL_FACE`) como recurso extra de visibilidade.
- Ordem do `display` (JOGANDO): câmera → chão → canos → capivara (+ asas, + inclinação) →
  inimigo → HUD.

### 6.8 HUD (placar e mensagens)

Texto 2D com `glutBitmapCharacter`, dentro de uma projeção **ortográfica** sobreposta
(salvar/empilhar projeção e modelview, desligar iluminação, desenhar, restaurar):
- `JOGANDO`: pontuação no topo.
- `INICIO`: título + "ESPAÇO/clique para começar".
- `GAMEOVER`: "Pontuação: N" + "ESPAÇO/clique para reiniciar".

---

## 7. Fluxo de dados (loop principal)

```
idle():
    dt = (agora - ultimoTempo); ultimoTempo = agora
    se estado == JOGANDO:
        passoFisica(dt)            // gravidade + posição
        atualizarCanos(dt)         // mover, reciclar, pontuar
        atualizarInimigo(dt)       // FSM: vagar/perseguir
        verificarColisoes()        // AABB + esfera → talvez GAMEOVER
    glutPostRedisplay()

display():
    desenha cena conforme o estado + HUD

teclado/mouse (ESPAÇO / clique):
    INICIO   → reinicia e vai para JOGANDO (+ primeiro pulo)
    JOGANDO  → pular (velocidadeY = IMPULSO; tempoDoUltimoPulo = agora)
    GAMEOVER → reinicia e vai para JOGANDO
```

---

## 8. Parâmetros ajustáveis (constantes no topo do arquivo)

`GRAVIDADE`, `IMPULSO_PULO`, `VELOCIDADE_CANO`, `ESPACO_ENTRE_CANOS`, `ALTURA_BRECHA`,
`NUM_CANOS`, `ALTURA_TETO`, `RAIO_CAPIVARA`, `RAIO_INIMIGO`, `RAIO_PERCEPCAO`,
`VELOCIDADE_PERSEGUICAO`, `INTERVALO_SORTEIO`, `DURACAO_BATIDA`, `AMPLITUDE_BATIDA`.

Valores definidos por tentativa durante a implementação; ficam agrupados e comentados para
facilitar ajuste fino e explicação.

---

## 9. Verificação

Jogo GLUT interativo → validação **manual rodando o app** (`./build.sh`):
- A capivara cai por gravidade e sobe ao pular; asas batem **no pulo**.
- Canos rolam, reciclam e a pontuação incrementa ao passar.
- Colidir com cano/chão/inimigo leva a GAMEOVER; reiniciar funciona.
- O inimigo **vaga** quando longe e **persegue** quando perto (mudança de estado visível).
- HUD mostra placar e mensagens nas telas certas.

As funções puras (`passoFisica`, `sobreposicaoAABB`, `distanciaEsferas`) são pequenas e
auto-contidas, fáceis de conferir manualmente.

---

## 10. Fora de escopo (YAGNI)

- Áudio (música/efeitos).
- Múltiplos inimigos, power-ups, níveis, dificuldade progressiva.
- Menu gráfico, ranking/persistência de pontuação.
- Cenário elaborado (nuvens, parallax, árvores) além do chão e céu atuais.
- Modelos .obj para canos ou inimigo (são primitivas, por decisão).
