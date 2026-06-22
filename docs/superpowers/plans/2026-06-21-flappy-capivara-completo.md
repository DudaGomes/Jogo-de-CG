# Flappy Capivara — Plano de Implementação

> **Para quem for implementar:** SUB-SKILL OBRIGATÓRIA — use superpowers:subagent-driven-development (recomendado) ou superpowers:executing-plans para implementar tarefa a tarefa. Os passos usam checkbox (`- [ ]`) para acompanhamento.

**Goal:** Transformar a base atual (janela, câmera, capivara, asas) num Flappy Bird jogável e completo, em um único `main.cpp`, atendendo todos os requisitos da disciplina (colisão, IA, iluminação, sombreamento, textura, visibilidade).

**Architecture:** Tudo em `main.cpp`, organizado em seções. Loop principal no `idle()` com `dt` real; estado global em variáveis/structs simples; renderização em `display()`. Colisão em duas técnicas (AABB para canos/chão, esfera-esfera para o inimigo). IA por FSM reativa.

**Tech Stack:** C++17, OpenGL/GLUT (macOS), Assimp (modelo .obj da capivara/asas), stb_image (textura). Sem bibliotecas novas.

**Especificação:** [docs/superpowers/specs/2026-06-21-flappy-capivara-completo-design.md](../specs/2026-06-21-flappy-capivara-completo-design.md)

---

## Observações de execução

- **Branch:** o repositório está na `main`. Antes de começar, crie uma branch:
  `git checkout -b flappy-jogo-completo`.
- **Compilar e rodar:** `./build.sh` (compila e abre o jogo; feche com ESC).
- **Rodar os testes das funções puras:** depois de compilar, `./flappy_capivara --testes`
  (roda asserts e sai sem abrir janela).
- Os valores numéricos (gravidade, velocidades, raios, ângulos) são pontos de partida
  razoáveis; ajuste fino rodando o jogo. Todos ficam em constantes no topo, comentadas.

---

## Estrutura de arquivos

- **Modificar:** `main.cpp` — único arquivo de código. Todas as tarefas mexem nele.
- Sem arquivos novos de código. `wings.obj`, `Capybara.obj` e a textura já existem.

Seções do `main.cpp` ao final (ordem física no arquivo):
1. Includes
2. Constantes (janela, câmera, **parâmetros de jogo**)
3. Struct `Modelo` + carregamento (já existe)
4. **Estado global do jogo** (novo)
5. **Funções puras** (`passoFisica`, `sobreposicaoAABB`, `distanciaEsferas`) + `rodarTestes`
6. **Canos** (atualizar/desenhar)
7. **Inimigo / IA** (atualizar/desenhar)
8. Capivara + **asas** (desenhar)
9. **HUD** (texto ortográfico)
10. `display`, `reshape`, input, `idle`, `inicializarOpenGL`, `main`

---

## Task 1: Parâmetros de jogo e estado global

**Files:**
- Modify: `main.cpp` (bloco de constantes, após `CAPIVARA_X`/`CAPIVARA_Y`)

- [ ] **Step 1: Adicionar constantes de jogo**

Logo após a constante `CAPIVARA_Y` (linha ~46), adicione um bloco de parâmetros. Mantenha
`CAPIVARA_X` como está. **Remova** `const float CAPIVARA_Y` (vira variável de estado).

```cpp
// ============================================================
//  PARÂMETROS DO JOGO (ajuste fino rodando o jogo)
// ============================================================
const float GRAVIDADE        = -15.0f;  // unidades/s² (puxa para baixo)
const float IMPULSO_PULO      =   6.0f;  // velocidade p/ cima ao pular
const float ALTURA_TETO       =   6.0f;  // capivara não passa disso
const float CAPIVARA_Y_INICIAL=   3.0f;  // altura no começo

const int   NUM_CANOS         =   4;     // pares reutilizados
const float VELOCIDADE_CANO   =   3.0f;  // unidades/s p/ a esquerda
const float ESPACO_CANOS      =   5.0f;  // distância entre pares
const float LARGURA_CANO      =   1.2f;  // espessura do cano
const float ALTURA_BRECHA     =   2.6f;  // tamanho da abertura
const float CANO_X_INICIAL    =   7.0f;  // x do primeiro cano
const float CANO_X_RECICLA    =  -8.0f;  // sai de cena à esquerda

const float RAIO_CAPIVARA     =   0.7f;  // p/ colisão (esfera e meia-AABB)
const float RAIO_INIMIGO      =   0.4f;
const float RAIO_PERCEPCAO    =   3.0f;  // distância p/ começar a perseguir
const float VEL_PERSEGUICAO   =   2.2f;  // unidades/s ao perseguir
const float VEL_VAGUEIO       =   1.2f;  // unidades/s ao vagar
const float INTERVALO_SORTEIO =   1.5f;  // s entre sorteios de direção

const float DURACAO_BATIDA    =   0.25f; // s de uma batida de asa
const float AMPLITUDE_BATIDA  =  35.0f;  // graus de abertura da batida
```

