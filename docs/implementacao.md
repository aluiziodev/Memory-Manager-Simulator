# Decisões de implementação

## Organização e arquitetura

O projeto usa C++17 e CMake. A biblioteca `memsim-core` reúne o núcleo (`core/`) e a análise e execução dos comandos (`script/`). O executável `memsim` implementa a CLI em `cli/`. O executável `memsim-visual` usa Qt 6 Widgets em `visual/qt/` e compartilha o mesmo núcleo e parser, para que os dois modos executem as mesmas regras. O controlador de passos em `visual/session.*` usa apenas C++ padrão e é testável sem Qt.

O núcleo não lê arquivos nem imprime mensagens. `script::parseLine` transforma uma linha em comando tipado, `script::execute` despacha esse comando ao núcleo, e a CLI ou a janela formata o resultado para seu próprio meio de apresentação. Essa separação corresponde ao padrão Command na representação dos comandos, sem uma hierarquia de classes. A escolha do algoritmo usa um `enum` e um ramo de seleção; não foi criada uma hierarquia Strategy porque as quatro opções compartilham quase todo o percurso dos buracos.

## Estrutura da memória

`Block` guarda endereço inicial, tamanho e ID. O ID zero representa um buraco; IDs positivos identificam alocações. Cada intervalo é semiaberto `[start, start + size)`, portanto o endereço final exclusivo pode ser igual à capacidade. `MemoryManager` guarda um `std::vector<Block>` ordenado por endereço, a capacidade, o último ID emitido e o ponteiro Next Fit. O vetor cobre toda a memória, não contém blocos vazios nem buracos adjacentes. A implementação verifica essas invariantes com asserções ao concluir mutações bem-sucedidas.

O vetor representa intervalos, não um byte por posição. Isso evita alocar um vetor proporcional à capacidade apenas para simular o mapa. A CLI gera um caractere por byte ao exibir `show`; a interface Qt calcula a largura de cada intervalo a partir de seu endereço e da largura disponível. Sua tabela permite consultar inclusive blocos estreitos demais para receber um rótulo no desenho.

`init` cria um buraco único e reinicia contador de IDs e ponteiro. Uma alocação bem-sucedida divide o buraco escolhido, se houver sobra, atribui o próximo ID e atualiza o ponteiro para o fim do bloco. Uma falha preserva todo o estado e não consome ID. `freeId` e `freeAddr` aceitam apenas um bloco alocado existente; após a liberação, unem os buracos vizinhos. `getStats` calcula ocupado, livre, número de buracos, maior buraco e percentual de uso a partir dos intervalos atuais.

## Algoritmos de alocação e compactação

First Fit percorre os buracos por endereço e usa o primeiro em que o pedido cabe. Best Fit examina todos e escolhe o menor que comporta o pedido. Worst Fit examina todos e escolhe o maior. Como o vetor já está ordenado, empates em Best e Worst mantêm o menor endereço. `StepResult` conserva a sequência examinada e uma cópia do buraco escolhido antes da divisão.

Next Fit inicia a busca no primeiro buraco cujo fim é estritamente maior que o ponteiro. Se o ponteiro estiver dentro de um buraco, esse buraco é considerado inteiro e a alocação ocorre em seu início. A busca dá uma volta circular, sem examinar o mesmo buraco duas vezes. Quando o ponteiro chega à capacidade, a busca recomeça no primeiro buraco. Liberar e falhar em uma alocação não mudam o ponteiro. Um `alloc` bem-sucedido com qualquer algoritmo atualiza o ponteiro, pois ele registra o fim do último bloco alocado.

`defrag INIT` cola os blocos no fim, deixando o buraco no início; `END` cola os blocos no início; `OPT` avalia cada corte `k`, com os `k` primeiros blocos no início e os demais no fim. O custo é a soma dos tamanhos dos blocos cujo endereço inicial mudou. Prefixos e sufixos de custo permitem avaliar todos os cortes em tempo linear; em empate, vence o maior `k`. Os três modos preservam a ordem relativa e os IDs e zeram o ponteiro. O resultado registra origem, destino e tamanho de cada bloco movido, além dos bytes deslocados.

## Comandos, erros e apresentação

O parser aceita comandos e opções sem distinção entre maiúsculas e minúsculas, espaços externos e arquivos CRLF. Ignora linhas vazias e comentários iniciados por `#`. Valida quantidade de argumentos, opções e faixa numérica antes de criar um comando. No roteiro visual, `show` e `stats` são analisados sintaticamente e depois omitidos; o primeiro passo executável precisa ser `init`. Erros de sintaxe impedem a troca do arquivo carregado. Erros de execução contam como passos, mostram a mensagem e preservam o estado.

`StepResult` usa valores próprios, não referências a blocos que podem ser movidos por uma mutação. Ele contém erro, ponteiros antes/depois, buracos examinados e escolhido, bloco alocado ou liberado, regiões unidas e movimentos de compactação. A CLI formata o mapa, as estatísticas e erros em português. `show` inclui mapa, blocos ativos e estatísticas; `stats` inclui apenas as estatísticas. O percentual usa ponto e duas casas, independente da localidade. `init`, `alloc` e liberações bem-sucedidos não emitem texto, conforme o exemplo do enunciado.

## Interface e histórico

`visual::Session` guarda uma entrada inicial e uma cópia completa de `MemoryManager` e do resultado por passo. Voltar remove a entrada atual; avançar executa de novo o comando a partir da cópia restaurada. Essa forma simples de Memento preserva blocos, sequência de IDs, ponteiro, erro e destaques após falhas ou um segundo `init`. Reiniciar mantém o roteiro e restaura o estado anterior ao primeiro passo.

`Window` organiza seletor de arquivo, controles e um único `QTimer`. `MemoryMap` desenha escala, blocos, buracos, ponteiro e contornos das mudanças; a cor depende do ID e permanece estável após compactação. `StepPanel` mostra comando, estatísticas, busca, movimentos e erros. Falhas aparecem em um alerta destacado, inclusive a falta de espaço contíguo com livre total e maior buraco. O intervalo de reprodução aceita ponto ou vírgula decimal e é convertido para milissegundos entre 1 e `INT_MAX`. Avançar, Voltar, Reiniciar, carregar outro roteiro, Pausar e chegar ao fim interrompem a reprodução.

## Build e verificação

O Qt é opcional no CMake: `MEMSIM_BUILD_VISUAL=OFF` compila CLI, núcleo e testes sem Qt. No Windows com MinGW, o build copia as DLLs do compilador para junto dos executáveis. Com Qt ligado, `windeployqt` copia as bibliotecas e plugins da interface, inclusive o plugin `offscreen` usado pelos testes. No Linux, `scripts/build-linux.sh` usa o Qt do sistema e executa os mesmos testes; `.github/workflows/linux.yml` repete esse processo no Ubuntu 24.04 após o envio das alterações ao repositório.

Os testes reproduzem a sessão de 64 bytes, a tabela Next Fit de 32 bytes, a divergência dos quatro algoritmos, os três modos de compactação e a falha por fragmentação. Uma referência independente que modela cada byte compara 16 mil operações determinísticas com o núcleo. Testes da interface verificam carga de roteiro, histórico, mapa, painel, intervalo, pausa e estados finais dos exemplos nos modos CLI e visual.

A execução manual da janela nativa e a repetição do build em outra máquina da equipe ainda fazem parte da homologação final.
