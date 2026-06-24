#!/bin/zsh
# ============================================================
#  Script de compilação E execução do Flappy Capivara
#  Usado pelo VSCode (Cmd+Shift+B) e também pelo terminal.
#  Evita problemas com espaço no caminho da pasta.
#
#  Uso:
#    ./build.sh           -> compila e abre o jogo
#    ./build.sh compilar  -> só compila (não abre a janela)
# ============================================================

# Vai para a pasta onde este script está (a raiz do projeto)
cd "$(dirname "$0")" || exit 1

echo "==> Compilando main.cpp ..."

# IMPORTANTE: usamos /usr/bin/clang++ (clang da Apple), que acha o SDK do
# macOS sozinho. Um 'clang++' do Homebrew (llvm) no PATH NÃO encontra o
# GLUT/glut.h e quebra a compilação.
#  -I  : onde achar os headers da Assimp
#  -L  : onde achar a biblioteca da Assimp
#  -lassimp : linka a biblioteca da Assimp
#  miniaudio (áudio) precisa dos frameworks de áudio do macOS + pthread.
/usr/bin/clang++ -std=c++17 main.cpp -o flappy_capivara \
    -I/opt/homebrew/include \
    -L/opt/homebrew/lib -lassimp \
    -framework OpenGL \
    -framework GLUT \
    -framework CoreFoundation \
    -framework CoreAudio \
    -framework AudioToolbox \
    -lpthread \
    -Wno-deprecated-declarations

# $? guarda o código de saída do clang++ (0 = sucesso)
if [ $? -ne 0 ]; then
    echo "==> ERRO na compilacao. Jogo NAO sera executado."
    exit 1
fi

echo "==> Compilado com sucesso!"

# Modo "só compilar": útil para verificar o build sem abrir a janela.
if [ "$1" = "compilar" ]; then
    exit 0
fi

echo "==> Abrindo o jogo ..."

# Executa o jogo recém-compilado
./flappy_capivara