- [ ] **Step 2: Adicionar o estado global**

Logo após o bloco acima (e antes da struct `Modelo` ou logo depois dela; deixe junto dos
outros globais `g_capivara`/`g_asas`):

```cpp
// ============================================================
//  ESTADO GLOBAL DO JOGO
// ============================================================
enum EstadoJogo { INICIO, JOGANDO, GAMEOVER };
EstadoJogo g_estado = INICIO;

float g_capivaraY      = CAPIVARA_Y_INICIAL;
float g_velocidadeY    = 0.0f;
float g_tempoUltimoPulo= -10.0f;   // bem no passado => asas em repouso
int   g_pontuacao      = 0;

struct Cano { float x; float centroBrecha; bool contado; };
Cano g_canos[NUM_CANOS];

enum EstadoIA { VAGANDO, PERSEGUINDO };
struct Inimigo {
    float x, y;
    float vx, vy;
    EstadoIA estadoIA;
    float tempoProxSorteio;
};
Inimigo g_inimigo;

float g_tempoAnterior = 0.0f;      // p/ calcular dt no idle
```

- [ ] **Step 3: Compilar**

Run: `./build.sh`
Expected: compila sem erros. (O jogo abre igual ao de antes — `CAPIVARA_Y` ainda não é
usado em lugar nenhum porque o `desenharCapivara` será ajustado na Task 2; se o build
reclamar de `CAPIVARA_Y` indefinido em `desenharCapivara`/`desenharAsas`, troque por
`g_capivaraY` agora.) Feche com ESC.

- [ ] **Step 4: Commit**

```bash
git add main.cpp
git commit -m "feat: parametros de jogo e estado global"
```

---

## Task 2: Física, pulo e câmera acompanhando

**Files:**
- Modify: `main.cpp` (`desenharCapivara`, `desenharAsas`, `display`, `teclado`, novo `mouse`, `idle`, `main`)

- [ ] **Step 1: Usar `g_capivaraY` no desenho da capivara + inclinação**

Substitua o corpo de `desenharCapivara()` por:

```cpp
void desenharCapivara() {
    // Inclina conforme a velocidade: nariz p/ cima subindo, p/ baixo caindo.
    float inclina = g_velocidadeY * 4.0f;          // graus (proporcional)
    if (inclina >  30.0f) inclina =  30.0f;
    if (inclina < -45.0f) inclina = -45.0f;

    glPushMatrix();
        glTranslatef(CAPIVARA_X, g_capivaraY, 0.0f);
        glRotatef(90.0f, 0.0f, 1.0f, 0.0f);   // de perfil, olhando p/ direita
        glRotatef(inclina, 0.0f, 0.0f, 1.0f); // inclinação do voo
        desenharModelo(g_capivara);
    glPopMatrix();
}
```

- [ ] **Step 2: As asas seguem a capivara**

Em `desenharAsas()`, troque as duas referências a `CAPIVARA_Y` por `g_capivaraY` na linha do
`glTranslatef`. (A correção do bater de asas vem na Task 7; por ora pode continuar contínua.)

- [ ] **Step 3: Adicionar `idle()` com física**

Substitua o corpo de `idle()` por:

```cpp
void idle() {
    float agora = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    float dt = agora - g_tempoAnterior;
    g_tempoAnterior = agora;
    if (dt > 0.05f) dt = 0.05f;   // evita "pulo" grande se travar

    if (g_estado == JOGANDO) {
        // gravidade + integração da posição
        g_velocidadeY += GRAVIDADE * dt;
        g_capivaraY   += g_velocidadeY * dt;

        // teto: limita (não mata)
        if (g_capivaraY > ALTURA_TETO) {
            g_capivaraY = ALTURA_TETO;
            g_velocidadeY = 0.0f;
        }
        // chão (vira game over na Task 4; por enquanto só trava p/ testar)
        if (g_capivaraY < 0.0f) {
            g_capivaraY = 0.0f;
            g_velocidadeY = 0.0f;
        }
    }
    glutPostRedisplay();
}
```

