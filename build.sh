#!/bin/zsh
# ============================================================
#  Script de compilação E execução do Flappy Capivara
#  Usado pelo VSCode (Cmd+Shift+B) e também pelo terminal.
#  Evita problemas com espaço no caminho da pasta.
# ============================================================

# Vai para a pasta onde este script está (a raiz do projeto)
cd "$(dirname "$0")" || exit 1

echo "==> Compilando main.cpp ..."

# Compila o jogo com as flags do OpenGL/GLUT (macOS) + Assimp.
#  -I  : onde achar os headers da Assimp
#  -L  : onde achar a biblioteca da Assimp
#  -lassimp : linka a biblioteca da Assimp
clang++ -std=c++17 main.cpp -o flappy_capivara \
    -I/opt/homebrew/include \
    -L/opt/homebrew/lib -lassimp \
    -framework OpenGL \
    -framework GLUT \
    -Wno-deprecated-declarations

# $? guarda o código de saída do clang++ (0 = sucesso)
if [ $? -ne 0 ]; then
    echo "==> ERRO na compilacao. Jogo NAO sera executado."
    exit 1
fi

echo "==> Compilado com sucesso! Abrindo o jogo ..."

# Executa o jogo recém-compilado
./flappy_capivara
