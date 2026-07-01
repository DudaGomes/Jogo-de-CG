# Guia de Apresentação — Flappy Capivara

Guia completo para apresentar e defender o trabalho de **Computação Gráfica**.
Tudo está em um único arquivo: [`main.cpp`](main.cpp) (~1840 linhas), organizado em
seções. As linhas citadas são aproximadas.

---

## 0. Resumo em uma frase

Flappy Bird em 3D (renderização 3D, jogabilidade 2D no plano XY) feito em **C++
com OpenGL/GLUT**, onde a capivara desvia de canos e uma **abelha aliada com IA
de piloto automático** voa à frente navegando as brechas.

**Stack e o porquê de cada peça:**

| Biblioteca | Para quê | Por que essa |
|---|---|---|
| **OpenGL** | Renderização (geometria, luz, textura, z-buffer) | É o exigido pela disciplina |
| **GLUT** | Janela, contexto e eventos (teclado/mouse/idle) | Simples, multiplataforma, clássico em CG acadêmico |
| **Assimp** | Carregar modelos `.obj` e `.glb` | Evita escrever um parser; triangula e gera normais |
| **stb_image** | Ler a textura PNG da capivara | Header único, domínio público |
| **stb_truetype** | Rasterizar a fonte (título/HUD) | Header único; fonte real sem depender do SO |
| **miniaudio** | Saída de áudio | Header único; tocamos áudio **sintetizado** por nós |
| **Blender** | Remodelar o cano (corpo + borda) | Corrigir a distorção da borda ao esticar |

**Por que um único arquivo `main.cpp`?** O projeto é pequeno e o objetivo é
conseguir **explicar tudo** de cima a baixo. Um arquivo em seções é mais fácil de
ler e defender do que vários módulos.

---

## 1. Como o programa roda (o esqueleto)

`main()` ([main.cpp:1691](main.cpp)):
1. Se rodar com `--testes`, executa os testes das funções puras e sai.
2. `glutInit` + `glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH)` — pede
   **double buffering** (evita "piscar"), cor RGB e **z-buffer**.
3. Cria a janela e **registra os callbacks**:
   - `display` — desenha um quadro.
   - `reshape` — recalcula projeção quando a janela muda de tamanho.
   - `teclado` / `mouse` — entrada.
   - `idle` — chamado o tempo todo quando não há eventos: é o **loop de física**.
4. `inicializarOpenGL()` — liga luz, z-buffer, culling, material (config fixa).
5. Carrega modelos, fonte, textura procedural e áudio.
6. `glutMainLoop()` — entra no laço infinito do GLUT.

**Loop do jogo:** `idle()` ([main.cpp:1607](main.cpp)) calcula `dt` (tempo real
desde o último quadro, com `glutGet(GLUT_ELAPSED_TIME)`), atualiza a física **só se
`JOGANDO`**, atualiza efeitos e chama `glutPostRedisplay()` (pede um novo quadro).
Usar `dt` real deixa o jogo **independente da taxa de quadros**.

---

## 2. Câmera e projeção 3D

- **Projeção perspectiva**: `gluPerspective(45, aspecto, 0.1, 100)` em `reshape`
  ([main.cpp:1549](main.cpp)). Perspectiva = objetos distantes ficam menores (dá
  profundidade). O `0.1` e `100` são os planos *near/far* (o que é visível em Z).
- **Câmera**: `gluLookAt(0,2,10, 0,2,0, 0,1,0)` em `display` ([main.cpp:1466](main.cpp))
  — a câmera fica afastada em **Z=10**, olhando o centro. O "up" é o eixo Y.
- **Jogabilidade 2D**: tudo acontece no plano **XY** (a capivara só sobe/desce; os
  canos andam em X). O 3D vem da renderização e do cenário em camadas de Z.

---

## 3. Iluminação e sombreamento ⭐ (tópico forte da 3ª unidade)

Tudo em `inicializarOpenGL()` ([main.cpp:1643](main.cpp)):

- **Luz direcional (o Sol)**: `dirSol = {-0.4, 1.0, 0.6, 0.0}`. O **4º componente = 0**
  significa **direção** (raios paralelos, como o Sol), e não uma posição (ponto).
- **Três componentes do modelo de reflexão**:
  - **Difusa** `{1.0, 0.96, 0.86}` — luz amarelada (quente).
  - **Ambiente** `{0.40, 0.42, 0.48}` — preenchimento frio, pra as sombras não
    ficarem pretas.
  - **Especular** `{0.5,0.5,0.5}` + **material especular** + `GL_SHININESS 24`
    ([main.cpp:1679](main.cpp)) — o brilho pontual; shininess controla o tamanho do brilho.
- **Sombreamento Gouraud** (suave, por vértice): vem das **normais por vértice**
  geradas na carga (`aiProcess_GenSmoothNormals`, [main.cpp:308](main.cpp)) + interpolação padrão.
