# Memory Manager Simulator

Projeto em C++17 para simulação da alocação de memória. A interface em Qt 6 Widgets exibe blocos alocados, espaços livres e uma escala de endereços. O executável de linha de comando aceita comandos interativos e roteiros de texto.

## Build base e testes

Requer um compilador com suporte a C++17 e CMake 3.20 ou superior.

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## Linha de comando

Inicie uma sessão interativa e digite um comando por linha, começando por `init 64`. Use `show` para consultar o mapa ou `stats` para consultar as estatísticas. Encerre a entrada com Ctrl+D no Linux e no macOS ou Ctrl+Z seguido de Enter no Windows.

```sh
./build/memsim
```

Para executar um roteiro, redirecione o arquivo para a entrada padrão. A saída contém as consultas, os relatórios de compactação e as mensagens de erro, sem o prompt `> `. Linhas vazias e comentários iniciados por `#` são ignorados. Erros de comando são exibidos e a leitura continua.

```sh
./build/memsim < tests/cli-session.cmd
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
