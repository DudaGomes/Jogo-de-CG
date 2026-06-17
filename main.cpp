// ============================================================
//  Flappy Capivara — Jogo de Computação Gráfica
//  Fase 1: Janela GLUT, câmera 3D e chão
// ============================================================

// Inclui as bibliotecas do OpenGL e GLUT para macOS
#include <GLUT/glut.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <cstdlib>   // exit()

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
        glVertex3f(-10.0f, 0.0f, -5.0f);
        glVertex3f( 10.0f, 0.0f, -5.0f);
        glVertex3f( 10.0f, 0.0f,  5.0f);
        glVertex3f(-10.0f, 0.0f,  5.0f);
    glEnd();

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

    // Inicia o loop principal do GLUT (não retorna daqui)
    glutMainLoop();

    return 0;
}