- [ ] **Step 4: Pulo no teclado e no mouse**

Substitua `teclado()` por:

```cpp
void pular() {
    if (g_estado == JOGANDO) {
        g_velocidadeY = IMPULSO_PULO;
        g_tempoUltimoPulo = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    }
}

void teclado(unsigned char tecla, int x, int y) {
    if (tecla == 27) exit(0);          // ESC fecha
    if (tecla == ' ') {
        if (g_estado == JOGANDO) pular();
        // INICIO/GAMEOVER tratados na Task 3 (começar/reiniciar)
    }
}

void mouse(int botao, int estadoBotao, int x, int y) {
    if (botao == GLUT_LEFT_BUTTON && estadoBotao == GLUT_DOWN) {
        if (g_estado == JOGANDO) pular();
    }
}
```

Para testar agora, force o estado: em `main()`, logo antes de `glutMainLoop()`, adicione
**temporariamente** `g_estado = JOGANDO;` (será removido na Task 3).

- [ ] **Step 5: Registrar o callback de mouse**

Em `main()`, junto dos outros `glut...Func`, adicione:

```cpp
glutMouseFunc(mouse);
```

- [ ] **Step 6: Compilar e testar**

Run: `./build.sh`
Expected: a capivara **cai** sozinha por gravidade; ESPAÇO ou clique fazem ela **subir**;
ela inclina o nariz ao subir/cair; trava no chão e no teto. Feche com ESC.

- [ ] **Step 7: Commit**

```bash
git add main.cpp
git commit -m "feat: fisica de queda e pulo da capivara"
```

---

## Task 3: Estados do jogo (INICIO / JOGANDO / GAMEOVER)

**Files:**
- Modify: `main.cpp` (nova `reiniciarJogo`, `teclado`, `mouse`, `main`)

- [ ] **Step 1: Função de reset**

Adicione antes de `display()`:

```cpp
// Reinicia tudo para um novo jogo e começa a jogar.
void reiniciarJogo() {
    g_capivaraY   = CAPIVARA_Y_INICIAL;
    g_velocidadeY = 0.0f;
    g_pontuacao   = 0;
    g_tempoUltimoPulo = -10.0f;

    // Canos serão posicionados na Task 5; aqui só zera o estado básico.
    g_estado = JOGANDO;
    pular();   // primeiro impulso ao começar
}
```

- [ ] **Step 2: ESPAÇO/clique começam e reiniciam**

Atualize `teclado()` (ramo do espaço) e `mouse()` para tratar os três estados:

```cpp
void acaoPrincipal() {
    if      (g_estado == INICIO)   reiniciarJogo();
    else if (g_estado == JOGANDO)  pular();
    else if (g_estado == GAMEOVER) reiniciarJogo();
}
```

E faça `teclado` (tecla `' '`) e `mouse` (clique esquerdo) chamarem `acaoPrincipal()` em vez
de chamar `pular()` direto.

- [ ] **Step 3: Remover o estado forçado**

Em `main()`, **remova** o `g_estado = JOGANDO;` temporário da Task 2. O jogo começa em
`INICIO`.

- [ ] **Step 4: Compilar e testar**

Run: `./build.sh`
Expected: ao abrir, a capivara fica **parada** (estado INICIO, sem gravidade). ESPAÇO/clique
**começam** o jogo (ela passa a cair/pular). (Game over real vem na Task 4; ainda não há como
chegar em GAMEOVER.) Feche com ESC.

- [ ] **Step 5: Commit**

```bash
git add main.cpp
git commit -m "feat: maquina de estados do jogo (inicio/jogando/gameover)"
```

---

## Task 4: Funções puras de colisão + auto-testes

**Files:**
- Modify: `main.cpp` (novas funções puras + `rodarTestes` + ramo `--testes` em `main`)

- [ ] **Step 1: Escrever as funções puras**

Adicione na seção de funções puras (antes dos desenhos):