- **`GL_COLOR_MATERIAL`** ([main.cpp:1673](main.cpp)): faz `glColor` definir a cor
  do material (ambiente+difusa). Assim pintamos objetos com `glColor` normalmente.
- **`GL_NORMALIZE`**: como escalamos os modelos, as normais precisam ser
  renormalizadas, senão a luz fica errada.
- A luz é **reposicionada em `display`** ([main.cpp:1471](main.cpp)) **depois** do
  `gluLookAt`, pra a direção do Sol ficar fixa no mundo (não gira com a câmera).

**Por que direcional e não pontual?** O mundo "rola" infinito sob o céu; raios
paralelos iluminam tudo por igual, independente da posição. Uma luz pontual criaria
falloff (queda de intensidade) estranho num cenário que se repete.

---

## 4. Texturas

Duas texturas, duas técnicas:

1. **Capivara — imagem PNG** (`carregarTextura`, [main.cpp:323](main.cpp)): lida com
   `stb_image`, enviada com `glTexImage2D`, filtro `GL_LINEAR` (suave), mapeada pelas
   **coordenadas UV** do modelo (`glTexCoord2f`, [main.cpp:439](main.cpp)).
   `stbi_set_flip_vertically_on_load` corrige a origem (OpenGL lê de baixo p/ cima).
2. **Chão — textura procedural** (`criarTexturaGrama`, [main.cpp:349](main.cpp)):
   geramos uma imagem 32×32 **por código** (verdes variados + fios claros/escuros),
   com filtro **`GL_NEAREST`** (aspecto pixelado, sem borrar) e **`GL_REPEAT`**
   (tiling). No chão usamos `glTexCoord2f` até 12/8, então a textura **se repete**
   várias vezes ([main.cpp:1494](main.cpp)).

**Por que NEAREST e não LINEAR no chão?** NEAREST mantém o visual retrô/pixelado;
LINEAR suavizaria e perderia o estilo.

---

## 5. Modelos 3D (Assimp + bounding box + display lists)

- **Carga** (`carregarModelo`, [main.cpp:304](main.cpp)): `aiImportFile` com
  `aiProcess_Triangulate` (vira tudo triângulo) e `aiProcess_GenSmoothNormals`
  (cria normais suaves). Carregamos `.obj` (capivara, asas, abelha, grama, 2 árvores)
  e `.glb` (cano).
- **Bounding box** (`calcularBoundingBox`, [main.cpp:274](main.cpp)): mede a caixa
  que envolve todos os vértices → guarda o **centro** (para centralizar na origem) e
  a **maior dimensão** (para **escalar** todo modelo a um tamanho-alvo padrão). É o
  que permite usar modelos de tamanhos diferentes sem ajustar cada um na mão.
- **Display lists** (`desenharModelo`, [main.cpp:389](main.cpp)): na 1ª vez a
  geometria é **compilada** (`glNewList/glEndList`) e fica na GPU; nas próximas só
  `glCallList`. Ganho grande de performance ao desenhar **muitas cópias** (grama e
  árvores são dezenas por quadro) sem reenviar vértices toda hora.
- **Cor por material** (`corMaterial` + `usarCorMaterial`): lê a cor difusa (Kd) do
  `.mtl`/material via `aiGetMaterialColor` — é assim que a abelha sai amarela/preta.
  `desaturar` puxa a cor levemente pro cinza (harmoniza a paleta).

---

## 6. Visibilidade

- **Z-buffer** (`GL_DEPTH_TEST`, [main.cpp:1649](main.cpp)): objeto mais perto tapa o
  mais longe. Limpamos o depth buffer a cada quadro (`glClear(... GL_DEPTH_BUFFER_BIT)`).
- **Back-face culling** (`GL_CULL_FACE` + `glCullFace(GL_BACK)`, [main.cpp:1686](main.cpp)):
  descarta faces **traseiras** (não visíveis), decididas pela **ordem dos vértices**
  (winding). Economiza desenho.
  - ⚠️ Detalhe que o professor pode cutucar: tivemos que ordenar os vértices do
    **chão** em sentido anti-horário (visto de cima) pra a face apontar pra cima,
    senão o culling escondia o chão. E **desligamos o culling no HUD** ([main.cpp:1423](main.cpp)),
    porque os quads de texto vêm em sentido horário e sumiriam.

---

## 7. Física e controle

- **Integração de Euler semi-implícita** no `idle` ([main.cpp:1615](main.cpp)):
  `velocidadeY += GRAVIDADE*dt; capivaraY += velocidadeY*dt`.
- **Pulo** (`pular`, [main.cpp:1557](main.cpp)): seta `velocidadeY = IMPULSO_PULO`
  (velocidade pra cima instantânea), registra o tempo (pra a batida de asa) e toca som.
- **Inclinação** (`desenharCapivara`, [main.cpp:707](main.cpp)): gira a capivara
  proporcional à velocidade (nariz pra cima subindo, pra baixo caindo), com limites.
