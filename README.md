# Memory Manager Simulator

Projeto em C++17 para visualização da alocação de memória. A interface em Qt 6 Widgets exibe blocos alocados, espaços livres e uma escala de endereços. O projeto também inclui um executável de linha de comando que exibe uma mensagem de identificação.

## Build base e testes

Requer um compilador com suporte a C++17 e CMake 3.20 ou superior.

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/memsim
```

## Protótipo visual

Para compilar a interface, instale o pacote de desenvolvimento do Qt 6 Widgets (`qt6-base-dev` no Ubuntu 22.04). Se o Qt foi instalado em um caminho personalizado, passe seu prefixo ao CMake com `-DCMAKE_PREFIX_PATH="<prefixo-do-Qt>"`.

```sh
cmake -S . -B build-qt -DMEMSIM_BUILD_VISUAL=ON
cmake --build build-qt
ctest --test-dir build-qt --output-on-failure
./build-qt/memsim-visual
```

Use **Abrir roteiro…** para visualizar o conteúdo de um arquivo `.cmd`, como o [roteiro de exemplo](tests/prototipo-f01.cmd). O arquivo é exibido como texto. **Reproduzir exemplo** percorre três estados predefinidos do mapa, com intervalo de 500 ms. **Pausar** interrompe a reprodução.

O teste automatizado da interface roda com o plugin `offscreen` do Qt.
