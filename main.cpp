// ============================================================
//  Flappy Capivara — Jogo de Computação Gráfica
//  Fase 1: Janela GLUT, câmera 3D e chão
// ============================================================

// Inclui as bibliotecas do OpenGL e GLUT para macOS
#include <GLUT/glut.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <cstdlib>   // exit()
#include <vector>    // std::vector para guardar os vértices
#include <cstdio>    // printf para mensagens no terminal
#include <cmath>     // sinf() para a animação de bater as asas
#include <cassert>   // assert() nos auto-testes das funções puras
#include <string>    // std::string para ler o argumento --testes

// Biblioteca Assimp — carrega o modelo 3D (.obj) da capivara
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// stb_image — carrega a imagem PNG da textura (1 arquivo, domínio público).
// STB_IMAGE_IMPLEMENTATION faz o header incluir o código de fato.
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// ============================================================
//  Configurações da janela
// ============================================================
const int LARGURA_JANELA  = 800;
const int ALTURA_JANELA   = 600;
const char* TITULO_JANELA = "Flappy Capivara";

// ============================================================
//  Configuração da câmera
//  O jogo tem gameplay 2D no plano XY, mas a câmera fica
//  levemente afastada no eixo Z para dar sensação de 3D.
// ============================================================
const float CAMERA_X = 0.0f;   // olha para o centro da cena
const float CAMERA_Y = 2.0f;   // altura dos olhos
const float CAMERA_Z = 10.0f;  // distância para a tela do jogo

// ============================================================
//  Posição da capivara no mundo (plano XY — gameplay 2D).
//  Por enquanto fica fixa; na Fase 3 ela vai cair/pular.
// ============================================================
const float CAPIVARA_X = -2.0f;   // um pouco à esquerda (X fica fixo)

// ============================================================
//  PARÂMETROS DO JOGO (ajuste fino rodando o jogo)
// ============================================================
const float GRAVIDADE         = -15.0f;  // unidades/s² (puxa para baixo)
const float IMPULSO_PULO       =   6.0f;  // velocidade p/ cima ao pular
const float ALTURA_TETO        =   6.0f;  // capivara não passa disso
const float CAPIVARA_Y_INICIAL =   3.0f;  // altura no começo

const int   NUM_CANOS          =   4;     // pares reutilizados
const float VELOCIDADE_CANO    =   3.0f;  // unidades/s p/ a esquerda
const float ESPACO_CANOS       =   5.0f;  // distância entre pares
const float LARGURA_CANO       =   1.2f;  // espessura do cano
const float ALTURA_BRECHA      =   2.6f;  // tamanho da abertura
const float CANO_X_INICIAL     =   7.0f;  // x do primeiro cano
const float CANO_X_RECICLA     =  -8.0f;  // sai de cena à esquerda

const float RAIO_CAPIVARA      =   0.7f;  // p/ colisão (esfera e meia-AABB)
const float RAIO_INIMIGO       =   0.4f;
const float RAIO_PERCEPCAO     =   3.0f;  // distância p/ começar a perseguir
const float VEL_PERSEGUICAO    =   2.2f;  // unidades/s ao perseguir
const float VEL_VAGUEIO        =   1.2f;  // unidades/s ao vagar
const float INTERVALO_SORTEIO  =   1.5f;  // s entre sorteios de direção

const float DURACAO_BATIDA     =   0.25f; // s de uma batida de asa
const float AMPLITUDE_BATIDA   =  35.0f;  // graus de abertura da batida

// ============================================================
//  STRUCT MODELO — guarda tudo que precisamos de um modelo 3D.
//  Usamos a MESMA struct e as MESMAS funções para a capivara,
//  as asas e (depois) as árvores. Isso evita repetir código.
// ============================================================
struct Modelo {
    const aiScene* cena = nullptr;   // dados 3D carregados pela Assimp
    GLuint textura = 0;              // textura (0 = não tem, usa cor)
    float corR = 1, corG = 1, corB = 1;  // cor quando não há textura
    // Centro e escala calculados do "bounding box" para normalizar:
    float centroX = 0, centroY = 0, centroZ = 0;
    float escala = 1.0f;
};