- **Controle**: `teclado`/`mouse` chamam `acaoPrincipal()`, que decide pelo estado
  (começar / pular / reiniciar).

---

## 8. Canos (obstáculos) + o conserto no Blender

- **Geração/movimento** (`inicializarCanos`/`atualizarCanos`, [main.cpp:829](main.cpp)):
  4 pares que rolam pra esquerda e, ao sair, **reciclam** pra direita com nova brecha.
- **Reciclagem = *object pooling***: reaproveitamos os mesmos 4 canos em vez de
  criar/destruir — sem alocação de memória durante a partida.
- **Pontuação**: quando o cano passa do X da capivara, `pontuacao++` + brilho + poeira
  + som ([main.cpp:908](main.cpp)).
- **Cano com corpo + borda** (`desenharCanoModelo`, [main.cpp:965](main.cpp)): o
  `Pipe_novo.glb` tem **2 malhas**. `prepararCano` ([main.cpp:940](main.cpp)) separa
  qual é o **corpo** (mais alto) e a **borda**. Ao desenhar, **esticamos só o corpo**
  em Y (cilindro liso não distorce) e desenhamos a **borda com escala uniforme** na
  boca do cano. *Esse foi o motivo de ir ao Blender:* antes o modelo era uma peça só
  e, ao esticar, a borda deformava.

---

## 9. Detecção de colisões

- **AABB** (`sobreposicaoAABB`, [main.cpp:641](main.cpp); `verificarColisoesCanos`,
  [main.cpp:1001](main.cpp)): a capivara é uma **caixa** (centro ± `RAIO_CAPIVARA`) e
  cada cano é outra caixa; há colisão quando se sobrepõem **em X e em Y** ao mesmo
  tempo. O **chão** é um limite inferior (`y - raio ≤ 0`). Qualquer colisão chama
  `morrer()`.
- **Função pura + testes**: `sobreposicaoAABB` e `distanciaEsferas` não têm estado e
  são testadas com `./flappy_capivara --testes` ([main.cpp:653](main.cpp)).

**Por que AABB e não esferas?** Cano é um retângulo alto; uma caixa encaixa
naturalmente e o teste é **exato e barato** (4 comparações). Uma esfera seria uma
aproximação ruim (muito espaço vazio). Implementamos também a **distância entre
centros** (base da colisão por esferas) e a deixamos testada, como demonstração da
técnica.

---

## 10. Inteligência Artificial — a abelha aliada (piloto automático) ⭐

Onde: `atualizarAliada` ([main.cpp:1030](main.cpp)).

A abelha é uma **IA que joga o jogo sozinha** e guia o jogador. Ela fica num **X
fixo à frente** da capivara (`ABELHA_X`) e, **a cada quadro**:
1. **Percebe o ambiente**: procura, entre os 4 canos, o que está **entrando/logo à
   frente** dela em X (`d = cano.x - ABELHA_X`, o menor `d` acima de `-LARGURA_CANO`).
2. **Define o alvo**: o **centro da brecha** desse cano (`centroBrecha`).
3. **Age com a mesma física da capivara**: aplica **gravidade** e, se está **abaixo**
   do alvo, dá uma **"batida de asa" automática** (`vy = ABELHA_IMPULSO`).

Assim ela oscila em torno do centro da brecha e **passa sempre pelo vão** — nunca
pelo corpo do cano. Como mira no *centro* (que não depende do tamanho da brecha),
funciona em todas as dificuldades.

**Que tipo de IA é essa?** É uma **IA reativa baseada em regras** (um agente que
*percebe* o ambiente e *age* por uma regra simples — também chamada de *steering* /
piloto automático). Não é uma FSM de estados nem busca.

**Por que não A\*/pathfinding ou FSM?** O espaço de decisão é praticamente **1D** (a
abelha só escolhe subir ou não) e o **alvo é conhecido** (o centro da próxima
brecha). Uma regra reativa é **ótima e robusta** aqui; A\* precisaria de um grafo de
navegação que o jogo não tem, seria overkill. *(Observação honesta para a banca: uma
versão anterior era um inimigo com FSM vagar/perseguir; trocamos por essa IA que
realmente joga, que demonstra o conceito de forma mais marcante.)*

---

## 11. Cenário e profundidade (o que faz parecer 3D)

- **Chão** texturizado (quad no plano XZ, [main.cpp:1488](main.cpp)).
- **Grama 3D** em **5 fileiras** de profundidade Z (`desenharGrama`, [main.cpp:893](main.cpp)),
  rolando e reciclando — forma um gramado inteiro.
- **Árvores ao fundo** com **parallax**: rolam a **40%** da velocidade dos canos
  (`ARVORE_PARALLAX`, `atualizarArvores`, [main.cpp:868](main.cpp)); tamanho, modelo
  e profundidade **sorteados** ao reaparecer.
