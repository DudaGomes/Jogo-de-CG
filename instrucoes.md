# Trabalho Final – Computação Gráfica
### Game 3D em OpenGL

**Professor:** Dr. Laurindo de Sousa Britto Neto
**Instituição:** Universidade Federal do Piauí (UFPI) – Centro de Ciências da Natureza – Departamento de Computação

---

## 1. Descrição Geral

Desenvolva um **Game em 3D (tema livre)** usando **OpenGL**.

1. Pesquise como o jogo escolhido funciona e use-o como base de inspiração. O **requisito mínimo** é desenvolver um **jogo em 3D com jogabilidade em 2D** dentro de uma janela **GLUT**.
2. Use ao máximo sua **criatividade**, incluindo novas características, mecânicas ou jogabilidades inexistentes no jogo original.
3. Implemente:
   - **Detecção de colisões**
   - **Algoritmos de Inteligência Artificial**
   - Recursos do **OpenGL**: **iluminação, sombreamento, textura e visibilidade** (estudados na 3ª unidade do curso).
4. Você pode (e deve) integrar outras bibliotecas complementares, como **áudio**, **modelos 3D**, entre outras que agreguem ao projeto.

---

## 2. Relatório Técnico (Obrigatório)

O grupo deve entregar, junto com o projeto, um **relatório técnico em PDF**, seguindo o **template de artigos da Sociedade Brasileira de Computação (SBC)**.

> 🔗 Baixar o template da SBC no site oficial da sociedade.

**Extensão:** de **2 a 6 páginas**.

### Seções mínimas obrigatórias

| # | Seção | Conteúdo esperado |
|---|-------|--------------------|
| 1 | **Título e autores** | Nomes e matrículas dos integrantes da dupla |
| 2 | **Resumo** | Breve descrição do jogo |
| 3 | **Introdução** | Contexto, tema, regras, motivação do jogo desenvolvido e seus objetivos |
| 4 | **Materiais** | Descrição de ferramentas e bibliotecas utilizadas e suas finalidades |
| 5 | **Métodos** | Principais funcionalidades implementadas: movimentação, colisões, IA, visibilidade, iluminação, sombreamento, texturas etc. |
| 6 | **Resultados e Discussão** | Capturas de tela do jogo em execução e descrição do funcionamento do jogo |
| 7 | **Conclusão** | Principais desafios enfrentados, soluções adotadas e trabalhos futuros |
| 8 | **Referências** | Bibliotecas, tutoriais ou artigos consultados |

---

## 3. Informações Gerais

| Item | Detalhe |
|------|---------|
| **Valor** | 10,0 pontos |
| **Equipe** | Em dupla |
| **Apresentações** | 29/06/2026 e 01/07/2026 |
| **Entrega** | Até **28/06/2026 às 23h59**, via **SIGAA**, em um arquivo **.zip** contendo: <br>• Todos os arquivos do projeto (código-fonte, imagens, modelos 3D, áudio etc.) <br>• O relatório em **PDF** (obrigatório) |

---

## 4. Observações Importantes

- ❌ **Não é necessário** enviar o arquivo executável.
- ⚠️ A detecção de **qualquer tipo de cópia** (entre alunos, da internet, etc.) acarretará **nota zero para todos os envolvidos**.
- ⚠️ A **não entrega do relatório técnico** ou a **não apresentação do trabalho** acarretará **nota zero**.
- ⚠️ A **falta em qualquer um dos dias de apresentação** acarretará **redução na nota final** do trabalho.
- ℹ️ Integrantes de uma mesma dupla **podem receber notas diferentes**, em função:
  - do **desempenho individual na apresentação**;
  - do **desempenho qualitativo** obtido durante a 3ª unidade.
- ⛔ **Trabalhos entregues após o prazo não serão aceitos.**

---

## 5. Ideias para Temas

> Sugestão: **transforme jogos clássicos 2D em jogos 3D**.

- Angry Birds
- Arkanoid
- Asteroids
- Bomberman
- Crossy Road
- Donkey Kong
- Duck Hunt
- Flappy Bird
- Frogger
- Outrun
- Pacman
- Pong
- Seaquest
- Snake
- Sokoban
- Space Invaders
- Tetris
- Tower Bloxx
- Shooter (2D/3D, plataforma ou top‑down)
- Entre outros...