// Os modelos do jogo
Modelo g_capivara;
Modelo g_asas;

// Caminhos dos arquivos (pasta models3d/)
const char* OBJ_CAPIVARA = "models3d/Capybara/Capybara.obj";
const char* TEX_CAPIVARA = "models3d/Capybara/Capybara_BaseColor.png";
const char* OBJ_ASAS     = "models3d/wings/wings.obj";

// ============================================================
//  ESTADO GLOBAL DO JOGO
// ============================================================
enum EstadoJogo { INICIO, JOGANDO, GAMEOVER };
EstadoJogo g_estado = INICIO;

float g_capivaraY       = CAPIVARA_Y_INICIAL;
float g_velocidadeY     = 0.0f;
float g_tempoUltimoPulo = -10.0f;   // bem no passado => asas em repouso
int   g_pontuacao       = 0;

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

float g_tempoAnterior = 0.0f;       // p/ calcular dt no idle

// ------------------------------------------------------------
//  Calcula o centro e a escala de UM modelo a partir da caixa
//  que envolve todos os seus vértices (bounding box).
//  tamanhoAlvo = quantas unidades a maior dimensão deve ocupar.
// ------------------------------------------------------------
void calcularBoundingBox(Modelo& mod, float tamanhoAlvo) {
    float minX =  1e9, minY =  1e9, minZ =  1e9;
    float maxX = -1e9, maxY = -1e9, maxZ = -1e9;

    for (unsigned int m = 0; m < mod.cena->mNumMeshes; m++) {
        const aiMesh* malha = mod.cena->mMeshes[m];
        for (unsigned int v = 0; v < malha->mNumVertices; v++) {
            aiVector3D p = malha->mVertices[v];
            if (p.x < minX) minX = p.x;  if (p.x > maxX) maxX = p.x;
            if (p.y < minY) minY = p.y;  if (p.y > maxY) maxY = p.y;
            if (p.z < minZ) minZ = p.z;  if (p.z > maxZ) maxZ = p.z;
        }
    }

    mod.centroX = (minX + maxX) / 2.0f;
    mod.centroY = (minY + maxY) / 2.0f;
    mod.centroZ = (minZ + maxZ) / 2.0f;

    float maior = maxX - minX;
    if (maxY - minY > maior) maior = maxY - minY;
    if (maxZ - minZ > maior) maior = maxZ - minZ;
    if (maior > 0) mod.escala = tamanhoAlvo / maior;
}

// ------------------------------------------------------------
//  Carrega UM modelo .obj do disco para a struct Modelo.
//  Retorna true se deu certo.
// ------------------------------------------------------------
bool carregarModelo(Modelo& mod, const char* caminhoObj, float tamanhoAlvo) {
    // Triangulate: vira tudo triângulo. GenSmoothNormals: cria normais.
    mod.cena = aiImportFile(caminhoObj,
                            aiProcess_Triangulate |
                            aiProcess_GenSmoothNormals);

    if (!mod.cena || mod.cena->mNumMeshes == 0) {
        printf("ERRO: nao consegui carregar '%s'\n", caminhoObj);
        return false;
    }

    calcularBoundingBox(mod, tamanhoAlvo);
    printf("Modelo '%s' carregado: %u malha(s).\n",
           caminhoObj, mod.cena->mNumMeshes);
    return true;
}