```cpp
#include <cassert>   // (coloque junto dos outros includes, no topo)

// Sobreposição de duas caixas alinhadas aos eixos (AABB), em 2D (X,Y).
bool sobreposicaoAABB(float ax0, float ax1, float ay0, float ay1,
                      float bx0, float bx1, float by0, float by1) {
    return ax0 <= bx1 && ax1 >= bx0 && ay0 <= by1 && ay1 >= by0;
}

// Distância euclidiana entre dois pontos no plano de jogo (z = 0).
float distanciaEsferas(float x0, float y0, float x1, float y1) {
    float dx = x1 - x0, dy = y1 - y0;
    return sqrtf(dx*dx + dy*dy);
}
```

- [ ] **Step 2: Escrever os asserts (o "teste")**

Adicione logo abaixo:

```cpp
void rodarTestes() {
    // AABB: caixas que se tocam => true; separadas => false
    assert( sobreposicaoAABB(0,2, 0,2,  1,3, 1,3) == true);
    assert( sobreposicaoAABB(0,1, 0,1,  2,3, 2,3) == false);
    assert( sobreposicaoAABB(0,2, 0,2,  2,4, 2,4) == true);   // encostando

    // Distância: (0,0)->(3,4) = 5
    assert( distanciaEsferas(0,0, 3,4) > 4.999f &&
            distanciaEsferas(0,0, 3,4) < 5.001f );
    assert( distanciaEsferas(1,1, 1,1) == 0.0f );

    printf("TODOS OS TESTES OK\n");
}
```

- [ ] **Step 3: Ramo `--testes` no `main`**

Logo no início de `main()` (antes de `glutInit`):

```cpp
if (argc > 1 && std::string(argv[1]) == "--testes") {
    rodarTestes();
    return 0;
}
```

Inclua `#include <string>` no topo.

- [ ] **Step 4: Compilar e rodar os testes**

Run: `./build.sh` (compila; pode fechar a janela com ESC) e depois
`./flappy_capivara --testes`
Expected: imprime `TODOS OS TESTES OK` e sai (sem abrir janela).

- [ ] **Step 5: Commit**

```bash
git add main.cpp
git commit -m "feat: funcoes puras de colisao (AABB e esfera) com auto-testes"
```

---

## Task 5: Canos — criar, mover, reciclar, desenhar, pontuar

**Files:**
- Modify: `main.cpp` (novas `inicializarCanos`, `atualizarCanos`, `desenharCanos`; chamadas em `reiniciarJogo`, `idle`, `display`, `main`)

- [ ] **Step 1: Inicializar os canos**

Adicione (precisa de `#include <cstdlib>` — já existe — para `rand`):

```cpp
// Sorteia uma altura de brecha dentro de uma faixa segura.
float sortearBrecha() {
    // entre 1.6 e 4.4 (deixa folga p/ o chão e o teto)
    return 1.6f + (rand() % 100) / 100.0f * 2.8f;
}

void inicializarCanos() {
    for (int i = 0; i < NUM_CANOS; i++) {
        g_canos[i].x = CANO_X_INICIAL + i * ESPACO_CANOS;
        g_canos[i].centroBrecha = sortearBrecha();
        g_canos[i].contado = false;
    }
}
```

Chame `inicializarCanos();` dentro de `reiniciarJogo()` (após zerar `g_pontuacao`).

- [ ] **Step 2: Atualizar os canos (mover/reciclar/pontuar)**

```cpp
void atualizarCanos(float dt) {
    for (int i = 0; i < NUM_CANOS; i++) {
        g_canos[i].x -= VELOCIDADE_CANO * dt;

        // pontua quando o cano passa do X da capivara
        if (!g_canos[i].contado && g_canos[i].x < CAPIVARA_X) {
            g_canos[i].contado = true;
            g_pontuacao++;
        }
        // recicla para a direita ao sair de cena
        if (g_canos[i].x < CANO_X_RECICLA) {
            g_canos[i].x += NUM_CANOS * ESPACO_CANOS;
            g_canos[i].centroBrecha = sortearBrecha();
            g_canos[i].contado = false;
        }
    }
}
```

Chame `atualizarCanos(dt);` no `idle()`, dentro do `if (g_estado == JOGANDO)`.

- [ ] **Step 3: Desenhar os canos**

