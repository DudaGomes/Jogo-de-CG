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
#include <assimp/material.h>   // aiGetMaterialColor (cor por material)

// stb_image — carrega a imagem PNG da textura (1 arquivo, domínio público).
// STB_IMAGE_IMPLEMENTATION faz o header incluir o código de fato.
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// stb_truetype — rasteriza uma fonte .ttf de verdade (glifos sólidos).
// Usado para o título arcade "FLAPPY CAPIVARA" com a fonte Pixelify Sans.
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

// miniaudio — toca áudio (header único). Não decodifica arquivos: nós
// SINTETIZAMOS os efeitos 8-bit no código (sem depender de .wav/.mp3).
#define MA_NO_DECODING
#define MA_NO_ENCODING
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

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
const float ALTURA_BRECHA      =   3.0f;  // tamanho da abertura
const float CANO_X_INICIAL     =   7.0f;  // x do primeiro cano
const float CANO_X_RECICLA     =  -8.0f;  // sai de cena à esquerda

const float RAIO_CAPIVARA      =   0.45f; // p/ colisão (esfera e meia-AABB)
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
    bool usarCorMaterial = false;    // true: pinta cada parte com a cor do .mtl
    float desaturar = 0.0f;          // 0=cor original, 1=cinza (neutraliza)
    // Centro e escala calculados do "bounding box" para normalizar:
    float centroX = 0, centroY = 0, centroZ = 0;
    float tamX = 1, tamY = 1, tamZ = 1;  // dimensões brutas (largura/altura/prof.)
    float escala = 1.0f;
    GLuint lista = 0;                // display list (compila a geometria 1x)
};

// Os modelos do jogo
Modelo g_capivara;
Modelo g_asas;
Modelo g_abelha;
Modelo g_grama;
Modelo g_cano;

// Caminhos dos arquivos (pasta models3d/)
const char* OBJ_CAPIVARA = "models3d/Capybara/Capybara.obj";
const char* TEX_CAPIVARA = "models3d/Capybara/Capybara_BaseColor.png";
const char* OBJ_ASAS     = "models3d/Bat wing/bat wing.obj";
const char* OBJ_ABELHA   = "models3d/Bee/model.obj";
const char* OBJ_GRAMA    = "models3d/Grass Patch/model.obj";
const char* GLB_CANO     = "models3d/Pipe_novo.glb";

// Tufos de grama 3D que rolam na base e reciclam ao sair da tela.
// Várias FILEIRAS em profundidades diferentes (z) formam um gramado
// inteiro, dando sensação de profundidade 3D.
const int   NUM_GRAMAS   = 14;     // colunas (ao longo de X)
const float GRAMA_ESPACO = 1.6f;   // distância entre tufos
const float GRAMA_DY     = 0.30f;  // assenta no chão (y=0)
const float GRAMA_TAM    = 2.4f;   // tamanho de cada tufo
const float GRAMA_LIMITE = 12.0f;  // recicla quando passa disso à esquerda
const int   GRAMA_FILEIRAS = 5;
const float GRAMA_Z[GRAMA_FILEIRAS] = { 3.4f, 1.6f, -0.2f, -2.0f, -3.8f };
float g_gramaX[NUM_GRAMAS];

// Árvores ao FUNDO (profundidade). Rolam mais devagar (parallax) e
// reciclam. Alternam dois modelos (Tree e Tree-2).
Modelo g_arvore1, g_arvore2;
const char* OBJ_ARVORE1   = "models3d/Tree/model.obj";
const char* OBJ_ARVORE2   = "models3d/Tree-2/model.obj";
const int   NUM_ARVORES   = 8;
const float ARVORE_ESPACO = 3.2f;
const float ARVORE_TAM    = 2.4f;    // tamanho-base (menor que antes)
const float ARVORE_LIMITE = 14.0f;
const float ARVORE_PARALLAX = 0.4f;  // 40% da velocidade dos canos
// Cada árvore tem posição, modelo, escala e profundidade próprios
// (sorteados) para variar o cenário e reforçar a sensação de 3D.
float g_arvoreX[NUM_ARVORES];
int   g_arvoreTipo[NUM_ARVORES];     // 0 = Tree, 1 = Tree-2
float g_arvoreEscala[NUM_ARVORES];   // fator de tamanho (umas menores, outras maiores)
float g_arvoreZ[NUM_ARVORES];        // profundidade (mais longe = mais atrás)

// Textura procedural da grama (gerada no código, sem arquivo)
GLuint g_texturaGrama = 0;

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

// Game feel: tremor de tela e flash (decaem com o tempo)
float g_shake = 0.0f;
float g_flash = 0.0f;

// ---- Sistema de PARTÍCULAS (poeira, brilho, explosão) ----
struct Particula {
    float x, y, z, vx, vy, vz;
    float vida, vidaMax;     // segundos restantes / iniciais
    float r, g, b, tam;
    bool  ativa = false;
};
const int MAX_PART = 256;
Particula g_part[MAX_PART];

// ============================================================
//  ÁUDIO — efeitos 8-bit SINTETIZADOS (miniaudio, sem arquivos)
//  Cada "voz" gera uma onda quadrada (ou ruído) com decaimento.
//  É a estética clássica de arcade (Flappy/Mario), e livre de copyright.
// ============================================================
struct Voz {
    bool  ativa = false;
    int   tipo;          // 0 = blip (pulo), 1 = moeda (ponto), 2 = ruído (batida)
    float t, dur;        // tempo decorrido / duração
    float f0, f1;        // frequência inicial / final
    float fase;
    unsigned int seed;   // para o ruído
};
const int NUM_VOZ = 12;
Voz       g_voz[NUM_VOZ];
float     g_volumeAudio = 0.22f;
ma_device g_audioDevice;
bool      g_audioOK = false;

// Callback de áudio (roda em outra thread): mistura as vozes ativas.
void audioCallback(ma_device* dev, void* saida, const void* entrada, ma_uint32 nFrames) {
    (void)dev; (void)entrada;
    float* out = (float*)saida;
    const float SR = 44100.0f;
    for (ma_uint32 i = 0; i < nFrames; i++) {
        float s = 0.0f;
        for (int v = 0; v < NUM_VOZ; v++) {
            if (!g_voz[v].ativa) continue;
            Voz& z = g_voz[v];
            float prog = z.t / z.dur;
            if (prog >= 1.0f) { z.ativa = false; continue; }
            float env = 1.0f - prog;             // decaimento linear
            float amostra;
            if (z.tipo == 2) {                    // ruído branco (batida)
                z.seed = z.seed * 1664525u + 1013904223u;
                amostra = ((z.seed >> 16) & 0xFFFF) / 32768.0f - 1.0f;
            } else {
                float freq = z.f0 + (z.f1 - z.f0) * prog;
                if (z.tipo == 1) freq = (prog < 0.30f) ? z.f0 : z.f1; // moeda: pula nota
                z.fase += freq / SR;
                if (z.fase > 1.0f) z.fase -= 1.0f;
                amostra = (z.fase < 0.5f) ? 1.0f : -1.0f;  // onda quadrada
            }
            s += amostra * env;
            z.t += 1.0f / SR;
        }
        s *= g_volumeAudio;
        if (s > 1.0f) s = 1.0f; else if (s < -1.0f) s = -1.0f;
        out[i] = s;
    }
}

// Dispara uma voz livre (ativa por último = thread-safe simples).
void dispararVoz(int tipo, float dur, float f0, float f1) {
    for (int v = 0; v < NUM_VOZ; v++) {
        if (!g_voz[v].ativa) {
            Voz& z = g_voz[v];
            z.tipo = tipo; z.dur = dur; z.t = 0.0f;
            z.f0 = f0; z.f1 = f1; z.fase = 0.0f;
            z.seed = (unsigned)rand();
            z.ativa = true;
            return;
        }
    }
}