*(O material de apoio traz, para cada jogo acima, exemplos visuais comparando a versão 2D clássica com possíveis reinterpretações em 3D, servindo de inspiração estética e de jogabilidade.)*

---

## 6. Exemplo Guiado de Desenvolvimento (baseado em Pac-Man 3D)

O material de apoio detalha um passo a passo de como estruturar um jogo (usando Pac-Man como exemplo). Esse roteiro pode ser adaptado para qualquer tema escolhido.

### 6.1 Criando o Cenário

**Legenda da matriz do mapa:**

| Código | Significado |
|--------|--------------|
| `0` | Vazio |
| `1` | Parede |
| `2` | Comida |
| `3` | Pastilha de Força |
| `4` | Porta (Fantasma) |
| `V` | Fantasma Vermelho |
| `R` | Fantasma Roxo |
| `A` | Fantasma Azul |
| `L` | Fantasma Laranja |
| `P` | Pac-man |
| `X` | Passagem Secreta |

- O cenário é representado por uma **matriz 21x21**, onde cada célula define o tipo de elemento presente naquela posição.
- **Primitivas do OpenGL/GLUT sugeridas:**
  - **Paredes:** `glutSolidCube();`
  - **Pac-man, Fantasmas, Comidas, Pastilhas:** `glutSolidSphere();` ou `gluSphere();`

### 6.2 Modelando Personagens

- Pode-se usar **primitivas do OpenGL/GLUT** (esferas, cubos, etc.) para representações simples.
- Para personagens mais elaborados, pode-se importar **modelos 3D** prontos:
  - Formato **.OBJ** (mais comum)
  - Outros formatos suportados pela biblioteca escolhida

### 6.3 Detecção de Colisões

Utiliza-se **volumes envolventes** para simplificar o cálculo de colisão entre objetos 3D complexos.

**Tipos de volumes envolventes:**

| Tipo | Descrição |
|------|-----------|
| **Esferas Envolventes** | Definidas por raio e centro. Simples de manipular, mas aproximação fraca (muito espaço vazio). |
| **AABBs** (Axis Aligned Bounding Boxes) | Definidas por dois pontos extremos. Simples de criar/manipular, mas têm problemas com objetos na diagonal. |
| **OBBs** (Object Oriented Bounding Boxes) | Definidas por centro, normais das faces e tamanho de cada eixo. Difíceis de criar/manipular, mas boa aproximação à forma do objeto e bom desempenho em colisões complexas. |
| **Outros** | Cilindros, K-DOP, etc. |

#### Detecção de colisão por esferas (passo a passo)

1. Calcular a distância entre os centros das duas esferas:

   ```
   D = √[ (x1 - x0)² + (y1 - y0)² + (z1 - z0)² ]
   ```

2. Somar os raios das duas esferas → **S**.
3. Comparar:
   - Se **D ≤ S** → **existe colisão**;
   - Caso contrário → **não existe colisão**.

### 6.4 Inteligência Artificial Simples

**Abordagem sugerida:** IA Reativa com **Máquina de Estados Finitos (FSM)** — o personagem percebe o ambiente e altera seu comportamento (muda de estado).

- **Estado 1 — Movimento Randômico:**
  Função randômica sorteia uma direção (Norte, Sul, Leste, Oeste) e o personagem se move naquela direção até encontrar um obstáculo.

- **Estado 2 — Detecção de Proximidade:**
  Função baseada na **detecção de colisão por esferas** verifica se o alvo (ex.: o jogador) está próximo.
  - **Se** detectar proximidade → ativa função de **movimento em direção ao alvo**;
  - **Caso contrário** → mantém o **movimento randômico** (Estado 1).

---

## 7. Checklist Rápido de Entrega

- [ ] Jogo 3D funcional em OpenGL/GLUT (jogabilidade pelo menos em 2D)
- [ ] Detecção de colisões implementada
- [ ] Algoritmo de IA implementado
- [ ] Iluminação, sombreamento, textura e visibilidade aplicados
- [ ] Bibliotecas complementares integradas (áudio, modelos 3D etc.), se aplicável
- [ ] Relatório técnico em PDF (template SBC, 2–6 páginas, todas as seções)
- [ ] Arquivo `.zip` com código-fonte + assets + relatório
- [ ] Envio no SIGAA até **28/06/2026, 23h59**
- [ ] Preparação para apresentação em **29/06/2026** ou **01/07/2026**