- **Céu em degradê** (`desenharCeu`, [main.cpp:594](main.cpp)): um quad em projeção
  **ortográfica** com cor diferente em cima e embaixo (interpolada), no lugar de cor
  chapada.

**Parallax** = objetos distantes se movem mais devagar → sensação de distância.

---

## 12. Partículas, "game feel" e áudio

- **Partículas** (`emitir*`/`desenharParticulas`, [main.cpp:515](main.cpp)):
  **billboards** (quads sempre virados pra tela) com **blending** (transparência,
  `GL_SRC_ALPHA`). Poeira ao passar o cano, brilho ao pontuar, explosão ao morrer.
- **Screen shake** (`g_shake`): ao morrer, a câmera treme por um translate aleatório
  em `display` ([main.cpp:1475](main.cpp)); decai no `idle`.
- **Flash** (`desenharFlash`, [main.cpp:611](main.cpp)): clarão branco rápido ao bater.
- **Áudio sintetizado** (`audioCallback`, [main.cpp:203](main.cpp)): **não usamos
  arquivos**. Um callback do miniaudio gera as amostras em tempo real —
  **onda quadrada** (pulo/ponto) e **ruído** (batida), cada uma com **envelope** de
  decaimento (estilo 8-bit). `tocarPulo/Ponto/Morte` disparam as "vozes".

**Por que sintetizar o áudio?** Deixa o projeto **autocontido** (sem `.wav`), **livre
de direitos autorais**, e a estética 8-bit combina com o jogo.

---

## 13. HUD e fontes (3 sistemas de texto)

Tudo em projeção **ortográfica** (`desenharHUD`, [main.cpp:1418](main.cpp)):
1. **Bitmap do GLUT** (`glutBitmapCharacter`): placar simples durante o jogo.
2. **Fonte vetorial (stroke)** do GLUT: texto grande escalável (ainda no código).
3. **TrueType (Pixelify Sans)** rasterizada em **atlas de textura** via stb_truetype:
   título e "GAME OVER" com **gradiente** e **contorno** (`desenharTTF*`,
   [main.cpp:1222](main.cpp)). Preenchido e bonito.

---

## 14. Estados do jogo e dificuldade

- **Máquina de estados do jogo**: `INICIO → JOGANDO → GAMEOVER` (`g_estado`).
  `display` e `idle` checam o estado; `acaoPrincipal` decide a ação de espaço/clique.
- **Modos de dificuldade** (teclas **1/2/3** na tela inicial):
  `alturaBrechaAtual`/`velocidadeCanoAtual` ([main.cpp:814](main.cpp)) ligam/desligam
  o **encolhimento da brecha** (piso **2.1**) e o **aumento de velocidade** conforme o
  modo (Fácil = nada muda; Médio = brecha encolhe; Difícil = brecha encolhe + acelera).

---

## 15. 🔧 Cola rápida — "para mudar X, mexa em Y"

| Quero mudar... | Onde |
|---|---|
| Velocidade dos canos | `VELOCIDADE_CANO` ([main.cpp:61](main.cpp)) |
| Tamanho da brecha / piso mínimo | `ALTURA_BRECHA` ([main.cpp:64](main.cpp)) / `alturaBrechaAtual` ([main.cpp:822](main.cpp)) |
| Gravidade e força do pulo | `GRAVIDADE`, `IMPULSO_PULO` ([main.cpp:55](main.cpp)) |
| Onde/como a abelha voa | `ABELHA_X`, `ABELHA_IMPULSO` ([main.cpp:71](main.cpp)) |
| Cor das asas | `g_asas.corR/G/B` ([main.cpp:1740](main.cpp)) |
| Direção/cor do Sol | `dirSol`, `difusaSol`, `ambienteSol` ([main.cpp:1657](main.cpp)) |
| Intensidade do brilho | `GL_SHININESS` / `specMat` ([main.cpp:1679](main.cpp)) |
| Cor do céu | `desenharCeu` ([main.cpp:594](main.cpp)) |
| Aparência da grama | `criarTexturaGrama` ([main.cpp:349](main.cpp)) |
| Zoom/ângulo da câmera | `CAMERA_Z` ([main.cpp:48](main.cpp)), `gluPerspective` ([main.cpp:1549](main.cpp)) |
| Quantidade de grama/árvores | `NUM_GRAMAS`, `NUM_ARVORES` ([main.cpp:113](main.cpp)) |
| Força do parallax | `ARVORE_PARALLAX` ([main.cpp:131](main.cpp)) |
| Volume do áudio | `g_volumeAudio` ([main.cpp:198](main.cpp)) |
| Batida de asa (força/flutter) | `AMPLITUDE_BATIDA`, `AMP_IDLE`, `VEL_IDLE` ([main.cpp:74](main.cpp)) |
| Tamanho do "GAME OVER" | escala em `desenharTTFcentralizado` ([main.cpp:1399](main.cpp)) |
| Tamanho da janela | `LARGURA_JANELA`, `ALTURA_JANELA` ([main.cpp:39](main.cpp)) |