```cpp
// Desenha um bloco (cubo escalado) entre y0 e y1, no x dado.
void desenharBlocoCano(float x, float y0, float y1) {
    float altura = y1 - y0;
    glPushMatrix();
        glTranslatef(x, (y0 + y1) / 2.0f, 0.0f);
        glScalef(LARGURA_CANO, altura, LARGURA_CANO);
        glutSolidCube(1.0f);
    glPopMatrix();
}

void desenharCanos() {
    glColor3f(0.20f, 0.70f, 0.25f);   // verde cano
    for (int i = 0; i < NUM_CANOS; i++) {
        float c = g_canos[i].centroBrecha;
        float meia = ALTURA_BRECHA / 2.0f;
        desenharBlocoCano(g_canos[i].x, 0.0f, c - meia);          // cano de baixo
        desenharBlocoCano(g_canos[i].x, c + meia, ALTURA_TETO+2); // cano de cima
    }
}
```

Chame `desenharCanos();` no `display()`, depois do chão e antes da capivara.

- [ ] **Step 4: Compilar e testar**

Run: `./build.sh`
Expected: comece o jogo (ESPAÇO). Pares de canos verdes rolam da direita p/ a esquerda, com
brechas em alturas variadas, e reaparecem à direita. Atravessando a brecha, nada mata ainda
(colisão é a Task 6). Feche com ESC.

- [ ] **Step 5: Commit**

```bash
git add main.cpp
git commit -m "feat: canos rolando, reciclagem e contagem de pontos"
```

---

## Task 6: Colisão capivara × canos e chão → game over

**Files:**
- Modify: `main.cpp` (nova `verificarColisoesCanos`; chamada no `idle`)

- [ ] **Step 1: Verificação de colisão usando AABB**

```cpp
// Caixa da capivara (centro em CAPIVARA_X, g_capivaraY; meia-extensão = RAIO_CAPIVARA)
void verificarColisoesCanos() {
    float cx0 = CAPIVARA_X - RAIO_CAPIVARA, cx1 = CAPIVARA_X + RAIO_CAPIVARA;
    float cy0 = g_capivaraY - RAIO_CAPIVARA, cy1 = g_capivaraY + RAIO_CAPIVARA;

    // chão
    if (g_capivaraY - RAIO_CAPIVARA <= 0.0f) { g_estado = GAMEOVER; return; }

    float meiaL = LARGURA_CANO / 2.0f;
    for (int i = 0; i < NUM_CANOS; i++) {
        float px0 = g_canos[i].x - meiaL, px1 = g_canos[i].x + meiaL;
        float c = g_canos[i].centroBrecha, meia = ALTURA_BRECHA / 2.0f;

        // cano de baixo: y de 0 até (c - meia)
        bool bate = sobreposicaoAABB(cx0,cx1, cy0,cy1, px0,px1, 0.0f, c - meia);
        // cano de cima: y de (c + meia) até o topo
        bate = bate || sobreposicaoAABB(cx0,cx1, cy0,cy1, px0,px1,
                                        c + meia, ALTURA_TETO + 2.0f);
        if (bate) { g_estado = GAMEOVER; return; }
    }
}
```

Chame `verificarColisoesCanos();` no `idle()`, dentro do `if (JOGANDO)`, **depois** de
`atualizarCanos`.

- [ ] **Step 2: Compilar e testar**

Run: `./build.sh`
Expected: bater em um cano ou tocar o chão **congela** o jogo (estado GAMEOVER). ESPAÇO/clique
reinicia. Passar limpo pela brecha não mata e o placar (ainda no terminal? não — só na Task 8)
deve estar contando internamente. Feche com ESC.

- [ ] **Step 3: Commit**

```bash
git add main.cpp
git commit -m "feat: colisao AABB com canos e chao causa game over"
```

---

## Task 7: Inimigo + IA (FSM) + colisão por esfera

**Files:**
- Modify: `main.cpp` (novas `inicializarInimigo`, `atualizarInimigo`, `desenharInimigo`, `verificarColisaoInimigo`; chamadas em `reiniciarJogo`, `idle`, `display`)

- [ ] **Step 1: Inicializar o inimigo**

```cpp
void inicializarInimigo() {
    g_inimigo.x = 4.0f;
    g_inimigo.y = 3.0f;
    g_inimigo.vx = -VEL_VAGUEIO;
    g_inimigo.vy = 0.0f;
    g_inimigo.estadoIA = VAGANDO;
    g_inimigo.tempoProxSorteio = 0.0f;
}
```

Chame `inicializarInimigo();` dentro de `reiniciarJogo()`.

- [ ] **Step 2: Atualizar a IA (FSM reativa)**

