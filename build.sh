#!/bin/zsh
# ============================================================
#  Script de compilação E execução do Flappy Capivara
#  Usado pelo VSCode (Cmd+Shift+B) e também pelo terminal.
#  Evita problemas com espaço no caminho da pasta.
# ============================================================

# Vai para a pasta onde este script está (a raiz do projeto)
cd "$(dirname "$0")" || exit 1

echo "==> Compilando main.cpp ..."

# Compila o jogo com as flags do OpenGL/GLUT do macOS
clang++ -std=c++17 main.cpp -o flappy_capivara \
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
