# Interface Qt: decisões de implementação

A simulação visual usa Qt 6 Widgets em um executável separado (`memsim-visual`). A CLI e a janela compartilham o mesmo `MemoryManager` e o mesmo parser. O controlador em `visual/session.*` não depende de Qt: recebe o roteiro validado, executa cada `Command` e guarda uma cópia completa do núcleo e do resultado a cada passo. Voltar remove a última cópia; avançar novamente executa o comando a partir do estado restaurado. Isso preserva IDs, ponteiro Next Fit, erro e destaques após um `init` repetido ou uma falha.

A janela em `visual/qt/window.*` coordena o seletor de arquivo, os botões e um único `QTimer`. A leitura valida o arquivo inteiro antes de trocar a sessão. Um arquivo inválido preserva a sessão anterior. O intervalo é analisado em `playback_interval.*`, aceitando ponto ou vírgula decimal e apenas valores que correspondam a 1 até `INT_MAX` milissegundos.

O `MemoryMap` em `visual/qt/memory_map.*` converte os intervalos `[início, fim)` para posições proporcionais à largura. A cor de cada bloco depende do ID; buracos usam hachura. A tabela ao lado do mapa permite consultar ID, início e tamanho mesmo quando o desenho de um bloco ocupa menos de um pixel. O ponteiro atual é marcado no mapa. Contornos distinguem buraco escolhido, alocação, liberação, coalescência e destinos da compactação.

O `StepPanel` em `visual/qt/step_panel.*` mostra estatísticas e detalhes do `StepResult`. O resultado já contém cópias do buraco escolhido antes da divisão, buracos examinados, blocos liberados e movidos, além dos ponteiros antes e depois. O painel mostra origem e destino de cada movimento. Em falhas, a mensagem completa aparece em um alerta destacado e o comando e o indicador do passo mudam de cor. Voltar ou avançar para um passo válido remove o alerta.

Os roteiros em `examples/` cobrem Next Fit, OPT, escolha diferente entre algoritmos e erro por fragmentação. Os testes automatizados verificam navegação até o fim e volta ao início, mapa de 8 e 4096 bytes, painel, intervalo e pausa, além dos estados finais na CLI e no controlador visual.