```cpp
void atualizarInimigo(float dt) {
    // distância até a capivara (mesma fórmula da colisão por esfera)
    float D = distanciaEsferas(g_inimigo.x, g_inimigo.y, CAPIVARA_X, g_capivaraY);

    // transição de estado
    g_inimigo.estadoIA = (D <= RAIO_PERCEPCAO) ? PERSEGUINDO : VAGANDO;

    if (g_inimigo.estadoIA == PERSEGUINDO) {
        // move em direção à capivara
        float dx = CAPIVARA_X - g_inimigo.x;
        float dy = g_capivaraY - g_inimigo.y;
        float n = sqrtf(dx*dx + dy*dy);
        if (n > 0.0001f) {
            g_inimigo.x += (dx / n) * VEL_PERSEGUICAO * dt;
            g_inimigo.y += (dy / n) * VEL_PERSEGUICAO * dt;
        }
    } else {
        // VAGANDO: sorteia nova direção de tempos em tempos
        float agora = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
        if (agora >= g_inimigo.tempoProxSorteio) {
            float ang = (rand() % 360) * 3.14159f / 180.0f;
            g_inimigo.vx = cosf(ang) * VEL_VAGUEIO;
            g_inimigo.vy = sinf(ang) * VEL_VAGUEIO;
            g_inimigo.tempoProxSorteio = agora + INTERVALO_SORTEIO;
        }
        g_inimigo.x += g_inimigo.vx * dt;
        g_inimigo.y += g_inimigo.vy * dt;

        // mantém dentro da área de jogo (rebatendo nas bordas)
        if (g_inimigo.x < 0.0f)  { g_inimigo.x = 0.0f;  g_inimigo.vx = -g_inimigo.vx; }
        if (g_inimigo.x > 6.0f)  { g_inimigo.x = 6.0f;  g_inimigo.vx = -g_inimigo.vx; }
        if (g_inimigo.y < 1.0f)  { g_inimigo.y = 1.0f;  g_inimigo.vy = -g_inimigo.vy; }
        if (g_inimigo.y > ALTURA_TETO) { g_inimigo.y = ALTURA_TETO; g_inimigo.vy = -g_inimigo.vy; }
    }
}
```

Chame `atualizarInimigo(dt);` no `idle()` dentro do `if (JOGANDO)`.

- [ ] **Step 3: Colisão por esfera com o inimigo**

```cpp
void verificarColisaoInimigo() {
    float D = distanciaEsferas(g_inimigo.x, g_inimigo.y, CAPIVARA_X, g_capivaraY);
    if (D <= RAIO_INIMIGO + RAIO_CAPIVARA) g_estado = GAMEOVER;
}
```

Chame `verificarColisaoInimigo();` no `idle()`, dentro do `if (JOGANDO)`, após
`atualizarInimigo`.

- [ ] **Step 4: Desenhar o inimigo (primitiva)**

```cpp
void desenharInimigo() {
    glPushMatrix();
        glTranslatef(g_inimigo.x, g_inimigo.y, 0.0f);
        // amarelo vagando, vermelho perseguindo (deixa a IA visível)
        if (g_inimigo.estadoIA == PERSEGUINDO) glColor3f(0.9f, 0.2f, 0.1f);
        else                                   glColor3f(0.95f, 0.85f, 0.1f);
        glutSolidSphere(RAIO_INIMIGO, 16, 16);
    glPopMatrix();
}
```

Chame `desenharInimigo();` no `display()`, depois da capivara.

- [ ] **Step 5: Compilar e testar**

Run: `./build.sh`
Expected: uma esfera amarela **vaga** pela área quando a capivara está longe; ao chegar perto
(≤ raio de percepção) ela fica **vermelha** e **persegue** a capivara; encostar = game over.
Feche com ESC.

- [ ] **Step 6: Commit**

```bash
git add main.cpp
git commit -m "feat: inimigo com IA (FSM vagar/perseguir) e colisao por esfera"
```

---

## Task 8: HUD — placar e mensagens de estado

**Files:**
- Modify: `main.cpp` (novas `desenharTexto`, `desenharHUD`; chamada no `display`)

- [ ] **Step 1: Função de texto em projeção ortográfica**

