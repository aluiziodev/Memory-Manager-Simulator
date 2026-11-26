# Memory Manager Simulator

Projeto em C++ para simulação da alocação de memória. A interface em Qt6 exibe blocos alocados, espaços livres e uma escala de endereços. O executável de linha de comando aceita comandos interativos e roteiros de texto.

As classes, estruturas de dados, algoritmos e decisões de interface estão descritos em [Decisões de implementação](docs/implementacao.md). A [verificação contra o enunciado](docs/verificacao.md) registra evidências e pendências de homologação.

## Build base e testes

Precisa de um compilador com suporte a C++17 e CMake 3.20 ou superior.

No windows:

```PowerShell
cmake -S . -B build
cmake --build build
ctest --test-dir build -C Debug --output-on-failure
```

## Linha de comando

Inicie uma sessão interativa e digite um comando por linha, começando por `init 64`. Use `show` para consultar o mapa ou `stats` para consultar as estatísticas. Encerre a entrada com Ctrl+D no Linux e no macOS ou Ctrl+Z seguido de Enter no Windows.

No linux:

```sh
./build/memsim
```

No windows (PowerShell):

```powershell
.\build\Debug\memsim.exe
```

Para executar um roteiro, redirecione o arquivo para a entrada padrão. A saída contém as consultas, os relatórios de compactação e as mensagens de erro, sem o prompt `> `. Linhas vazias e comentários iniciados por `#` são ignorados. Erros de comando são exibidos e a leitura continua.

No linux:

```sh
./build/memsim < tests/cli-session.cmd
```

No windows (PowerShell):

```powershell
Get-Content .\tests\cli-session.cmd | .\build\Debug\memsim.exe
```

## INTERFACE GRÁFICA

Para compilar a interface, instale: Qt 6 Widgets e configure o CMake com `-DMEMSIM_BUILD_VISUAL=ON`. **Siga os passos abaixo conforme o sistema operacional**.

### Linux (Ubuntu 22.04/24.04)

Em um terminal Linux, instale o compilador, CMake, Ninja e Qt 6. O pacote `qt6-wayland` permite usar a janela diretamente em sessões Wayland.

```sh
sudo apt update
sudo apt install build-essential cmake ninja-build qt6-base-dev qt6-wayland
bash scripts/build-qt.sh
./build-qt/memsim-visual
```

O script configura, compila e roda todos os testes.

Se o Qt estiver em um prefixo personalizado, configure com `CMAKE_PREFIX_PATH=/caminho/do/qt bash scripts/build-qt.sh`. Em outras distribuições, instale os pacotes equivalentes de Qt 6 Widgets, compilador, CMake e Ninja.

### Windows (PowerShell)

Instale o [Qt pelo instalador oficial](https://doc.qt.io/qt-6/get-and-install-qt.html). Selecione **Qt 6.x → MinGW 64-bit** (com Qt Widgets) e, em **Developer and Designer Tools**, o **MinGW compatível com essa versão do Qt**, além de Ninja e CMake. Também é possível instalar [CMake separadamente](https://cmake.org/download/); use CMake 3.22 ou mais recente. Qt Creator é opcional: ele sozinho não instala Qt nem o compilador.

Abra o PowerShell **na raiz do repositório** e rode os comandos para compilar e executar:

```powershell
$env:PATH = 'C:\Qt\Tools\mingw1310_64\bin;C:\Qt\6.10.1\mingw_64\bin;' + $env:PATH
cmake -S . -B build-qt -G Ninja -DCMAKE_MAKE_PROGRAM=C:/Qt/Tools/Ninja/ninja.exe -DCMAKE_CXX_COMPILER=C:/Qt/Tools/mingw1310_64/bin/g++.exe -DCMAKE_PREFIX_PATH=C:/Qt/6.10.1/mingw_64 -DMEMSIM_BUILD_VISUAL=ON
cmake --build build-qt
ctest --test-dir build-qt --output-on-failure
.\build-qt\memsim-visual.exe
```

Isso irá gerar o executável `memsim-visual.exe` também pode ser aberto diretamente pelo Explorador de Arquivos e basta ir diretamente na pasta `build-qt` e clicar duas vezes nele para executar normalmente também.

### Como usar a interface visual

Use **Abrir roteiro…** para selecionar um arquivo `.cmd`, existem alguns de exemplo em `examples/`, mas pode rodar qualquer outro que você desejar. A janela valida o roteiro antes de trocar a sessão; comentários, linhas vazias, `show` e `stats` não contam como passos visuais. **Avançar**, **Voltar** e **Reiniciar** controlam a execução. **Reproduzir** avança no intervalo indicado em segundos (`0,5` ou `0.5`); **Pausar** interrompe o temporizador.