---

## 16. Decisões de projeto (o clássico "por que X e não Y")

- **Immediate mode (`glBegin/glEnd`) e não shaders/VBOs modernos** → o pipeline de
  função fixa mostra **explicitamente** transformações, iluminação e textura, que é o
  que a disciplina cobre. Menos performático, mas compensamos com **display lists**.
- **Assimp** em vez de escrever um leitor de `.obj` → economiza tempo e já triangula/
  gera normais.
- **AABB** em vez de esferas para os canos → encaixe exato e barato em retângulos.
- **Luz direcional** em vez de pontual → mundo aberto que rola sob o Sol.
- **Áudio sintetizado** em vez de arquivos → autocontido e sem copyright.
- **Textura procedural** do chão → sem arquivo, com tiling e estilo pixelado.
- **Capivara com X fixo** (mundo rola) → é a mecânica do Flappy; simplifica física e
  colisão.
- **IA reativa** em vez de FSM/A\* → o problema é 1D com alvo conhecido.

---

## 17. 20+ perguntas prováveis do professor (com respostas)

1. **Como criam a sensação 3D com jogabilidade 2D?** Projeção em perspectiva + câmera
   afastada em Z; jogo no plano XY; cenário em camadas de profundidade (Z) + parallax.
2. **Que projeção usam e por quê?** Perspectiva (`gluPerspective` 45°) — dá
   profundidade. Ortográfica só no HUD e no céu.
3. **Que tipo de luz? Como configuraram?** Uma luz **direcional** (Sol, 4º
   componente 0), com **difusa + ambiente + especular** e `GL_SHININESS`.
   Sombreamento **Gouraud** (normais por vértice). Ver [main.cpp:1643](main.cpp).
4. **Diferença entre luz pontual e direcional?** Posição com w=1 é um **ponto** (com
   queda de intensidade); com **w=0** é **direção** (raios paralelos). Usamos
   direcional pra simular o Sol num mundo que rola.
5. **O que é back-face culling e por que usam?** Descarta faces traseiras pela ordem
   dos vértices (winding); economiza. Tivemos que acertar o winding do chão e
   **desligá-lo no HUD**.
6. **Como garantem a ordem de profundidade?** Z-buffer (`GL_DEPTH_TEST`), limpo a
   cada quadro.
7. **Como carregam modelos? Que formatos?** Assimp — `.obj` (capivara, asas, abelha,
   grama, árvores) e `.glb` (cano). Triangula e gera normais.
8. **O que é a bounding box no código?** A caixa que envolve os vértices; usamos o
   **centro** para centralizar e a **maior dimensão** para escalar tudo a um tamanho
   padrão.
9. **O que são display lists e por que usaram?** Geometria compilada 1× na GPU;
   acelera desenhar muitas cópias (grama/árvores).
10. **Como texturizam a capivara? E o chão?** Capivara: PNG (stb_image) por UV.
    Chão: textura **procedural** (código), `GL_NEAREST` + `GL_REPEAT` (tiling).
11. **Por que `GL_NEAREST` no chão?** Mantém o pixelado; `GL_LINEAR` borraria.
12. **Como é a física?** Euler semi-implícito com `dt` real; pulo = impulso de
    velocidade. Independente de FPS.
13. **Como detectam colisão? Que volume envolvente?** **AABB** (caixas) capivara×cano
    e chão. Função pura testada.
14. **Por que AABB e não esferas?** Cano é retângulo → caixa encaixa e o teste é
    exato/barato. (Temos a distância de esferas implementada/testada.)
15. **Onde está a IA e como funciona?** A abelha aliada (`atualizarAliada`): acha o
    próximo cano, mira no centro da brecha e aplica gravidade + batida automática. IA
    **reativa baseada em regras** — ela joga o jogo.
16. **Por que não usaram A\*/FSM?** Espaço 1D e alvo conhecido → regra reativa é ótima
    e robusta; pathfinding seria desnecessário.
17. **Como o cano não distorce a borda?** 2 malhas (corpo/borda); esticamos só o
    **corpo** (cilindro liso) e desenhamos a **borda com escala uniforme**.
18. **Como criam a profundidade do cenário?** Camadas: chão, grama em fileiras de Z,
    árvores em **parallax**, céu em degradê. Tudo com reciclagem.
19. **O que é parallax e onde aparece?** Fundo mais lento que a frente → distância.
    Árvores a 40% da velocidade dos canos.
20. **Como funciona a pontuação?** Quando o cano passa do X da capivara (não contado),
    `pontuacao++` + som + partículas; o cano recicla.
21. **O áudio usa arquivos?** Não — **sintetizado** em tempo real (onda quadrada +
    ruído + envelope) num callback do miniaudio.