// ------------------------------------------------------------
//  Carrega uma imagem PNG como textura para um Modelo.
//  Precisa ser chamada DEPOIS de criar a janela (contexto GL).
// ------------------------------------------------------------
void carregarTextura(Modelo& mod, const char* caminho) {
    int largura, altura, canais;

    // OpenGL espera a imagem de baixo p/ cima; invertemos na vertical.
    stbi_set_flip_vertically_on_load(true);

    unsigned char* dados = stbi_load(caminho, &largura, &altura, &canais, 3);
    if (!dados) {
        printf("AVISO: nao consegui carregar a textura '%s'.\n", caminho);
        return;
    }

    glGenTextures(1, &mod.textura);
    glBindTexture(GL_TEXTURE_2D, mod.textura);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, largura, altura, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, dados);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(dados);
    printf("Textura '%s' carregada: %dx%d.\n", caminho, largura, altura);
}

// ------------------------------------------------------------
//  Desenha UM modelo já normalizado (centralizado e escalado).
//  Quem chama é responsável por posicionar/rotacionar antes
//  (glTranslatef / glRotatef no mundo).
// ------------------------------------------------------------
void desenharModelo(const Modelo& mod) {
    if (!mod.cena) return;

    glPushMatrix();
        // Aplica a escala e centraliza o modelo na origem
        glScalef(mod.escala, mod.escala, mod.escala);
        glTranslatef(-mod.centroX, -mod.centroY, -mod.centroZ);

        // Com textura: cor branca para mostrar as cores da imagem.
        // Sem textura: usa a cor definida na struct.
        if (mod.textura != 0) {
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, mod.textura);
            glColor3f(1.0f, 1.0f, 1.0f);
        } else {
            glColor3f(mod.corR, mod.corG, mod.corB);
        }

        // Percorre cada malha e desenha seus triângulos
        for (unsigned int m = 0; m < mod.cena->mNumMeshes; m++) {
            const aiMesh* malha = mod.cena->mMeshes[m];
            bool temUV = malha->HasTextureCoords(0);

            glBegin(GL_TRIANGLES);
            for (unsigned int f = 0; f < malha->mNumFaces; f++) {
                const aiFace& face = malha->mFaces[f];
                for (unsigned int i = 0; i < face.mNumIndices; i++) {
                    unsigned int idx = face.mIndices[i];

                    if (malha->HasNormals()) {
                        aiVector3D n = malha->mNormals[idx];
                        glNormal3f(n.x, n.y, n.z);
                    }
                    if (temUV) {
                        aiVector3D uv = malha->mTextureCoords[0][idx];
                        glTexCoord2f(uv.x, uv.y);
                    }
                    aiVector3D p = malha->mVertices[idx];
                    glVertex3f(p.x, p.y, p.z);
                }
            }
            glEnd();
        }

        glDisable(GL_TEXTURE_2D);
    glPopMatrix();
}

// ============================================================
//  FUNÇÕES PURAS DE COLISÃO (sem estado, fáceis de testar)
// ============================================================

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

// Auto-testes das funções puras. Roda com: ./flappy_capivara --testes
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

// ============================================================
//  Ajustes das ASAS (fáceis de mexer enquanto encaixamos).
//  Posição relativa à capivara, rotação e tamanho.
// ============================================================
const float ASA_DX     =  0.2f;  // desloca p/ trás (-) ou frente/ombro (+)
const float ASA_DY     =  0.5f;  // altura sobre as costas
const float ASA_DZ     =  0.0f;  // profundidade
const float ASA_ROT_Y  =  90.0f; // gira para alinhar com a capivara
const float ASA_ROT_Z  = -20.0f; // inclina em diagonal (ponta p/ cima e trás)
const float ASA_TAM    =  1.6f;  // tamanho-alvo das asas (menor)

// ============================================================
//  Desenha a capivara no mundo (posição + giro de perfil).
// ============================================================
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