void tocarPulo()  { dispararVoz(0, 0.12f, 400.0f, 900.0f); }   // blip subindo
void tocarPonto() { dispararVoz(1, 0.18f, 988.0f, 1319.0f); }  // moeda (B5 -> E6)
void tocarMorte() {
    dispararVoz(2, 0.30f, 0.0f, 0.0f);          // ruído
    dispararVoz(0, 0.45f, 320.0f, 70.0f);       // tom descendo
}

void iniciarAudio() {
    ma_device_config cfg = ma_device_config_init(ma_device_type_playback);
    cfg.playback.format   = ma_format_f32;
    cfg.playback.channels = 1;
    cfg.sampleRate        = 44100;
    cfg.dataCallback      = audioCallback;
    if (ma_device_init(NULL, &cfg, &g_audioDevice) != MA_SUCCESS) {
        printf("AVISO: nao consegui iniciar o audio.\n");
        return;
    }
    ma_device_start(&g_audioDevice);
    g_audioOK = true;
    printf("Audio iniciado (efeitos 8-bit sintetizados).\n");
}

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

    mod.tamX = maxX - minX;
    mod.tamY = maxY - minY;
    mod.tamZ = maxZ - minZ;

    float maior = mod.tamX;
    if (mod.tamY > maior) maior = mod.tamY;
    if (mod.tamZ > maior) maior = mod.tamZ;
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
//  Cria uma textura PIXELADA de grama, gerada no próprio código.
//  Verdes variados + alguns fios mais claros/escuros. Filtro NEAREST
//  mantém o aspecto de "pixel art"; REPEAT permite repetir no chão.
// ------------------------------------------------------------
void criarTexturaGrama() {
    const int N = 32;
    unsigned char px[N * N * 3];
    for (int y = 0; y < N; y++) {
        for (int x = 0; x < N; x++) {
            float t = (rand() % 100) / 100.0f;
            // verde-sage terroso (menos saturado, mais neutro)
            float r = 0.26f + 0.08f * t;
            float g = 0.38f + 0.12f * t;
            float b = 0.20f + 0.07f * t;
            // fio de grama mais claro de vez em quando
            if (rand() % 6 == 0) { r = 0.40f; g = 0.50f; b = 0.30f; }
            // terra/sombra mais escura de vez em quando
            else if (rand() % 9 == 0) { r = 0.20f; g = 0.26f; b = 0.15f; }
            int i = (y * N + x) * 3;
            px[i]   = (unsigned char)(r * 255);
            px[i+1] = (unsigned char)(g * 255);
            px[i+2] = (unsigned char)(b * 255);
        }
    }
    glGenTextures(1, &g_texturaGrama);
    glBindTexture(GL_TEXTURE_2D, g_texturaGrama);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, N, N, 0, GL_RGB, GL_UNSIGNED_BYTE, px);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    printf("Textura de grama (procedural) criada.\n");
}

// ------------------------------------------------------------
//  Desenha UM modelo já normalizado (centralizado e escalado).
//  Quem chama é responsável por posicionar/rotacionar antes
//  (glTranslatef / glRotatef no mundo).
// ------------------------------------------------------------
// Aplica a cor do material, opcionalmente dessaturada (puxa p/ cinza).
void corMaterial(const Modelo& mod, float r, float g, float b) {
    float lum = 0.30f * r + 0.59f * g + 0.11f * b;   // luminância
    float d = mod.desaturar;
    glColor3f(r + (lum - r) * d, g + (lum - g) * d, b + (lum - b) * d);
}

void desenharModelo(Modelo& mod) {
    if (!mod.cena) return;

    // Na 1ª vez, compila toda a geometria numa display list (fica na GPU).
    // Nas próximas, só "chamamos" a lista — muito mais rápido para desenhar
    // muitas cópias (grama, árvores) sem reenviar os vértices a cada frame.
    if (mod.lista != 0) { glCallList(mod.lista); return; }
    mod.lista = glGenLists(1);
    glNewList(mod.lista, GL_COMPILE);

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

            // Cor por material: lê o Kd (cor difusa) do .mtl desta malha.
            // Assim a abelha sai amarela/preta/branca como no modelo.
            if (mod.usarCorMaterial && mod.textura == 0) {
                aiColor4D cor;
                const aiMaterial* mat = mod.cena->mMaterials[malha->mMaterialIndex];
                if (aiGetMaterialColor(mat, AI_MATKEY_COLOR_DIFFUSE, &cor) == AI_SUCCESS)
                    corMaterial(mod, cor.r, cor.g, cor.b);
            }

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

    glEndList();
    glCallList(mod.lista);  // desenha pela 1ª vez
}

