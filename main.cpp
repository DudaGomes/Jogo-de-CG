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

// Biblioteca Assimp — carrega o modelo 3D (.obj) da capivara
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

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
const float CAPIVARA_X = -2.0f;   // um pouco à esquerda
const float CAPIVARA_Y =  2.5f;   // altura inicial

// ============================================================
//  MODELO 3D DA CAPIVARA (carregado de um arquivo .obj)
//  Usamos a biblioteca Assimp para ler o arquivo e guardamos
//  o resultado em variáveis globais para desenhar a cada frame.
// ============================================================
const char* CAMINHO_MODELO = "Capybara/Capybara.obj";

// Ponteiro para a cena 3D carregada pela Assimp (malha, vértices...)
const aiScene* g_cena = nullptr;

// Para encaixar a capivara na tela, guardamos o centro e a escala
// calculados a partir do "bounding box" (caixa que envolve o modelo).
float g_centroX = 0, g_centroY = 0, g_centroZ = 0;
float g_escala  = 1.0f;

// ------------------------------------------------------------
//  Calcula a caixa que envolve o modelo (menor e maior ponto)
//  para podermos centralizar e redimensionar a capivara.
// ------------------------------------------------------------
void calcularBoundingBox() {
    // Inicializa os extremos com valores bem grandes/pequenos
    float minX =  1e9, minY =  1e9, minZ =  1e9;
    float maxX = -1e9, maxY = -1e9, maxZ = -1e9;

    // Percorre todas as malhas e todos os vértices do modelo
    for (unsigned int m = 0; m < g_cena->mNumMeshes; m++) {
        const aiMesh* malha = g_cena->mMeshes[m];
        for (unsigned int v = 0; v < malha->mNumVertices; v++) {
            aiVector3D p = malha->mVertices[v];
            if (p.x < minX) minX = p.x;  if (p.x > maxX) maxX = p.x;
            if (p.y < minY) minY = p.y;  if (p.y > maxY) maxY = p.y;
            if (p.z < minZ) minZ = p.z;  if (p.z > maxZ) maxZ = p.z;
        }
    }

    // Centro = meio da caixa
    g_centroX = (minX + maxX) / 2.0f;
    g_centroY = (minY + maxY) / 2.0f;
    g_centroZ = (minZ + maxZ) / 2.0f;

    // Escala = faz a maior dimensão virar ~2 unidades no jogo
    float tamX = maxX - minX;
    float tamY = maxY - minY;
    float tamZ = maxZ - minZ;
    float maior = tamX;
    if (tamY > maior) maior = tamY;
    if (tamZ > maior) maior = tamZ;
    if (maior > 0) g_escala = 2.8f / maior;  // tamanho da capivara na tela
}

// ------------------------------------------------------------
//  Carrega o modelo do disco. Chamado uma única vez no main().
//  Retorna true se deu certo.
// ------------------------------------------------------------
bool carregarCapivara() {
    // aiImportFile lê o arquivo e já faz pós-processamento:
    //  - Triangulate: transforma qualquer face em triângulos
    //  - GenSmoothNormals: gera normais (necessárias p/ iluminação)
    g_cena = aiImportFile(CAMINHO_MODELO,
                          aiProcess_Triangulate |
                          aiProcess_GenSmoothNormals);

    if (!g_cena || g_cena->mNumMeshes == 0) {
        printf("ERRO: nao consegui carregar '%s'\n", CAMINHO_MODELO);
        printf("Coloque o arquivo .obj nessa pasta e tente de novo.\n");
        return false;
    }

    calcularBoundingBox();
    printf("Modelo carregado: %u malha(s).\n", g_cena->mNumMeshes);
    return true;
}

// ============================================================
//  Desenha a capivara: percorre cada triângulo do modelo
//  carregado e envia os vértices/normais para o OpenGL.
// ============================================================
void desenharCapivara() {
    if (!g_cena) return;  // modelo não carregado, não desenha nada

    glPushMatrix();

    // 1) Posiciona a capivara no mundo (plano XY do jogo)
    glTranslatef(CAPIVARA_X, CAPIVARA_Y, 0.0f);

    // 2) Gira a capivara para ficar de PERFIL, olhando para a direita
    //    (90° no eixo Y deixa a capivara olhando para a direita, +X)
    glRotatef(90.0f, 0.0f, 1.0f, 0.0f);

    // 3) Aplica a escala calculada (encaixa na tela)
    glScalef(g_escala, g_escala, g_escala);

    // 4) Centraliza o modelo na origem (tira o deslocamento dele)
    glTranslatef(-g_centroX, -g_centroY, -g_centroZ);

    // Cor base da capivara (caramelo) — depois a textura/luz refina
    glColor3f(0.62f, 0.47f, 0.32f);

    // Percorre cada malha do modelo
    for (unsigned int m = 0; m < g_cena->mNumMeshes; m++) {
        const aiMesh* malha = g_cena->mMeshes[m];

        // Cada "face" já é um triângulo (por causa do Triangulate)
        glBegin(GL_TRIANGLES);
        for (unsigned int f = 0; f < malha->mNumFaces; f++) {
            const aiFace& face = malha->mFaces[f];

            // Para cada um dos 3 vértices do triângulo
            for (unsigned int i = 0; i < face.mNumIndices; i++) {
                unsigned int idx = face.mIndices[i];

                // Normal (direção da superfície) — usada pela iluminação
                if (malha->HasNormals()) {
                    aiVector3D n = malha->mNormals[idx];
                    glNormal3f(n.x, n.y, n.z);
                }

                // Posição do vértice
                aiVector3D p = malha->mVertices[idx];
                glVertex3f(p.x, p.y, p.z);
            }
        }
        glEnd();
    }

    glPopMatrix();
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

    // Desenha a capivara sobre o cenário
    desenharCapivara();

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
void teclado(unsigned char tecla, int x, int y) {
    if (tecla == 27) {  // ESC — fecha o jogo
        exit(0);
    }
    // Futuramente: espaço/clique para a capivara pular
}

// ============================================================
//  Callback idle — chamado quando não há eventos pendentes.
//  Aqui faremos a atualização da física no futuro.
// ============================================================
void idle() {
    // Por enquanto só pede redesenho contínuo
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
    glutIdleFunc(idle);          // loop ocioso

    // Aplica as configurações iniciais do OpenGL
    inicializarOpenGL();

    // Carrega o modelo 3D da capivara (uma única vez)
    carregarCapivara();

    // Inicia o loop principal do GLUT (não retorna daqui)
    glutMainLoop();

    return 0;
}