// ============================================================
//  Desenha as asas (modelo .obj) sobre as costas da capivara,
//  com animação de bater usando seno do tempo.
// ============================================================
void desenharAsas() {
    // Ângulo do bater de asas: vai e volta suavemente com o tempo.
    // GLUT_ELAPSED_TIME = milissegundos desde o início do programa.
    float tempo = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;  // segundos
    float anguloBater = sinf(tempo * 7.0f) * 25.0f;       // ±25 graus

    glPushMatrix();
        // Posiciona as asas sobre a capivara
        glTranslatef(CAPIVARA_X + ASA_DX, g_capivaraY + ASA_DY, ASA_DZ);

        // IMPORTANTE: o bater vem PRIMEIRO no código para ser aplicado
        // por ÚLTIMO aos vértices => gira no eixo X do MUNDO, ou seja,
        // as asas sobem e descem na tela (e não para os lados).
        glRotatef(anguloBater, 1.0f, 0.0f, 0.0f);

        // Depois orientamos o modelo das asas para encaixar na capivara
        glRotatef(ASA_ROT_Y, 0.0f, 1.0f, 0.0f);  // alinha com a capivara
        glRotatef(ASA_ROT_Z, 0.0f, 0.0f, 1.0f);  // inclina em diagonal

        desenharModelo(g_asas);
    glPopMatrix();
}

// ============================================================
//  CANOS (obstáculos) — primitivas glutSolidCube
// ============================================================

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
        desenharBlocoCano(g_canos[i].x, 0.0f, c - meia);            // cano de baixo
        desenharBlocoCano(g_canos[i].x, c + meia, ALTURA_TETO + 2); // cano de cima
    }
}

// Colisão da capivara (caixa AABB) com os canos e o chão.
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

// ============================================================
//  INIMIGO com IA (Máquina de Estados Finitos: vagar/perseguir)
// ============================================================
void inicializarInimigo() {
    g_inimigo.x = 4.0f;
    g_inimigo.y = 3.0f;
    g_inimigo.vx = -VEL_VAGUEIO;
    g_inimigo.vy = 0.0f;
    g_inimigo.estadoIA = VAGANDO;
    g_inimigo.tempoProxSorteio = 0.0f;
}

void atualizarInimigo(float dt) {
    // distância até a capivara (mesma fórmula da colisão por esfera)
    float D = distanciaEsferas(g_inimigo.x, g_inimigo.y, CAPIVARA_X, g_capivaraY);

    // transição de estado (IA reativa)
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
        if (g_inimigo.x < 0.0f) { g_inimigo.x = 0.0f; g_inimigo.vx = -g_inimigo.vx; }
        if (g_inimigo.x > 6.0f) { g_inimigo.x = 6.0f; g_inimigo.vx = -g_inimigo.vx; }
        if (g_inimigo.y < 1.0f) { g_inimigo.y = 1.0f; g_inimigo.vy = -g_inimigo.vy; }
        if (g_inimigo.y > ALTURA_TETO) { g_inimigo.y = ALTURA_TETO; g_inimigo.vy = -g_inimigo.vy; }
    }
}

void verificarColisaoInimigo() {
    float D = distanciaEsferas(g_inimigo.x, g_inimigo.y, CAPIVARA_X, g_capivaraY);
    if (D <= RAIO_INIMIGO + RAIO_CAPIVARA) g_estado = GAMEOVER;
}

void desenharInimigo() {
    glPushMatrix();
        glTranslatef(g_inimigo.x, g_inimigo.y, 0.0f);
        // amarelo vagando, vermelho perseguindo (deixa a IA visível)
        if (g_inimigo.estadoIA == PERSEGUINDO) glColor3f(0.9f, 0.2f, 0.1f);
        else                                   glColor3f(0.95f, 0.85f, 0.1f);
        glutSolidSphere(RAIO_INIMIGO, 16, 16);
    glPopMatrix();
}

// ============================================================
//  HUD — texto 2D (placar e mensagens), por cima da cena
// ============================================================
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