```cpp
// Desenha texto 2D na tela (coordenadas de pixel), por cima da cena.
void desenharTexto(float x, float y, const char* texto) {
    glRasterPos2f(x, y);
    for (const char* c = texto; *c != '\0'; c++)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
}

void desenharHUD() {
    // entra em 2D: salva projeção/modelview, desliga luz e profundidade
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    gluOrtho2D(0, LARGURA_JANELA, 0, ALTURA_JANELA);
    glMatrixMode(GL_MODELVIEW);  glPushMatrix(); glLoadIdentity();

    glColor3f(1.0f, 1.0f, 1.0f);
    char buf[64];

    if (g_estado == INICIO) {
        desenharTexto(250, 360, "FLAPPY CAPIVARA");
        desenharTexto(210, 300, "ESPACO ou clique para comecar");
    } else if (g_estado == JOGANDO) {
        snprintf(buf, sizeof(buf), "Pontos: %d", g_pontuacao);
        desenharTexto(20, ALTURA_JANELA - 30, buf);
    } else if (g_estado == GAMEOVER) {
        desenharTexto(300, 360, "GAME OVER");
        snprintf(buf, sizeof(buf), "Pontuacao: %d", g_pontuacao);
        desenharTexto(300, 320, buf);
        desenharTexto(210, 270, "ESPACO ou clique para reiniciar");
    }

    // restaura 3D
    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW);  glPopMatrix();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}
```

Inclua `#include <cstdio>` (já existe, por causa de `printf`/`snprintf`).

- [ ] **Step 2: Chamar o HUD por último no display**

No fim de `display()`, **antes** de `glutSwapBuffers()`, adicione `desenharHUD();`.

- [ ] **Step 3: Compilar e testar**

Run: `./build.sh`
Expected: tela inicial mostra "FLAPPY CAPIVARA" + instrução; jogando mostra "Pontos: N"
subindo a cada cano; game over mostra a pontuação e instrução de reinício. Feche com ESC.

- [ ] **Step 4: Commit**

```bash
git add main.cpp
git commit -m "feat: HUD com placar e mensagens de inicio/game over"
```

---

## Task 9: Correção das asas (batida simétrica disparada pelo pulo)

**Files:**
- Modify: `main.cpp` (`desenharAsas` reescrita; nova `desenharMetadeAsa`)

Contexto: o `wings.obj` é um par rígido (x de −88 a +88). Girar o par inteiro vira
"gangorra". A correção desenha **cada metade** (esquerda x ≤ centro, direita x > centro) com
rotações **opostas** em torno do eixo da raiz (eixo Z do modelo), pelo mesmo ângulo. O ângulo
vem de uma **batida única** disparada pelo pulo.

- [ ] **Step 1: Desenhar uma metade do modelo de asa**

Adicione antes de `desenharAsas()`:

```cpp
// Desenha só as faces de um lado do modelo das asas.
// lado = -1 (esquerda, x <= centro) ou +1 (direita, x > centro).
void desenharMetadeAsa(int lado) {
    const Modelo& mod = g_asas;
    glColor3f(mod.corR, mod.corG, mod.corB);
    for (unsigned int m = 0; m < mod.cena->mNumMeshes; m++) {
        const aiMesh* malha = mod.cena->mMeshes[m];
        glBegin(GL_TRIANGLES);
        for (unsigned int f = 0; f < malha->mNumFaces; f++) {
            const aiFace& face = malha->mFaces[f];
            // centro X da face (média dos 3 vértices) decide o lado
            float cx = 0.0f;
            for (unsigned int i = 0; i < face.mNumIndices; i++)
                cx += malha->mVertices[face.mIndices[i]].x;
            cx /= face.mNumIndices;
            bool ehDireita = (cx > mod.centroX);
            if ((lado > 0) != ehDireita) continue;   // pula o lado errado

            for (unsigned int i = 0; i < face.mNumIndices; i++) {
                unsigned int idx = face.mIndices[i];
                if (malha->HasNormals()) {
                    aiVector3D n = malha->mNormals[idx];
                    glNormal3f(n.x, n.y, n.z);
                }
                aiVector3D p = malha->mVertices[idx];
                glVertex3f(p.x, p.y, p.z);
            }
        }
        glEnd();
    }
}
```

- [ ] **Step 2: Reescrever `desenharAsas` com batida no pulo**