22. **Como desenham o texto?** 3 sistemas: bitmap do GLUT, stroke, e **TrueType em
    atlas** (stb_truetype) com gradiente/contorno. HUD em ortográfica.
23. **O que acontece na morte?** `morrer()`: estado→GAMEOVER, screen shake, flash,
    explosão de partículas, som.
24. **Como funcionam os modos de dificuldade?** Teclas 1/2/3; funções da pontuação
    ligam/desligam o encolhimento da brecha (piso 2.1) e o aumento de velocidade.
25. **Por que double buffering?** Desenha num buffer escondido e troca de uma vez
    (`glutSwapBuffers`), evitando "piscar" (tearing/flicker).
26. **O que é `GL_NORMALIZE` e por que precisam?** Como escalamos os modelos, as
    normais mudam de tamanho; `GL_NORMALIZE` as renormaliza pra a luz ficar correta.

---

## 18. Mapa dos requisitos da disciplina → código

| Requisito | Onde |
|---|---|
| Jogo 3D, jogabilidade ≥ 2D (GLUT) | `main`/`display` (plano XY, perspectiva) |
| **Colisão** | `sobreposicaoAABB` ([main.cpp:641](main.cpp)), `verificarColisoesCanos` ([main.cpp:1001](main.cpp)) |
| **IA** | `atualizarAliada` ([main.cpp:1030](main.cpp)) |
| **Iluminação** | `inicializarOpenGL` ([main.cpp:1643](main.cpp)) |
| **Sombreamento** | normais suaves + Gouraud ([main.cpp:308](main.cpp)) |
| **Textura** | `carregarTextura` ([main.cpp:323](main.cpp)), `criarTexturaGrama` ([main.cpp:349](main.cpp)) |
| **Visibilidade** | z-buffer ([main.cpp:1649](main.cpp)) + culling ([main.cpp:1686](main.cpp)) |
| **Modelos 3D** | Assimp `carregarModelo` ([main.cpp:304](main.cpp)) |
| **Áudio (bônus)** | `audioCallback` ([main.cpp:203](main.cpp)) |

> ⚠️ Os números de linha acima e ao longo do guia são **aproximados** e podem
> estar deslocados após edições; use os **nomes das funções** como âncora.

---

## 19. Compilar & dependências

**Plataforma:** macOS (usamos os frameworks **OpenGL** e **GLUT** da Apple).

**Dependências:**
- **Xcode Command Line Tools** — fornecem o compilador (`clang`) e os frameworks
  `OpenGL`/`GLUT`. Instalar: `xcode-select --install`.
- **Assimp** (carrega os modelos) — via Homebrew: `brew install assimp`. Fica em
  `/opt/homebrew/include` (headers) e `/opt/homebrew/lib` (biblioteca).
- **stb_image, stb_truetype, miniaudio** — **não precisam instalar**: são
  *single-header libraries* que já acompanham o projeto (`.h` na raiz). O `main.cpp`
  "ativa" o código de cada uma com um `#define ..._IMPLEMENTATION` antes do `#include`.

**Como compilamos (`build.sh`):**
- Usamos **`/usr/bin/clang++`** (o clang **da Apple**), não um `clang++` genérico.
  *Por quê?* Se houver um `clang++` do Homebrew (LLVM) no `PATH`, ele **não acha o SDK
  do macOS** e falha com `'GLUT/glut.h' file not found`. O clang da Apple acha o SDK
  sozinho.
- Flags principais:
  - `-I/opt/homebrew/include -L/opt/homebrew/lib -lassimp` → achar e **linkar** o Assimp.
  - `-framework OpenGL -framework GLUT` → os frameworks gráficos do macOS.
  - `-framework CoreFoundation -framework CoreAudio -framework AudioToolbox -lpthread`
    → exigidos pelo **miniaudio** (áudio do sistema + threads).
  - `-std=c++17` e `-Wno-deprecated-declarations` (silencia avisos de APIs antigas).

**Comandos:**
```bash
./build.sh                  # compila e abre o jogo
./build.sh compilar         # só compila (sem abrir a janela)
./flappy_capivara --testes  # roda os testes das funções de colisão
```

**Perguntas prováveis:** *"Como vocês compilam? Que bibliotecas precisam?"* → Assimp via
Homebrew; OpenGL/GLUT são frameworks do macOS; stb/miniaudio já vêm no projeto.
*"Por que esses `-framework`?"* → OpenGL/GLUT para render e janela; Core/Audio para o
miniaudio.

---

## 20. Assets e pipeline (de onde vem cada coisa)

**De onde vêm os assets:**

| Asset | Origem | Formato |
|---|---|---|
| Capivara (+ textura) | poly.pizza (licença livre) | `.obj` + `.png` |
| Abelha, asa, grama, 2 árvores | poly.pizza (licença livre) | `.obj` |
| **Cano** | **modelado por nós no Blender** | `.glb` (2 malhas: corpo + borda) |
| Fonte do HUD | Pixelify Sans (Google Fonts) | `.ttf` |
| Textura do chão | **gerada por código** (procedural) | — |
| Efeitos sonoros | **sintetizados por código** (8-bit) | — |