// ============================================================
//  Callback de desenho — chamado toda vez que a janela
//  precisa ser redesenhada (pelo glutPostRedisplay ou evento)
// ============================================================
void display() {
    // Limpa o buffer de cor e o buffer de profundidade (z-buffer)
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Carrega a matriz de modelo/visão e posiciona a câmera
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // gluLookAt(posição da câmera,  ponto que ela mira,  vetor "cima")
    gluLookAt(CAMERA_X, CAMERA_Y, CAMERA_Z,   // posição
              0.0f,     2.0f,     0.0f,         // alvo (mesma altura)
              0.0f,     1.0f,     0.0f);         // vetor up (eixo Y)

    // --------------------------------------------------------
    //  Desenha o chão como um quadrilátero no plano XZ
    //  (servirá de referência visual enquanto o jogo não tem
    //  cenário completo)
    // --------------------------------------------------------
    glColor3f(0.3f, 0.6f, 0.2f);  // cor verde-grama
    glBegin(GL_QUADS);
        glNormal3f(0.0f, 1.0f, 0.0f);  // normal apontando para cima (p/ luz)
        glVertex3f(-10.0f, 0.0f, -5.0f);
        glVertex3f( 10.0f, 0.0f, -5.0f);
        glVertex3f( 10.0f, 0.0f,  5.0f);
        glVertex3f(-10.0f, 0.0f,  5.0f);
    glEnd();

    // Desenha os canos (obstáculos)
    desenharCanos();

    // Desenha a capivara sobre o cenário
    desenharCapivara();

    // Desenha as asas da capivara
    desenharAsas();

    // Desenha o inimigo (esfera com IA)
    desenharInimigo();

    // HUD por último (texto 2D por cima de tudo)
    desenharHUD();

    // Troca os buffers (double buffering evita flickering)
    glutSwapBuffers();
}

// ============================================================
//  Callback de redimensionamento — chamado quando a janela
//  muda de tamanho. Reajusta o viewport e a projeção.
// ============================================================
void reshape(int largura, int altura) {
    // Evita divisão por zero
    if (altura == 0) altura = 1;

    // Define a área de desenho como a janela inteira
    glViewport(0, 0, largura, altura);

    // Configura a projeção perspectiva (sensação de profundidade)
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    // gluPerspective(campo de visão vertical, proporção, near, far)
    // - 45°  de abertura vertical
    // - proporção largura/altura da janela
    // - objetos entre 0.1 e 100 unidades são visíveis
    gluPerspective(45.0, (double)largura / altura, 0.1, 100.0);

    // Volta para a matriz de modelo/visão (padrão para o resto)
    glMatrixMode(GL_MODELVIEW);
}

// ============================================================
//  Callback de teclado — teclas especiais e normais
// ============================================================
// Dá o impulso de pulo (só faz efeito enquanto está jogando).
void pular() {
    if (g_estado == JOGANDO) {
        g_velocidadeY = IMPULSO_PULO;
        g_tempoUltimoPulo = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    }
}

// Reinicia tudo para um novo jogo e começa a jogar.
void reiniciarJogo() {
    g_capivaraY   = CAPIVARA_Y_INICIAL;
    g_velocidadeY = 0.0f;
    g_pontuacao   = 0;
    g_tempoUltimoPulo = -10.0f;

    inicializarCanos();
    inicializarInimigo();

    g_estado = JOGANDO;
    pular();   // primeiro impulso ao começar
}

// Ação de ESPAÇO/clique, conforme o estado atual.
void acaoPrincipal() {
    if      (g_estado == INICIO)   reiniciarJogo();
    else if (g_estado == JOGANDO)  pular();
    else if (g_estado == GAMEOVER) reiniciarJogo();
}

void teclado(unsigned char tecla, int x, int y) {
    if (tecla == 27) exit(0);          // ESC fecha
    if (tecla == ' ') acaoPrincipal();
}

void mouse(int botao, int estadoBotao, int x, int y) {
    if (botao == GLUT_LEFT_BUTTON && estadoBotao == GLUT_DOWN)
        acaoPrincipal();
}