// ------------------------------------------------------------
//  Desenha a geometria CRUA do modelo (sem escala/centralização).
//  Usado quando precisamos escalar de forma não-uniforme (o cano,
//  que é esticado para preencher cada altura).
// ------------------------------------------------------------
void desenharGeometria(const Modelo& mod) {
    for (unsigned int m = 0; m < mod.cena->mNumMeshes; m++) {
        const aiMesh* malha = mod.cena->mMeshes[m];
        if (mod.usarCorMaterial) {
            aiColor4D cor;
            const aiMaterial* mat = mod.cena->mMaterials[malha->mMaterialIndex];
            if (aiGetMaterialColor(mat, AI_MATKEY_COLOR_DIFFUSE, &cor) == AI_SUCCESS)
                corMaterial(mod, cor.r, cor.g, cor.b);
        }
        glBegin(GL_TRIANGLES);
        for (unsigned int f = 0; f < malha->mNumFaces; f++) {
            const aiFace& face = malha->mFaces[f];
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

// ------------------------------------------------------------
//  Desenha UMA malha específica do modelo (por índice), com a
//  cor do material. Usado no cano, que tem 2 malhas (corpo/borda)
//  desenhadas com escalas diferentes.
// ------------------------------------------------------------
void desenharMalhaCrua(const Modelo& mod, int meshIdx) {
    const aiMesh* malha = mod.cena->mMeshes[meshIdx];
    if (mod.usarCorMaterial) {
        aiColor4D cor;
        const aiMaterial* mat = mod.cena->mMaterials[malha->mMaterialIndex];
        if (aiGetMaterialColor(mat, AI_MATKEY_COLOR_DIFFUSE, &cor) == AI_SUCCESS)
            corMaterial(mod, cor.r, cor.g, cor.b);
    }
    glBegin(GL_TRIANGLES);
    for (unsigned int f = 0; f < malha->mNumFaces; f++) {
        const aiFace& face = malha->mFaces[f];
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

// ============================================================
//  PARTÍCULAS — poeira, brilho e explosão (billboards + blending)
// ============================================================
float aleat(float a, float b) { return a + (rand() % 1000) / 1000.0f * (b - a); }

void emitirParticula(float x, float y, float vx, float vy, float vida,
                     float r, float g, float b, float tam) {
    for (int i = 0; i < MAX_PART; i++) {
        if (!g_part[i].ativa) {
            Particula& p = g_part[i];
            p.x = x; p.y = y; p.z = 0.2f;
            p.vx = vx; p.vy = vy; p.vz = 0;
            p.vida = p.vidaMax = vida;
            p.r = r; p.g = g; p.b = b; p.tam = tam;
            p.ativa = true;
            return;
        }
    }
}

// Poeirinha clara ao passar pelo cano
void emitirPoeira(float x, float y) {
    for (int i = 0; i < 10; i++)
        emitirParticula(x + aleat(-0.2f, 0.2f), y + aleat(-0.3f, 0.3f),
                        aleat(-1.4f, -0.4f), aleat(-0.3f, 0.8f),
                        aleat(0.4f, 0.8f), 0.85f, 0.82f, 0.72f, aleat(0.10f, 0.20f));
}

// Brilho dourado ao pontuar
void emitirBrilho(float x, float y) {
    for (int i = 0; i < 14; i++) {
        float ang = aleat(0, 6.283f), v = aleat(1.0f, 2.5f);
        emitirParticula(x, y, cosf(ang) * v, sinf(ang) * v + 1.0f,
                        aleat(0.4f, 0.7f), 1.0f, 0.92f, 0.40f, aleat(0.07f, 0.15f));
    }
}

// Explosão alaranjada ao bater
void emitirExplosao(float x, float y) {
    for (int i = 0; i < 40; i++) {
        float ang = aleat(0, 6.283f), v = aleat(1.5f, 5.0f);
        emitirParticula(x, y, cosf(ang) * v, sinf(ang) * v,
                        aleat(0.5f, 1.0f), aleat(0.85f, 1.0f),
                        aleat(0.3f, 0.6f), 0.10f, aleat(0.10f, 0.24f));
    }
}

void atualizarParticulas(float dt) {
    for (int i = 0; i < MAX_PART; i++) {
        if (!g_part[i].ativa) continue;
        Particula& p = g_part[i];
        p.vida -= dt;
        if (p.vida <= 0) { p.ativa = false; continue; }
        p.vx *= 0.96f;            // arrasto do ar
        p.vy -= 4.0f * dt;        // gravidade leve
        p.x += p.vx * dt; p.y += p.vy * dt;
    }
}

void desenharParticulas() {
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBegin(GL_QUADS);
    for (int i = 0; i < MAX_PART; i++) {
        if (!g_part[i].ativa) continue;
        Particula& p = g_part[i];
        float a = p.vida / p.vidaMax;     // some suavemente
        float s = p.tam;
        glColor4f(p.r, p.g, p.b, a);
        // billboard no plano XY (a câmera olha por -Z, então encara a tela)
        glVertex3f(p.x - s, p.y - s, p.z);
        glVertex3f(p.x + s, p.y - s, p.z);
        glVertex3f(p.x + s, p.y + s, p.z);
        glVertex3f(p.x - s, p.y + s, p.z);
    }
    glEnd();
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}

// ============================================================
//  CÉU em DEGRADÊ e FLASH de tela (quads 2D em ortho)
// ============================================================
void desenharCeu() {
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    gluOrtho2D(0, 1, 0, 1);
    glMatrixMode(GL_MODELVIEW);  glPushMatrix(); glLoadIdentity();
    glBegin(GL_QUADS);
        // horizonte (embaixo): claro/neutro  | topo: azul dessaturado
        glColor3f(0.84f, 0.85f, 0.82f); glVertex2f(0, 0); glVertex2f(1, 0);
        glColor3f(0.44f, 0.58f, 0.70f); glVertex2f(1, 1); glVertex2f(0, 1);
    glEnd();
    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW);  glPopMatrix();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}

void desenharFlash() {
    if (g_flash <= 0.001f) return;
    glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    gluOrtho2D(0, 1, 0, 1);
    glMatrixMode(GL_MODELVIEW);  glPushMatrix(); glLoadIdentity();
    glColor4f(1.0f, 0.95f, 0.88f, g_flash);
    glBegin(GL_QUADS);
        glVertex2f(0, 0); glVertex2f(1, 0); glVertex2f(1, 1); glVertex2f(0, 1);
    glEnd();
    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW);  glPopMatrix();
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST); glEnable(GL_LIGHTING);
}

// Morte: game over + tremor + flash + explosão de partículas.
void morrer() {
    if (g_estado != JOGANDO) return;
    g_estado = GAMEOVER;
    g_shake = 0.5f;
    g_flash = 0.8f;
    emitirExplosao(CAPIVARA_X, g_capivaraY);
    tocarMorte();
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
// O bat wing é um PAR plano no plano XY. Desenhamos cada metade como
// UMA asa, controlando as duas separadamente para formar um par de voo:
//  - ASA DE TRÁS  (flanco -Z): alta e flutuante, atrás da capivara.
//  - ASA DA FRENTE (flanco +Z): mais baixa, acoplada nas costas (lado visível).
const float ASA_ROT_X  = 1.0f;  // leve inclinação para trás (ambas)
const float ASA_TAM    =  1.9f;   // tamanho-alvo das asas

// O modelo é um par PLANO espalhado na horizontal (eixo X). Para a asa
// "subir" das costas (e não ficar deitada), giramos cada asa em torno
// de Z por ASA_LEVANTA graus: a extensão horizontal vira vertical.
// A raiz (borda interna, x=0) é ancorada no ombro das duas asas.
const float ASA_LEVANTA = 45.0f;  // quanto a asa sobe (graus)

// Asa de TRÁS (flanco -Z): raiz no ombro, um pouco mais alta.
const float ASA_TRAS_DX = -0.10f;
const float ASA_TRAS_DY =  0.44f;
const float ASA_TRAS_DZ = -0.20f;

// Asa da FRENTE (flanco +Z): raiz no mesmo ombro, levemente mais baixa.
const float ASA_FRENTE_DX = -0.10f;
const float ASA_FRENTE_DY =  0.37f;
const float ASA_FRENTE_DZ =  0.02f;

// ------------------------------------------------------------
//  Posição da capivara NA TELA. Na tela inicial ela fica
//  centralizada no X e flutuando (idle bob); jogando, usa a
//  posição real do jogo.
// ------------------------------------------------------------
float capivaraTelaX() {
    return (g_estado == INICIO) ? 0.0f : CAPIVARA_X;
}
float capivaraTelaY() {
    if (g_estado == INICIO) {
        float t = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
        return CAPIVARA_Y_INICIAL + sinf(t * 2.0f) * 0.3f;  // flutua suave
    }
    return g_capivaraY;
}

// ============================================================
//  Desenha a capivara no mundo (posição + giro de perfil).
// ============================================================
void desenharCapivara() {
    // Inclina conforme a velocidade: nariz p/ cima subindo, p/ baixo caindo.
    float inclina = g_velocidadeY * 4.0f;          // graus (proporcional)
    if (inclina >  30.0f) inclina =  30.0f;
    if (inclina < -45.0f) inclina = -45.0f;

    glPushMatrix();
        glTranslatef(capivaraTelaX(), capivaraTelaY(), 0.0f);
        glRotatef(90.0f, 0.0f, 1.0f, 0.0f);   // de perfil, olhando p/ direita
        glRotatef(inclina, 0.0f, 0.0f, 1.0f); // inclinação do voo
        desenharModelo(g_capivara);
    glPopMatrix();
}

// ------------------------------------------------------------
//  Desenha só UMA metade do modelo das asas.
//  lado = -1 (esquerda, x <= centro) ou +1 (direita, x > centro).
//  Assim podemos girar cada metade em sentido oposto e o bater
//  fica simétrico (não vira "gangorra").
// ------------------------------------------------------------
void desenharMetadeAsa(int lado) {
    const Modelo& mod = g_asas;
    glColor3f(mod.corR, mod.corG, mod.corB);
    for (unsigned int m = 0; m < mod.cena->mNumMeshes; m++) {
        const aiMesh* malha = mod.cena->mMeshes[m];
        glBegin(GL_TRIANGLES);
        for (unsigned int f = 0; f < malha->mNumFaces; f++) {
            const aiFace& face = malha->mFaces[f];
            // centro X da face (média dos vértices) decide o lado
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

// ============================================================
//  Desenha as asas sobre as costas da capivara.
//  A batida é DISPARADA PELO PULO (animação one-shot): logo após
//  um pulo as asas dão uma batida e voltam ao repouso.
// ============================================================
void desenharAsas() {
    // Batida única disparada pelo pulo.
    float agora = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    float t = agora - g_tempoUltimoPulo;
    float anguloAsa = 0.0f;                        // repouso
    if (t < DURACAO_BATIDA) {
        float prog = t / DURACAO_BATIDA;           // 0 -> 1
        anguloAsa = sinf(prog * 3.14159f) * AMPLITUDE_BATIDA;  // sobe e volta
    }

    float capX = capivaraTelaX();
    float capY = capivaraTelaY();

    // Ângulo total de levantamento (base + batida). A metade ESQUERDA
    // (-1) se estende para -X; girar -ângulo em Z a leva para cima/trás.
    // As duas asas usam a MESMA metade e o MESMO sinal => batem juntas.
    float anguloTotal = -(ASA_LEVANTA + anguloAsa);

    // ---- Asa de TRÁS (flanco -Z): raiz no ombro, um pouco mais alta ----
    glPushMatrix();
        glTranslatef(capX + ASA_TRAS_DX, capY + ASA_TRAS_DY, ASA_TRAS_DZ);
        glRotatef(ASA_ROT_X, 1.0f, 0.0f, 0.0f);
        glRotatef(anguloTotal, 0.0f, 0.0f, 1.0f);  // levanta + batida
        glScalef(g_asas.escala, g_asas.escala, g_asas.escala);
        glTranslatef(-g_asas.centroX, -g_asas.centroY, -g_asas.centroZ);
        desenharMetadeAsa(-1);
    glPopMatrix();

    // ---- Asa da FRENTE (flanco +Z): raiz no mesmo ombro, mais baixa ----
    glPushMatrix();
        glTranslatef(capX + ASA_FRENTE_DX, capY + ASA_FRENTE_DY, ASA_FRENTE_DZ);
        glRotatef(ASA_ROT_X, 1.0f, 0.0f, 0.0f);
        glRotatef(anguloTotal, 0.0f, 0.0f, 1.0f);  // mesmo sinal => juntas
        glScalef(g_asas.escala, g_asas.escala, g_asas.escala);
        glTranslatef(-g_asas.centroX, -g_asas.centroY, -g_asas.centroZ);
        desenharMetadeAsa(-1);
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

// ---- DIFICULDADE PROGRESSIVA (funções da pontuação) ----
// Quanto mais pontos, mais rápido o cano vem (até um limite).
float velocidadeCanoAtual() {
    float extra = g_pontuacao * 0.12f;
    if (extra > 3.5f) extra = 3.5f;        // teto de velocidade
    return VELOCIDADE_CANO + extra;
}
// E a brecha vai encolhendo um pouco (até um mínimo jogável).
float alturaBrechaAtual() {
    float brecha = ALTURA_BRECHA - g_pontuacao * 0.04f;
    if (brecha < 2.1f) brecha = 2.1f;      // não fica impossível
    return brecha;
}

void inicializarCanos() {
    for (int i = 0; i < NUM_CANOS; i++) {
        g_canos[i].x = CANO_X_INICIAL + i * ESPACO_CANOS;
        g_canos[i].centroBrecha = sortearBrecha();
        g_canos[i].contado = false;
    }
}

// ---- GRAMA 3D na base (fileira que rola e recicla) ----
void inicializarGrama() {
    for (int i = 0; i < NUM_GRAMAS; i++)
        g_gramaX[i] = -GRAMA_LIMITE + i * GRAMA_ESPACO;
}

void atualizarGrama(float dt) {
    float vel = velocidadeCanoAtual();   // anda junto com os canos
    for (int i = 0; i < NUM_GRAMAS; i++) {
        g_gramaX[i] -= vel * dt;
        // ao sair pela esquerda, "apaga" e reaproveita o tufo à direita
        if (g_gramaX[i] < -GRAMA_LIMITE)
            g_gramaX[i] += NUM_GRAMAS * GRAMA_ESPACO;
    }
}

// ---- ÁRVORES de fundo (parallax + reciclagem) ----
// Sorteia modelo, tamanho e profundidade de UMA árvore.
void sortearArvore(int i) {
    g_arvoreTipo[i]   = rand() % 2;                          // Tree ou Tree-2
    g_arvoreEscala[i] = 0.55f + (rand() % 100) / 100.0f * 0.55f; // 0.55..1.10
    g_arvoreZ[i]      = -5.0f - (rand() % 100) / 100.0f * 2.5f;  // -5.0..-7.5
}

void inicializarArvores() {
    for (int i = 0; i < NUM_ARVORES; i++) {
        g_arvoreX[i] = -ARVORE_LIMITE + i * ARVORE_ESPACO;
        sortearArvore(i);
    }
}

void atualizarArvores(float dt) {
    float vel = velocidadeCanoAtual() * ARVORE_PARALLAX;  // mais devagar
    for (int i = 0; i < NUM_ARVORES; i++) {
        g_arvoreX[i] -= vel * dt;
        if (g_arvoreX[i] < -ARVORE_LIMITE) {
            g_arvoreX[i] += NUM_ARVORES * ARVORE_ESPACO;
            sortearArvore(i);   // ao reaparecer, vira outra árvore (aleatória)
        }
    }
}

void desenharArvores() {
    for (int i = 0; i < NUM_ARVORES; i++) {
        float f = g_arvoreEscala[i];
        // A base do tronco fica em y=0 para qualquer escala: o modelo
        // normalizado tem a base em -ARVORE_TAM/2, então subimos isso * f.
        float baseY = (ARVORE_TAM / 2.0f) * f;
        glPushMatrix();
            glTranslatef(g_arvoreX[i], baseY, g_arvoreZ[i]);
            glScalef(f, f, f);
            desenharModelo(g_arvoreTipo[i] == 0 ? g_arvore1 : g_arvore2);
        glPopMatrix();
    }
}

void desenharGrama() {
    // Cada coluna é desenhada em todas as fileiras (z), preenchendo o campo.
    // Fileiras alternadas levam meia-coluna de deslocamento para não ficar
    // tudo enfileirado igual (parece mais natural).
    for (int f = 0; f < GRAMA_FILEIRAS; f++) {
        float desloc = (f % 2) * (GRAMA_ESPACO * 0.5f);
        for (int i = 0; i < NUM_GRAMAS; i++) {
            glPushMatrix();
                glTranslatef(g_gramaX[i] + desloc, GRAMA_DY, GRAMA_Z[f]);
                desenharModelo(g_grama);
            glPopMatrix();
        }
    }
}

void atualizarCanos(float dt) {
    float vel = velocidadeCanoAtual();  // acelera conforme a pontuação
    for (int i = 0; i < NUM_CANOS; i++) {
        g_canos[i].x -= vel * dt;

        // pontua quando o cano passa do X da capivara
        if (!g_canos[i].contado && g_canos[i].x < CAPIVARA_X) {
            g_canos[i].contado = true;
            g_pontuacao++;
            emitirBrilho(CAPIVARA_X, g_capivaraY);   // brilho ao pontuar
            emitirPoeira(CAPIVARA_X, g_capivaraY);   // poeirinha
            tocarPonto();                            // som de moeda
        }
        // recicla para a direita ao sair de cena
        if (g_canos[i].x < CANO_X_RECICLA) {
            g_canos[i].x += NUM_CANOS * ESPACO_CANOS;
            g_canos[i].centroBrecha = sortearBrecha();
            g_canos[i].contado = false;
        }
    }
}

// Info por-malha do cano (corpo e borda), medida 1x no carregamento.
struct MalhaInfo {
    int   idx = 0;
    float cx = 0, cy = 0, cz = 0;        // centro do bounding box
    float tamX = 1, tamY = 1, tamZ = 1;  // dimensões
};
MalhaInfo g_tube, g_rim;

// Mede as 2 malhas do cano e decide qual é o CORPO (mais alto em Y) e
// qual é a BORDA (a outra). Robusto à ordem/nome das malhas no GLB.
void prepararCano() {
    if (!g_cano.cena || g_cano.cena->mNumMeshes < 2) return;
    MalhaInfo info[2];
    for (int m = 0; m < 2; m++) {
        const aiMesh* malha = g_cano.cena->mMeshes[m];
        float mnx=1e9f,mny=1e9f,mnz=1e9f, mxx=-1e9f,mxy=-1e9f,mxz=-1e9f;
        for (unsigned v = 0; v < malha->mNumVertices; v++) {
            aiVector3D p = malha->mVertices[v];
            if(p.x<mnx)mnx=p.x; if(p.x>mxx)mxx=p.x;
            if(p.y<mny)mny=p.y; if(p.y>mxy)mxy=p.y;
            if(p.z<mnz)mnz=p.z; if(p.z>mxz)mxz=p.z;
        }
        info[m].idx = m;
        info[m].cx=(mnx+mxx)/2; info[m].cy=(mny+mxy)/2; info[m].cz=(mnz+mxz)/2;
        info[m].tamX=mxx-mnx;   info[m].tamY=mxy-mny;   info[m].tamZ=mxz-mnz;
    }
    if (info[0].tamY >= info[1].tamY) { g_tube = info[0]; g_rim = info[1]; }
    else                              { g_tube = info[1]; g_rim = info[0]; }
}

// Desenha o cano preenchendo de y0 até y1, no x dado.
// CORPO: cilindro liso esticado em Y (esticar liso não distorce).
// BORDA: escala UNIFORME na boca do cano (por isso não distorce mais).
//  - invertido=false: cano de baixo, boca/borda para CIMA (y1).
//  - invertido=true : cano de cima,  boca/borda para BAIXO (y0).
void desenharCanoModelo(float x, float y0, float y1, bool invertido) {
    if (!g_cano.cena) return;
    float altura = y1 - y0;

    // fator horizontal: leva o diâmetro do CORPO à LARGURA_CANO
    float escXZ = LARGURA_CANO / g_tube.tamX;

    // ---- CORPO: estica em Y para preencher o segmento ----
    float escY = altura / g_tube.tamY;
    glPushMatrix();
        glTranslatef(x, (y0 + y1) / 2.0f, 0.0f);
        glScalef(escXZ, escY, escXZ);
        glTranslatef(-g_tube.cx, -g_tube.cy, -g_tube.cz);
        desenharMalhaCrua(g_cano, g_tube.idx);
    glPopMatrix();

    // ---- BORDA: escala uniforme (sem distorção) na boca do cano ----
    float yBoca = invertido ? y0 : y1;
    glPushMatrix();
        glTranslatef(x, yBoca, 0.0f);
        glScalef(escXZ, escXZ, escXZ);   // UNIFORME => rim mantém a proporção
        glTranslatef(-g_rim.cx, -g_rim.cy, -g_rim.cz);
        desenharMalhaCrua(g_cano, g_rim.idx);
    glPopMatrix();
}

void desenharCanos() {
    for (int i = 0; i < NUM_CANOS; i++) {
        float c = g_canos[i].centroBrecha;
        float meia = alturaBrechaAtual() / 2.0f;
        desenharCanoModelo(g_canos[i].x, 0.0f, c - meia, false);          // baixo
        desenharCanoModelo(g_canos[i].x, c + meia, ALTURA_TETO + 2, true); // cima
    }
}

// Colisão da capivara (caixa AABB) com os canos e o chão.
void verificarColisoesCanos() {
    float cx0 = CAPIVARA_X - RAIO_CAPIVARA, cx1 = CAPIVARA_X + RAIO_CAPIVARA;
    float cy0 = g_capivaraY - RAIO_CAPIVARA, cy1 = g_capivaraY + RAIO_CAPIVARA;

    // chão
    if (g_capivaraY - RAIO_CAPIVARA <= 0.0f) { morrer(); return; }

    float meiaL = LARGURA_CANO / 2.0f;
    for (int i = 0; i < NUM_CANOS; i++) {
        float px0 = g_canos[i].x - meiaL, px1 = g_canos[i].x + meiaL;
        float c = g_canos[i].centroBrecha, meia = alturaBrechaAtual() / 2.0f;

        // cano de baixo: y de 0 até (c - meia)
        bool bate = sobreposicaoAABB(cx0,cx1, cy0,cy1, px0,px1, 0.0f, c - meia);
        // cano de cima: y de (c + meia) até o topo
        bate = bate || sobreposicaoAABB(cx0,cx1, cy0,cy1, px0,px1,
                                        c + meia, ALTURA_TETO + 2.0f);
        if (bate) { morrer(); return; }
    }
}

// ============================================================
//  INIMIGO com IA (Máquina de Estados Finitos: vagar/perseguir)
// ============================================================
void inicializarInimigo() {
    g_inimigo.x = CAPIVARA_X + 1.5f;
    g_inimigo.y = 3.0f;
    g_inimigo.vx = 0.0f;
    g_inimigo.vy = 0.0f;
    g_inimigo.estadoIA = VAGANDO;
    g_inimigo.tempoProxSorteio = 0.0f;
}

// O "carrapato" FOGE da capivara: ele fica por perto, mas sempre se
// afasta na vertical para o lado oposto da capivara. Como a capivara
// tem X fixo, ela nunca o alcança — só fica tentando.
void atualizarInimigo(float dt) {
    float agora = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;

    // Foge para o lado vertical oposto ao da capivara, com leve oscilação.
    float ladoFuga = (g_inimigo.y >= g_capivaraY) ? +1.0f : -1.0f;
    float alvoY = g_capivaraY + ladoFuga * 2.0f + sinf(agora * 3.0f) * 0.5f;

    // Aproxima suavemente do alvo (movimento orgânico de fuga).
    float k = 3.0f * dt;
    if (k > 1.0f) k = 1.0f;
    g_inimigo.y += (alvoY - g_inimigo.y) * k;

    // X: vagueia um pouco à frente da capivara (nunca encosta).
    g_inimigo.x = CAPIVARA_X + 1.6f + sinf(agora * 1.3f) * 0.8f;

    // Limites verticais da área de jogo
    if (g_inimigo.y < 1.0f) g_inimigo.y = 1.0f;
    if (g_inimigo.y > ALTURA_TETO) g_inimigo.y = ALTURA_TETO;
}

void desenharInimigo() {
    // Zumbido: pequena oscilação rápida na vertical (parece voo de abelha).
    float agora = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    float zumbido = sinf(agora * 25.0f) * 0.04f;

    // Orientação: fica a MAIOR parte do tempo de PERFIL (voando, olhando
    // para frente) e de vez em quando gira suavemente para encarar a tela.
    //  - perfil       => ângulo 0
    //  - olhando p/ câmera => ângulo -90 (vista frontal vista antes)
    float fase  = sinf(agora * 0.7f);          // -1..1 (lento)
    float olhar = fmaxf(0.0f, fase);           // 0 na maior parte do ciclo
    olhar = olhar * olhar;                     // suaviza (fica mais no perfil)
    float anguloY = olhar * -90.0f;            // perfil -> câmera -> perfil

    glPushMatrix();
        glTranslatef(g_inimigo.x, g_inimigo.y + zumbido, 0.0f);
        glRotatef(anguloY, 0.0f, 1.0f, 0.0f);
        desenharModelo(g_abelha);
    glPopMatrix();
}

// ============================================================
//  HUD — texto 2D (placar e mensagens), por cima da cena
// ============================================================

// Texto simples em bitmap (placar durante o jogo, tela de início).
void desenharTexto(float x, float y, const char* texto) {
    glRasterPos2f(x, y);
    for (const char* c = texto; *c != '\0'; c++)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
}

// --- Fonte VETORIAL (stroke): pode ser ampliada sem ficar borrada,
//     dando um visual "grande e marcante" para o GAME OVER. ---

// Largura em pixels que um texto stroke ocupa numa dada escala.
float larguraStroke(const char* texto, float escala) {
    float w = 0;
    for (const char* c = texto; *c != '\0'; c++)
        w += glutStrokeWidth(GLUT_STROKE_MONO_ROMAN, *c);
    return w * escala;
}

// Desenha um texto stroke começando em (x, y) com dada escala/espessura.
void desenharStroke(float x, float y, float escala, float espessura,
                    const char* texto) {
    glLineWidth(espessura);
    glPushMatrix();
        glTranslatef(x, y, 0.0f);
        glScalef(escala, escala, 1.0f);
        for (const char* c = texto; *c != '\0'; c++)
            glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN, *c);
    glPopMatrix();
}

// Texto stroke CENTRALIZADO na horizontal, com contorno preto + preenchimento.
// Desenha primeiro grosso em preto (contorno) e depois fino na cor de cima.
void desenharStrokeCentralizado(float cx, float y, float escala,
                                const char* texto,
                                float r, float g, float b) {
    float largura = larguraStroke(texto, escala);
    float x = cx - largura / 2.0f;
    // contorno preto (linha mais grossa por baixo)
    glColor3f(0.0f, 0.0f, 0.0f);
    desenharStroke(x, y, escala, 6.0f, texto);
    // preenchimento colorido (linha mais fina por cima)
    glColor3f(r, g, b);
    desenharStroke(x, y, escala, 2.5f, texto);
}

// Retângulo com cantos arredondados (TRIANGLE_FAN: centro + borda).
void retanguloArredondado(float x, float y, float larg, float alt, float raio) {
    int seg = 6;  // segmentos por canto
    float cx = x + larg / 2.0f, cy = y + alt / 2.0f;
    glBegin(GL_TRIANGLE_FAN);
        glVertex2f(cx, cy);  // centro do leque
        // 4 cantos (cada um é um arco de 90°)
        for (int canto = 0; canto < 4; canto++) {
            // centro de cada arco (cantos internos do retângulo)
            float ax = (canto == 0 || canto == 3) ? x + raio : x + larg - raio;
            float ay = (canto < 2) ? y + raio : y + alt - raio;
            float ini = 0;
            if (canto == 0) ini = 180; else if (canto == 1) ini = 270;
            else if (canto == 2) ini = 0; else ini = 90;
            for (int s = 0; s <= seg; s++) {
                float ang = (ini + 90.0f * s / seg) * 3.14159f / 180.0f;
                glVertex2f(ax + cosf(ang) * raio, ay + sinf(ang) * raio);
            }
        }
        // fecha o leque voltando ao primeiro ponto
        glVertex2f(x + raio, y);
    glEnd();
}

// ============================================================
//  FONTE TTF (Pixelify Sans) via stb_truetype — glifos SÓLIDOS.
//  Rasterizamos a fonte uma vez para um "atlas" (textura) e
//  depois desenhamos cada letra como um quad texturizado.
// ============================================================
const char*  CAMINHO_FONTE = "fonts/PixelifySans.ttf";
const int    ATLAS_W = 1024, ATLAS_H = 1024; // atlas grande p/ caber tudo
const float  FONTE_ALTURA = 96.0f;           // px de rasterização (nítido)
const float  KERN_PX = 7.0f;                // kerning NEGATIVO (letras unidas)
const float  ESPACO_EXTRA = 45.0f;          // alarga o espaço entre PALAVRAS

stbtt_bakedchar g_glifos[96];   // dados dos caracteres ASCII 32..127
GLuint g_fonteTex = 0;          // textura do atlas (0 = não carregada)

// Carrega o .ttf e gera o atlas. Precisa do contexto GL já criado.
void carregarFonte() {
    FILE* f = fopen(CAMINHO_FONTE, "rb");
    if (!f) { printf("AVISO: fonte '%s' nao encontrada.\n", CAMINHO_FONTE); return; }
    fseek(f, 0, SEEK_END); long tam = ftell(f); fseek(f, 0, SEEK_SET);
    unsigned char* ttf = (unsigned char*)malloc(tam);
    fread(ttf, 1, tam, f); fclose(f);

    // Rasteriza os glifos para um bitmap de 1 canal (cobertura/alpha)
    unsigned char* bitmap = (unsigned char*)malloc(ATLAS_W * ATLAS_H);
    int r = stbtt_BakeFontBitmap(ttf, 0, FONTE_ALTURA, bitmap,
                                 ATLAS_W, ATLAS_H, 32, 96, g_glifos);
    if (r <= 0)
        printf("AVISO: atlas pequeno demais p/ a fonte (retorno %d).\n", r);

    // Expande para RGBA: branco com alpha = cobertura (p/ tingir com glColor)
    unsigned char* rgba = (unsigned char*)malloc(ATLAS_W * ATLAS_H * 4);
    for (int i = 0; i < ATLAS_W * ATLAS_H; i++) {
        rgba[i*4+0] = 255; rgba[i*4+1] = 255;
        rgba[i*4+2] = 255; rgba[i*4+3] = bitmap[i];
    }
    glGenTextures(1, &g_fonteTex);
    glBindTexture(GL_TEXTURE_2D, g_fonteTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ATLAS_W, ATLAS_H, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    free(ttf); free(bitmap); free(rgba);
    printf("Fonte TTF carregada: %s\n", CAMINHO_FONTE);
}

// Largura em pixels que o texto ocupa numa dada escala (com kerning negativo).
float larguraTTF(const char* texto, float escala) {
    float x = 0;
    for (const char* c = texto; *c; c++) {
        if ((unsigned char)*c < 32 || (unsigned char)*c >= 128) continue;
        x += g_glifos[(unsigned char)*c - 32].xadvance;
        // espaço entre palavras é alargado; entre letras é aproximado
        x += (*c == ' ') ? ESPACO_EXTRA : -KERN_PX;
    }
    return x * escala;
}

// Desenha o texto em cor sólida (usado para sombra e contorno).
void desenharTTFsolido(float x, float y, float escala, const char* texto,
                       float r, float g, float b, float a) {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, g_fonteTex);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(r, g, b, a);
    glBegin(GL_QUADS);
    float xpos = 0, ypos = 0;
    for (const char* c = texto; *c; c++) {
        if ((unsigned char)*c < 32 || (unsigned char)*c >= 128) continue;
        if (*c == ' ') { xpos += g_glifos[0].xadvance + ESPACO_EXTRA; continue; }
        stbtt_aligned_quad q;
        stbtt_GetBakedQuad(g_glifos, ATLAS_W, ATLAS_H, (unsigned char)*c - 32, &xpos, &ypos, &q, 1);
        // y é invertido: no atlas cresce p/ baixo, no nosso ortho p/ cima
        glTexCoord2f(q.s0, q.t0); glVertex2f(x + q.x0 * escala, y - q.y0 * escala);
        glTexCoord2f(q.s1, q.t0); glVertex2f(x + q.x1 * escala, y - q.y0 * escala);
        glTexCoord2f(q.s1, q.t1); glVertex2f(x + q.x1 * escala, y - q.y1 * escala);
        glTexCoord2f(q.s0, q.t1); glVertex2f(x + q.x0 * escala, y - q.y1 * escala);
        xpos += (*c == ' ') ? ESPACO_EXTRA : -KERN_PX;  // palavra x letra
    }
    glEnd();
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
}

// Desenha o texto com GRADIENTE vertical: branco no topo, laranja embaixo.
void desenharTTFgradiente(float x, float y, float escala, const char* texto) {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, g_fonteTex);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBegin(GL_QUADS);
    float xpos = 0, ypos = 0;
    for (const char* c = texto; *c; c++) {
        if ((unsigned char)*c < 32 || (unsigned char)*c >= 128) continue;
        if (*c == ' ') { xpos += g_glifos[0].xadvance + ESPACO_EXTRA; continue; }
        stbtt_aligned_quad q;
        stbtt_GetBakedQuad(g_glifos, ATLAS_W, ATLAS_H, (unsigned char)*c - 32, &xpos, &ypos, &q, 1);
        // topo (q.y0) = branco | base (q.y1) = laranja capivara (#F3C68F)
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        glTexCoord2f(q.s0, q.t0); glVertex2f(x + q.x0 * escala, y - q.y0 * escala);
        glTexCoord2f(q.s1, q.t0); glVertex2f(x + q.x1 * escala, y - q.y0 * escala);
        glColor4f(0.953f, 0.776f, 0.561f, 1.0f);
        glTexCoord2f(q.s1, q.t1); glVertex2f(x + q.x1 * escala, y - q.y1 * escala);
        glTexCoord2f(q.s0, q.t1); glVertex2f(x + q.x0 * escala, y - q.y1 * escala);
        xpos -= KERN_PX;
    }
    glEnd();
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
}

// Título completo: sombra dura + contorno preto pesado + gradiente.
void desenharTituloTTF(float cx, float y, float escala, const char* texto) {
    float x = cx - larguraTTF(texto, escala) / 2.0f;

    // 1) Sombra preta sólida deslocada +4x, -4y (4px p/ baixo na tela)
    desenharTTFsolido(x + 4, y - 4, escala, texto, 0, 0, 0, 1.0f);

    // 2) Contorno preto: 8 cópias deslocadas em volta (~3px) que abraçam
    //    a letra sem virar um bloco sólido atrás do texto.
    float d = 3.0f;
    float ox[8] = {-d,  0,  d, -d, d, -d, 0, d};
    float oy[8] = {-d, -d, -d,  0, 0,  d, d, d};
    for (int i = 0; i < 8; i++)
        desenharTTFsolido(x + ox[i], y + oy[i], escala, texto, 0, 0, 0, 1.0f);

    // 3) Preenchimento com gradiente branco -> laranja
    desenharTTFgradiente(x, y, escala, texto);
}

// Tela INICIAL seguindo a regra dos terços (eixo Y):
//  22% -> título | 48% -> capivara (3D) | 74% -> instrução
//  + chão 2D enxuto nos 15% inferiores com crista de grama.
void desenharTelaInicio() {
    float cx = LARGURA_JANELA / 2.0f;
    float W = LARGURA_JANELA, H = ALTURA_JANELA;

    // ---- CHÃO: faixa de grama pixelada nos 15% inferiores ----
    float chaoTopo = H * 0.15f;
    glColor3f(1.0f, 1.0f, 1.0f);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, g_texturaGrama);
    glBegin(GL_QUADS);
        glTexCoord2f(0, 0);              glVertex2f(0, 0);
        glTexCoord2f(W / 40.0f, 0);      glVertex2f(W, 0);
        glTexCoord2f(W / 40.0f, chaoTopo / 40.0f); glVertex2f(W, chaoTopo);
        glTexCoord2f(0, chaoTopo / 40.0f);         glVertex2f(0, chaoTopo);
    glEnd();
    glDisable(GL_TEXTURE_2D);
    // crista da grama: faixa de 4px, verde ~20% mais claro, no topo do chão
    glColor3f(0.30f, 0.62f, 0.25f);
    glBegin(GL_QUADS);
        glVertex2f(0, chaoTopo - 4); glVertex2f(W, chaoTopo - 4);
        glVertex2f(W, chaoTopo);     glVertex2f(0, chaoTopo);
    glEnd();

    // ---- TÍTULO a 22% do TOPO (em ortho, y mede de baixo p/ cima) ----
    float escTitulo = 0.72f;
    desenharTituloTTF(cx, H * (1.0f - 0.22f), escTitulo, "FLAPPY CAPIVARA");

    // ---- INSTRUÇÃO a 74% do topo: METADE do tamanho, piscando ----
    // Branco + contorno preto simples (2px), SEM sombra/duplicata.
    float escInstr = escTitulo * 0.5f;
    float tempo = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    float alpha = 0.65f + 0.35f * sinf(tempo * 3.0f);  // pisca 0.3 -> 1.0

    const char* instr = "CLIQUE ESPACO PARA COMECAR";
    float xi = cx - larguraTTF(instr, escInstr) / 2.0f;
    float yi = H * (1.0f - 0.74f);

    // contorno preto fino (4 cópias ~2px) + branco por cima
    float d = 2.0f;
    desenharTTFsolido(xi - d, yi, escInstr, instr, 0, 0, 0, alpha);
    desenharTTFsolido(xi + d, yi, escInstr, instr, 0, 0, 0, alpha);
    desenharTTFsolido(xi, yi - d, escInstr, instr, 0, 0, 0, alpha);
    desenharTTFsolido(xi, yi + d, escInstr, instr, 0, 0, 0, alpha);
    desenharTTFsolido(xi, yi, escInstr, instr, 1.0f, 1.0f, 1.0f, alpha);
}

// Desenha a tela/modal de GAME OVER (overlay + card + textos).
void desenharGameOver() {
    float cx = LARGURA_JANELA / 2.0f;  // centro horizontal da tela
    char buf[64];

    // 1) Overlay escuro semitransparente sobre a tela toda
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.0f, 0.0f, 0.0f, 0.7f);
    glBegin(GL_QUADS);
        glVertex2f(0, 0); glVertex2f(LARGURA_JANELA, 0);
        glVertex2f(LARGURA_JANELA, ALTURA_JANELA); glVertex2f(0, ALTURA_JANELA);
    glEnd();

    // 2) Card centralizado
    float cardL = 480, cardA = 280;                 // largura e altura
    float cardX = cx - cardL / 2.0f;                // canto inferior-esquerdo
    float cardY = ALTURA_JANELA / 2.0f - cardA / 2.0f;

    // sombra sutil (card deslocado, mais escuro e transparente)
    glColor4f(0.0f, 0.0f, 0.0f, 0.35f);
    retanguloArredondado(cardX + 8, cardY - 8, cardL, cardA, 18);
    // fundo do card (azul-ardósia elegante)
    glColor4f(0.13f, 0.16f, 0.24f, 0.97f);
    retanguloArredondado(cardX, cardY, cardL, cardA, 18);
    // borda clara fina
    glColor4f(0.45f, 0.55f, 0.75f, 1.0f);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
        glVertex2f(cardX + 18, cardY);
        glVertex2f(cardX + cardL - 18, cardY);
        glVertex2f(cardX + cardL, cardY + 18);
        glVertex2f(cardX + cardL, cardY + cardA - 18);
        glVertex2f(cardX + cardL - 18, cardY + cardA);
        glVertex2f(cardX + 18, cardY + cardA);
        glVertex2f(cardX, cardY + cardA - 18);
        glVertex2f(cardX, cardY + 18);
    glEnd();

    // 3) Conteúdo (de cima para baixo)
    // Título "GAME OVER" em dourado, grande e com contorno
    desenharStrokeCentralizado(cx, cardY + cardA - 95, 0.32f,
                               "GAME OVER", 1.0f, 0.78f, 0.25f);

    // Pontuação (branco)
    snprintf(buf, sizeof(buf), "PONTUACAO: %d", g_pontuacao);
    desenharStrokeCentralizado(cx, cardY + 95, 0.14f,
                               buf, 0.95f, 0.95f, 0.95f);

    // Rodapé com opacidade pulsante (efeito de "piscar")
    float tempo = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    float pulso = 0.5f + 0.5f * sinf(tempo * 4.0f);   // 0..1
    glColor4f(0.85f, 0.85f, 0.90f, pulso);
    const char* rodape = "PRESSIONE ESPACO PARA REINICIAR";
    float escR = 0.09f;
    float xR = cx - larguraStroke(rodape, escR) / 2.0f;
    desenharStroke(xR, cardY + 45, escR, 1.5f, rodape);

    glDisable(GL_BLEND);
}

void desenharHUD() {
    // entra em 2D: salva projeção/modelview, desliga luz, profundidade
    // e CULLING (senão os quads de texto, no sentido horário, somem).
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    gluOrtho2D(0, LARGURA_JANELA, 0, ALTURA_JANELA);
    glMatrixMode(GL_MODELVIEW);  glPushMatrix(); glLoadIdentity();

    glColor3f(1.0f, 1.0f, 1.0f);
    char buf[64];

    if (g_estado == INICIO) {
        desenharTelaInicio();
    } else if (g_estado == JOGANDO) {
        snprintf(buf, sizeof(buf), "Pontos: %d", g_pontuacao);
        desenharTexto(20, ALTURA_JANELA - 30, buf);
    } else if (g_estado == GAMEOVER) {
        desenharGameOver();
    }

    // restaura 3D
    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW);  glPopMatrix();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_CULL_FACE);
}

// ============================================================
//  Callback de desenho — chamado toda vez que a janela
//  precisa ser redesenhada (pelo glutPostRedisplay ou evento)
// ============================================================
void display() {
    // Limpa o buffer de cor e o buffer de profundidade (z-buffer)
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Céu em degradê (desenhado antes de tudo, sem profundidade)
    desenharCeu();

    // Carrega a matriz de modelo/visão e posiciona a câmera
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // gluLookAt(posição da câmera,  ponto que ela mira,  vetor "cima")
    gluLookAt(CAMERA_X, CAMERA_Y, CAMERA_Z,   // posição
              0.0f,     2.0f,     0.0f,         // alvo (mesma altura)
              0.0f,     1.0f,     0.0f);         // vetor up (eixo Y)

    // Reposiciona o sol no espaço do MUNDO (após a câmera).
    GLfloat dirSol[] = { -0.4f, 1.0f, 0.6f, 0.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, dirSol);

    // Screen shake: tremor da câmera ao bater (decai no idle).
    if (g_shake > 0.001f) {
        float s = g_shake * 0.15f;
        glTranslatef(aleat(-s, s), aleat(-s, s), 0.0f);
    }

    // --------------------------------------------------------
    //  Desenha o chão como um quadrilátero no plano XZ
    //  (servirá de referência visual enquanto o jogo não tem
    //  cenário completo)
    // --------------------------------------------------------
    // O chão 3D só aparece no jogo. Na tela inicial o cenário fica
    // limpo (só céu) e desenhamos um chão 2D enxuto no HUD.
    if (g_estado != INICIO) {
        glColor3f(1.0f, 1.0f, 1.0f);   // branco: mostra as cores da textura
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, g_texturaGrama);
        glBegin(GL_QUADS);
            glNormal3f(0.0f, 1.0f, 0.0f);  // normal p/ cima (luz)
            // Ordem anti-horária vista de cima => face para cima.
            // TexCoord > 1 repete a textura (tiling pixelado no chão).
            glTexCoord2f(0.0f,  0.0f); glVertex3f(-12.0f, 0.0f,  5.0f);
            glTexCoord2f(12.0f, 0.0f); glVertex3f( 12.0f, 0.0f,  5.0f);
            glTexCoord2f(12.0f, 8.0f); glVertex3f( 12.0f, 0.0f, -9.0f);
            glTexCoord2f(0.0f,  8.0f); glVertex3f(-12.0f, 0.0f, -9.0f);
        glEnd();
        glDisable(GL_TEXTURE_2D);
    }

    // Na tela INICIAL deixamos o cenário limpo: sem canos e sem inimigo.
    if (g_estado != INICIO) {
        desenharArvores();  // árvores ao fundo (profundidade)
        desenharCanos();    // obstáculos
        desenharGrama();    // tufos de grama na base (escondem o pé dos canos)
    }

    // Desenha a capivara sobre o cenário
    desenharCapivara();

    // Desenha as asas da capivara
    desenharAsas();

    // Inimigo só aparece quando o jogo já começou
    if (g_estado != INICIO) {
        desenharInimigo();
    }

    // Partículas por cima da cena 3D
    desenharParticulas();

    // Flash da tela (ao bater)
    desenharFlash();

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
        tocarPulo();
    }
}