> Contra suspeita de plágio: os modelos são de **licença livre** (poly.pizza) e o
> **cano nós mesmos modelamos** — o arquivo `blender/cano.blend` é a prova.

**Como cada asset entra no jogo** (tudo carregado no `main()`):
- **Modelos**: `carregarModelo` (Assimp lê `.obj`/`.glb`) → `calcularBoundingBox`
  (centraliza e escala para um tamanho padrão) → a geometria vira **display list** na
  primeira vez que é desenhada.
- **Textura da capivara**: `carregarTextura` (stb_image lê o PNG).
- **Textura do chão**: `criarTexturaGrama` (gera os pixels no código).
- **Fonte**: `carregarFonte` (stb_truetype rasteriza a `.ttf` num atlas de textura).
- **Áudio**: `iniciarAudio` (miniaudio) + `audioCallback` (sintetiza as ondas).
- **Cano**: `prepararCano` separa as 2 malhas (corpo/borda) para esticar só o corpo.

**Por que `.obj` e `.glb`?** `.obj` é texto simples (bom para os modelos prontos). O
**cano** saiu do Blender em `.glb` (glTF binário) porque ele guarda **2 malhas +
materiais** num arquivo só — e o Assimp lê os dois formatos com a mesma função.

---

## 21. Perguntas avançadas — banca exigente (com respostas)

Perguntas mais profundas que um professor rigoroso de CG costuma fazer. Se você
domina estas, domina o projeto.

### Pipeline e transformações

**Descreva o caminho de um vértice até a tela.**
Modelo → mundo (`glTranslate/Rotate/Scale`) → visão (`gluLookAt`) → projeção
(`gluPerspective`) → recorte no *frustum* → divisão perspectiva (por *w*) → viewport
(`glViewport`, NDC → pixels). No OpenGL fixo, mundo e visão vivem juntos na matriz
**MODELVIEW**; a projeção na **PROJECTION**.

**Por que `glLoadIdentity` antes do `gluLookAt` a cada quadro?**
Para zerar a MODELVIEW e não acumular a câmera do quadro anterior.

**Diferença entre GL_PROJECTION e GL_MODELVIEW? O que cada função afeta?**
PROJECTION = como o 3D vira 2D (perspectiva/orto). MODELVIEW = posição/orientação de
objetos e câmera. `gluPerspective` mexe na PROJECTION; `gluLookAt`/`glTranslate` na
MODELVIEW.

**Explique a ordem das transformações das asas (`translate → rotate → scale`).**
As matrizes são **pós-multiplicadas**: a última chamada é a **primeira** aplicada ao
vértice. Lê-se de baixo para cima — primeiro escala/centraliza o modelo, depois gira
(levanta/bate), depois translada para as costas da capivara. Inverter a ordem muda o
resultado (girar antes de transladar ≠ depois).

**O que significa o *w* na posição da luz (0 vs 1)?** Coordenada homogênea: `w=1` é um
**ponto** (posição, com atenuação); `w=0` é um ponto no infinito → uma **direção**
(raios paralelos). Usamos `w=0` (Sol).

### Iluminação (a fundo)

**Qual a equação de iluminação? Que vetores entram?**
`cor = ambiente + difusa·max(N·L,0) + especular·max(N·H,0)^shininess`. **N**=normal,
**L**=direção à luz, **V**=direção ao olho, **H**=(L+V) normalizado. O OpenGL fixo usa
**Blinn-Phong** (vetor H).

**Gouraud, Phong e flat — qual vocês usam?**
**Gouraud**: iluminação calculada **por vértice** e interpolada entre eles. *Flat* =
uma cor por face; *Phong* = por **pixel** (exige shaders — não temos). Limitação do
Gouraud: em malhas pouco densas o brilho especular pode "sumir" entre vértices.

**Por que reposicionam a luz dentro do `display`, depois do `gluLookAt`?**
`glLightfv(GL_POSITION)` transforma a luz pela MODELVIEW **atual**. Setando **depois**
da câmera, a direção do Sol fica fixa no mundo (não gira com a câmera).

**Para que serve `GL_NORMALIZE`?** Renormaliza as normais após a escala. Sem ele,
escalar o modelo deixa as normais com módulo ≠ 1 → iluminação clara/escura demais.