// ============================================================
//  Callback idle — chamado quando não há eventos pendentes.
//  Aqui faremos a atualização da física no futuro.
// ============================================================
void idle() {
    float agora = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    float dt = agora - g_tempoAnterior;
    g_tempoAnterior = agora;
    if (dt > 0.05f) dt = 0.05f;   // evita "pulo" grande se travar

    if (g_estado == JOGANDO) {
        // gravidade + integração da posição
        g_velocidadeY += GRAVIDADE * dt;
        g_capivaraY   += g_velocidadeY * dt;

        atualizarCanos(dt);

        // teto: limita (não mata)
        if (g_capivaraY > ALTURA_TETO) {
            g_capivaraY = ALTURA_TETO;
            g_velocidadeY = 0.0f;
        }

        verificarColisoesCanos();   // canos e chão => game over

        atualizarInimigo(dt);       // IA: vagar / perseguir
        verificarColisaoInimigo();  // encostar no inimigo => game over
    }
    glutPostRedisplay();
}

// ============================================================
//  Inicialização do OpenGL — configurações que ficam fixas
//  durante todo o jogo
// ============================================================
void inicializarOpenGL() {
    // Cor de fundo: azul-céu claro
    glClearColor(0.53f, 0.81f, 0.98f, 1.0f);

    // Habilita o teste de profundidade (z-buffer):
    // objetos mais longe ficam atrás de objetos mais perto
    glEnable(GL_DEPTH_TEST);

    // -------- Iluminação --------
    // Sem luz, o modelo aparece como uma silhueta chapada.
    // Com uma luz, vemos o volume 3D (sombreamento).
    glEnable(GL_LIGHTING);   // liga o cálculo de iluminação
    glEnable(GL_LIGHT0);     // liga a luz número 0

    // Posição da luz (x, y, z, w). w=1 => luz pontual nessa posição.
    GLfloat posicaoLuz[] = { 2.0f, 6.0f, 8.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, posicaoLuz);

    // Faz glColor3f() definir a cor do material (difusa/ambiente),
    // assim continuamos pintando os objetos com glColor normalmente.
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    // Como escalamos o modelo, as normais precisam ser renormalizadas
    // para a iluminação ficar correta.
    glEnable(GL_NORMALIZE);
}

// ============================================================
//  Ponto de entrada do programa
// ============================================================
int main(int argc, char** argv) {
    // Modo de teste: roda os auto-testes das funções puras e sai.
    if (argc > 1 && std::string(argv[1]) == "--testes") {
        rodarTestes();
        return 0;
    }

    // Inicializa o GLUT
    glutInit(&argc, argv);

    // Modo de display:
    //  GLUT_DOUBLE  = double buffering (sem flickering)
    //  GLUT_RGB     = cores RGB
    //  GLUT_DEPTH   = buffer de profundidade (z-buffer)
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);

    // Define tamanho e posição inicial da janela
    glutInitWindowSize(LARGURA_JANELA, ALTURA_JANELA);
    glutInitWindowPosition(100, 100);

    // Cria a janela com o título definido
    glutCreateWindow(TITULO_JANELA);

    // Registra os callbacks (funções chamadas pelo GLUT)
    glutDisplayFunc(display);    // redesenho
    glutReshapeFunc(reshape);    // redimensionamento
    glutKeyboardFunc(teclado);   // teclado
    glutMouseFunc(mouse);        // clique do mouse
    glutIdleFunc(idle);          // loop ocioso

    // Aplica as configurações iniciais do OpenGL
    inicializarOpenGL();

    // Carrega o modelo 3D da capivara e sua textura
    carregarModelo(g_capivara, OBJ_CAPIVARA, 2.8f);
    carregarTextura(g_capivara, TEX_CAPIVARA);

    // Carrega o modelo das asas (sem textura: cor creme)
    carregarModelo(g_asas, OBJ_ASAS, ASA_TAM);
    g_asas.corR = 0.96f; g_asas.corG = 0.95f; g_asas.corB = 0.90f;

    // Inicia o loop principal do GLUT (não retorna daqui)
    glutMainLoop();

    return 0;
}