// Reinicia tudo para um novo jogo e começa a jogar.
void reiniciarJogo() {
    g_capivaraY   = CAPIVARA_Y_INICIAL;
    g_velocidadeY = 0.0f;
    g_pontuacao   = 0;
    g_tempoUltimoPulo = -10.0f;

    inicializarCanos();
    inicializarGrama();
    inicializarArvores();
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
        atualizarGrama(dt);
        atualizarArvores(dt);

        // teto: limita (não mata)
        if (g_capivaraY > ALTURA_TETO) {
            g_capivaraY = ALTURA_TETO;
            g_velocidadeY = 0.0f;
        }

        verificarColisoesCanos();   // canos e chão => game over

        atualizarInimigo(dt);       // abelha foge (não mata mais)
    }

    // Efeitos continuam animando mesmo no game over:
    atualizarParticulas(dt);
    if (g_shake > 0) { g_shake -= dt * 1.5f; if (g_shake < 0) g_shake = 0; }
    if (g_flash > 0) { g_flash -= dt * 1.6f; if (g_flash < 0) g_flash = 0; }

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
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    // SOL DIRECIONAL: o 4º valor = 0 => luz vem de uma DIREÇÃO (raios
    // paralelos), como o sol, em vez de um ponto. Direção: alto e à frente.
    GLfloat dirSol[]   = { -0.4f, 1.0f, 0.6f, 0.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, dirSol);

    // Cor do sol levemente QUENTE (neutraliza o excesso de verde).
    GLfloat difusaSol[]   = { 1.0f, 0.96f, 0.86f, 1.0f };  // luz amarelada
    GLfloat ambienteSol[] = { 0.40f, 0.42f, 0.48f, 1.0f }; // preenchimento frio
    GLfloat specularSol[] = { 0.5f, 0.5f, 0.5f, 1.0f };    // brilho especular
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  difusaSol);
    glLightfv(GL_LIGHT0, GL_AMBIENT,  ambienteSol);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specularSol);

    // Luz ambiente GLOBAL fraca e fria, para as sombras não ficarem pretas.
    GLfloat ambienteGlobal[] = { 0.25f, 0.27f, 0.32f, 1.0f };
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambienteGlobal);

    // glColor define a cor difusa/ambiente do material.
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    // SPECULAR do material: um brilho suave e controlado (shininess).
    GLfloat specMat[] = { 0.35f, 0.35f, 0.35f, 1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specMat);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 24.0f);

    glEnable(GL_NORMALIZE);

    // -------- Visibilidade extra --------
    // Back-face culling: não desenha as faces traseiras dos objetos.
    // (Se algo sumir/ficar furado no jogo, basta remover estas 2 linhas.)
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
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
    carregarModelo(g_capivara, OBJ_CAPIVARA, 1.8f);
    carregarTextura(g_capivara, TEX_CAPIVARA);

    // Carrega a fonte TTF do título (precisa do contexto GL já criado)
    carregarFonte();

    // Cria a textura pixelada da grama (procedural)
    criarTexturaGrama();

    // Inicia o áudio (efeitos sintetizados)
    iniciarAudio();

    // Carrega o modelo das asas (sem textura: cor creme)
    carregarModelo(g_asas, OBJ_ASAS, ASA_TAM);
    // Asa de morcego: tom escuro arroxeado (visível contra o céu)
    g_asas.corR = 0.18f; g_asas.corG = 0.13f; g_asas.corB = 0.22f;

    // Abelha (o "alvo" que a capivara persegue): usa as cores do .mtl
    carregarModelo(g_abelha, OBJ_ABELHA, 0.9f);
    g_abelha.usarCorMaterial = true;

    // Grama 3D da base (usa as cores do .mtl: verde com florzinhas)
    carregarModelo(g_grama, OBJ_GRAMA, GRAMA_TAM);
    g_grama.usarCorMaterial = true;
    g_grama.desaturar = 0.35f;          // grama menos saturada
    inicializarGrama();

    // Cano (Pipe.glb): modelo verde por material; é esticado por segmento
    carregarModelo(g_cano, GLB_CANO, 1.0f);
    g_cano.usarCorMaterial = true;
    g_cano.desaturar = 0.22f;          // verde menos berrante
    prepararCano();                    // separa corpo/borda do cano

    // Árvores de fundo (dois modelos, cores do .mtl, levemente dessaturadas)
    carregarModelo(g_arvore1, OBJ_ARVORE1, ARVORE_TAM);
    carregarModelo(g_arvore2, OBJ_ARVORE2, ARVORE_TAM);
    g_arvore1.usarCorMaterial = true;  g_arvore1.desaturar = 0.30f;
    g_arvore2.usarCorMaterial = true;  g_arvore2.desaturar = 0.30f;
    inicializarArvores();

    // Inicia o loop principal do GLUT (não retorna daqui)
    glutMainLoop();

    return 0;
}
