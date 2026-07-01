# 🦫 Flappy Capivara

Uma reinterpretação **3D** do clássico **Flappy Bird**, feita em **C++ com OpenGL/GLUT**,
onde o pássaro dá lugar a uma **capivara**. Trabalho final da disciplina de
**Computação Gráfica** — Universidade Federal do Piauí (UFPI).

A jogabilidade é 2D (a capivara só sobe e desce; os canos rolam), mas a cena é
renderizada em 3D, com iluminação, texturas, cenário em profundidade e uma
**abelha aliada controlada por IA** que voa à frente navegando as brechas.

---

## 🎮 Como jogar

| Tecla / Ação | Efeito |
|---|---|
| **Espaço** ou **clique** | Começar / pular / reiniciar |
| **1 / 2 / 3** (na tela inicial) | Dificuldade **Fácil / Médio / Difícil** |
| **ESC** | Sair |

Passe pelo maior número de canos sem bater neles nem no chão. A cada ponto o jogo
fica mais difícil (a brecha encolhe e/ou a velocidade aumenta, conforme o modo).

---

## 🕹️ Recursos (e conceitos de Computação Gráfica aplicados)

- **Renderização 3D** em perspectiva com jogabilidade 2D (plano XY).
- **Iluminação e sombreamento** — luz **direcional** (Sol) com componentes difusa,
  ambiente e especular; sombreamento suave (Gouraud).
- **Texturas** — imagem PNG (capivara) e **textura procedural** gerada em código (chão).
- **Visibilidade** — *z-buffer* + *back-face culling*.
- **Modelos 3D** carregados via Assimp (`.obj` e `.glb`).
- **Detecção de colisão** por caixas envolventes (**AABB**).
- **Inteligência Artificial** — uma abelha aliada em **piloto automático** que joga
  o jogo sozinha, mirando no centro de cada brecha.
- **Cenário com profundidade** — grama em várias fileiras, árvores em *parallax* e
  céu em degradê.
- **Extras** — partículas, *screen shake*, flash, **áudio 8-bit sintetizado** em
  tempo real (sem arquivos de som) e HUD com fonte TrueType.

---

## ⚙️ Como compilar e rodar (macOS)

**Dependências:**
- Xcode Command Line Tools (fornece `clang`, OpenGL e GLUT).
- **Assimp**: `brew install assimp`
- As demais bibliotecas (`stb_image`, `stb_truetype`, `miniaudio`) já acompanham o
  projeto como *headers*.

**Compilar e abrir o jogo:**
```bash
./build.sh
```
**Só compilar (sem abrir a janela):**
```bash
./build.sh compilar
```
> O script usa `/usr/bin/clang++` (clang da Apple), que encontra o SDK do macOS.
> No VS Code, também dá para usar **Cmd + Shift + B**.

**Rodar os testes automáticos das funções de colisão:**
```bash
./flappy_capivara --testes
```

---

## 📁 Estrutura do projeto

```
main.cpp            # todo o jogo (organizado em seções)
build.sh            # compila e roda
stb_image.h         # carregar textura PNG
stb_truetype.h      # rasterizar a fonte do HUD
miniaudio.h         # saída de áudio
models3d/           # modelos 3D (.obj/.glb) e texturas
fonts/              # Pixelify Sans (fonte do título/HUD)
blender/cano.blend  # fonte do cano modelado no Blender
relatorio/          # relatório técnico (template SBC)
GUIA_APRESENTACAO.md # explicação completa do código
```

---

## 🧩 Créditos e bibliotecas

- **Modelos 3D** (capivara, abelha, asa, grama, árvores): [poly.pizza](https://poly.pizza/) (licença livre).
- **Cano**: modelado por nós no [Blender](https://www.blender.org/).
- **Fonte**: [Pixelify Sans](https://fonts.google.com/specimen/Pixelify+Sans) (Google Fonts).
- **Bibliotecas**: [OpenGL](https://www.opengl.org/) · GLUT · [Assimp](https://www.assimp.org/) ·
  [stb](https://github.com/nothings/stb) · [miniaudio](https://miniaud.io/).

---

## 👥 Autores

- **Eduardo Melo de Carvalho**
- **Maria Eduarda Farias Gomes**

Disciplina de Computação Gráfica — UFPI · Prof. Dr. Laurindo de Sousa Britto Neto.