```cpp
void desenharAsas() {
    // Batida única disparada pelo pulo (one-shot).
    float agora = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    float t = agora - g_tempoUltimoPulo;
    float anguloAsa = 0.0f;                       // repouso
    if (t < DURACAO_BATIDA) {
        float prog = t / DURACAO_BATIDA;          // 0 -> 1
        anguloAsa = sinf(prog * 3.14159f) * AMPLITUDE_BATIDA;  // sobe e volta
    }

    glPushMatrix();
        // posiciona o par nas costas da capivara
        glTranslatef(CAPIVARA_X + ASA_DX, g_capivaraY + ASA_DY, ASA_DZ);
        glRotatef(ASA_ROT_Y, 0.0f, 1.0f, 0.0f);
        glRotatef(ASA_ROT_Z, 0.0f, 0.0f, 1.0f);
        glScalef(g_asas.escala, g_asas.escala, g_asas.escala);
        glTranslatef(-g_asas.centroX, -g_asas.centroY, -g_asas.centroZ);

        // asa direita: gira +angulo em torno de Z (eixo da raiz)
        glPushMatrix();
            glRotatef( anguloAsa, 0.0f, 0.0f, 1.0f);
            desenharMetadeAsa(+1);
        glPopMatrix();

        // asa esquerda: gira -angulo (sentido oposto => bater simétrico)
        glPushMatrix();
            glRotatef(-anguloAsa, 0.0f, 0.0f, 1.0f);
            desenharMetadeAsa(-1);
        glPopMatrix();
    glPopMatrix();
}
```

Observação: se a batida parecer girar no eixo errado ao rodar, troque o eixo do
`glRotatef(±anguloAsa, ...)` (ex.: usar X em vez de Z) e/ou ajuste `ASA_ROT_Y/ASA_ROT_Z` e os
offsets `ASA_DX/DY/DZ`. O ponto fixo da correção é: **as duas metades giram em sentidos
opostos pelo mesmo ângulo**.

- [ ] **Step 3: Compilar e testar**

Run: `./build.sh`
Expected: as asas ficam **paradas** quando a capivara está caindo; a cada ESPAÇO/clique elas
dão **uma batida simétrica** (as duas pontas sobem e descem juntas, sem gangorra). Feche com
ESC.

- [ ] **Step 4: Commit**

```bash
git add main.cpp
git commit -m "fix: asas batem simetricamente e so no ato do pulo"
```

---

## Task 10: Visibilidade extra (back-face culling) e revisão final

**Files:**
- Modify: `main.cpp` (`inicializarOpenGL`)

- [ ] **Step 1: Ativar back-face culling**

No fim de `inicializarOpenGL()`:

```cpp
// Visibilidade: não desenha faces traseiras (recurso de visibilidade).
glEnable(GL_CULL_FACE);
glCullFace(GL_BACK);
```

- [ ] **Step 2: Compilar e testar**

Run: `./build.sh`
Expected: jogo continua igual e correto. Se a capivara, as asas ou os canos sumirem/ficarem
"furados" (faces com ordem invertida), **remova** o culling (não vale arriscar o visual por um
bônus). Rode também `./flappy_capivara --testes` → `TODOS OS TESTES OK`.

- [ ] **Step 3: Verificação final (rodar o jogo inteiro)**

Run: `./build.sh` e confirme tudo:
- Tela inicial → ESPAÇO começa.
- Capivara cai/pula; asas batem só no pulo; nariz inclina.
- Canos rolam, reciclam; placar sobe ao passar.
- Bater em cano/chão/inimigo → game over; reinício funciona.
- Inimigo vaga (amarelo) e persegue (vermelho) ao chegar perto.
- HUD correto em cada estado.

- [ ] **Step 4: Commit**

```bash
git add main.cpp
git commit -m "feat: back-face culling e revisao final do jogo"
```

---

## Cobertura do spec (auto-revisão)

- 6.1 Estados do jogo → Task 3, Task 8 (mensagens)
- 6.2 Física e controle (gravidade, pulo, inclinação) → Task 2
- 6.3 Canos (mover/reciclar/pontuar) → Task 5
- 6.4 Colisão AABB (canos/chão) → Task 4 (função) + Task 6; esfera (inimigo) → Task 4 + Task 7
- 6.5 IA do inimigo (FSM vagar/perseguir) → Task 7
- 6.6 Asas (split esquerda/direita + batida no pulo) → Task 9
- 6.7 Renderização / visibilidade (culling) → Task 10 (iluminação/sombreamento/textura já existem)
- 6.8 HUD → Task 8
- §9 Verificação (auto-testes + rodar o jogo) → Task 4 (`--testes`) + Task 10