**Tem sombra? Por quê?** Não. Sombra exigiria *shadow mapping*/*volumes* (fora do
escopo). Suavizamos com a luz ambiente. É trabalho futuro.

### Visibilidade (a fundo)

**Como o z-buffer decide o que aparece? O que é z-fighting?**
Cada fragmento tem profundidade; guarda-se a menor por pixel (`GL_LESS`). *Z-fighting*
= superfícies quase coplanares "brigam" pela profundidade e piscam, por falta de
precisão (piora com *near* muito pequeno).

**Culling: como o OpenGL sabe qual é a face de trás?** Pela **ordem dos vértices**
projetados (*winding*); por padrão anti-horário (CCW) = frente. Por isso o **chão**
sumia (winding invertido) e por isso **desligamos o culling no HUD** (quads horários).

**O que é o *frustum*? O que acontece fora dele?** É o volume visível (pirâmide
truncada) definido por FOV, aspecto, *near* e *far*. O que está fora é **recortado**
(*clipping*); antes do *near* ou além do *far* some.

### Textura e blending

**O que são UVs? E UV > 1?** Mapeiam cada vértice a um ponto (s,t) da textura. Com
`GL_REPEAT`, UV > 1 **repete** (tiling) — usamos no chão (TexCoord até 12/8).

**NEAREST vs LINEAR? Usam mipmaps?** NEAREST pega o texel mais próximo (pixelado);
LINEAR interpola (suave). **Não** usamos mipmaps (texturas pequenas + estilo pixel;
mipmap suavizaria e custaria à toa).

**Explique o blending das partículas (`GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA`).**
`final = α·fonte + (1−α)·fundo` (transparência). Por isso **desligamos o DEPTH_TEST**
ao desenhar partículas/HUD e as desenhamos **por último**.

**Os "billboards" são de verdade?** São quads no plano XY. Como a câmera olha ao longo
de **−Z**, eles encaram a tela. Se a câmera girasse, **não** acompanhariam (não
recalculamos pela câmera) — billboard "fixo", suficiente porque a câmera é fixa.

### Colisão e IA (crítico)

**A colisão AABB é exata? O cano é redondo, a caixa é quadrada.** É uma
**aproximação**: o cano vira uma caixa de largura `LARGURA_CANO` com o cilindro
inscrito (folga pequena nos cantos). É proposital (perdoa o jogador) e barato.

**Limitações do AABB? E se algo girasse?** AABB só serve para caixas alinhadas aos
eixos; objeto rotacionado exigiria **OBB**/SAT. Aqui nada gira no plano de colisão.

**Pode haver *tunneling* (atravessar o cano num quadro)?** Em tese, com passo grande.
Mitigamos com `dt` limitado a **0.05** e velocidades moderadas.

**A IA é "de verdade"? Como classifica? Pode falhar?** É um **agente reativo baseado em
regras** (*steering*/piloto automático): percebe (a próxima brecha) e age (bate asa
para mirar no centro). Não é busca nem aprendizado. É robusta porque o alvo é
conhecido; só falharia se as brechas mudassem bruscamente entre quadros (não acontece).

### Desempenho e engenharia

**Por que multiplicar por `dt`? E sem isso?** Deixa o movimento **independente do
FPS**. Sem `dt`, quem roda a 120 fps anda o dobro de quem roda a 60.

**Por que limitar `dt` a 0.05?** Se a janela travar, um `dt` gigante daria um "salto"
(e tunneling). O teto evita isso.

**Euler semi-implícito vs explícito; por que não RK4?** Semi-implícito (atualiza
velocidade e depois posição) é estável e simples; RK4 seria precisão desnecessária
para um pulo.

**Immediate mode e display lists são obsoletos? O que o OpenGL moderno faria?** Sim, o
pipeline de função fixa é **legado**; o moderno usa **VBO/VAO + shaders (GLSL)** e você
escreve a iluminação. Escolhemos o fixo por ser didático e o exigido; *display lists*
compensam a performance ao repetir geometria.

**Onde está o maior custo por quadro?** Na **vegetação**: grama (14×5) e árvores (8),
dezenas de instâncias. Por isso usamos *display lists* (geometria já na GPU) e culling.

**Por que *object pooling* nos canos/grama/árvores?** Reaproveitar os mesmos objetos
evita alocar/liberar memória a cada quadro (sem `new`/`delete` no loop).

### Áudio

**O callback de áudio roda em outra thread — há risco de corrida?** Sim, tecnicamente
(`dispararVoz` no main × `audioCallback` no áudio). Usamos uma abordagem simples (achar
a 1ª voz inativa e preencher), sem *mutex*; os efeitos são curtos e o risco é
desprezível no escopo. Uma versão robusta usaria um buffer *lock-free*.

**Onda quadrada não causa *aliasing*?** Causa (harmônicos acima de Nyquist), mas é
exatamente o "som 8-bit" que queremos; não filtramos de propósito.

### Honestidade (impressiona a banca)

**Qual a maior limitação/aproximação do projeto?** Ser capaz de citar: colisão
caixa×cano-redondo (aproximada); *billboards* fixos (câmera fixa); sem sombras; áudio
sem *mutex*; pipeline de função fixa (legado). Reconhecer os *trade-offs* mostra
domínio maior do que fingir que está tudo perfeito.
