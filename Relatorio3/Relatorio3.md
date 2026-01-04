# Relatorio 3 - Atividades 1 e 2 (FreeRTOS)

## Contextualizacao do Problema
O problema consiste em ler 8 botoes (PORTD) e acionar 8 LEDs correspondentes (PORTB) em um microcontrolador com FreeRTOS. A aplicacao foi dividida em duas tarefas concorrentes: uma tarefa produtora que captura o estado dos botoes e enfileira o valor, e uma tarefa consumidora que retira o valor da fila e atualiza os LEDs. As atividades 1 e 2 exploram comunicacao por fila e, na segunda, adicionam mutex para proteger uma secao critica e observar efeitos de tempo real e risco de deadlock.

## Objetivos
- Implementar o padrao produtor-consumidor com FreeRTOS.
- Medir tempos de computacao e tempos de espera por fila/mutex.
- Avaliar riscos de bloqueio indefinido (deadlock funcional) ao usar `portMAX_DELAY`.
- Comparar o comportamento com fila simples (Atividade1) e fila + mutex (Atividade2).

## Objetivos Especificos
- Ler o estado de 8 botoes e codificar em 1 byte.
- Enfileirar apenas quando houver mudanca de estado.
- Consumir o dado e atualizar os 8 LEDs.
- Instrumentar tempos de espera de escrita/leitura na fila e de espera pelo mutex.
- Analisar impacto de delays nas tarefas e do tamanho da fila (1 item).

## Arquitetura Geral do Sistema
- Hardware: 8 botoes de entrada (PORTD) e 8 LEDs de saida (PORTB).
- Software: FreeRTOS com duas tarefas e uma fila de tamanho 1 (`xQueue_LED`).
- Comunicacao: a tarefa produtora publica um byte com o mapa de LEDs; a tarefa consumidora aplica o valor nos pinos.
- Observabilidade: logs via `Serial` medem tempo de computacao (`micros`) e tempo de espera (`millis`).
- Atividade2 adiciona mutex (`myMutex`) para proteger a funcao `teste()` que usa um recurso global.

## Definicao das Tarefas e Suas Caracteristicas de Tempo Real
| Atividade | Tarefa | Funcao | Prioridade | Bloqueios | Periodicidade/Delay | Observacoes |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | `tarefa_Ler_botoes` | Leitura de botoes + envio para fila | 1 | `xQueueSend` com `portMAX_DELAY` | `vTaskDelay(50ms)` | Envia apenas quando ha mudanca |
| 1 | `LED_acende` | Recebe fila + atualiza LEDs | 1 | `xQueueReceive` com timeout 10 ticks | `vTaskDelay(10ms)` | Delay longo opcional (comentado) |
| 2 | `tarefa_Ler_botoes` | Leitura + `teste()` + envio | 2 | Mutex em `teste()` + `xQueueSend` com `portMAX_DELAY` | `vTaskDelay(50ms)` | Medicao de espera na fila |
| 2 | `LED_acende` | Recebe fila + `teste()` + LEDs | 1 | `xQueueReceive` 10 ticks + mutex em `teste()` | `vTaskDelay(2000ms)` | Delay proposital aumenta espera na fila |

## Avaliacao de Deadlock com Tempos de Espera de Escrita/Leitura (Atividade1)
Nos logs da `Atividade1.ino` (conversa.md), a escrita na fila variou de 0 ms ate ~1950 ms, enquanto a leitura ficou entre 0 e ~7 ms. Isso indica que:
- A fila de tamanho 1 vira o gargalo quando o consumidor demora a processar.
- O uso de `portMAX_DELAY` no `xQueueSend` pode bloquear o produtor indefinidamente se o consumidor parar.
- Nao ha espera circular (apenas fila), portanto o risco principal e um bloqueio unilateral do produtor, nao um deadlock classico. Ainda assim, e um deadlock funcional se o consumidor nunca mais executar.

## Avaliacao do Tempo de Espera para Bloqueio e Uso de portMAX_DELAY
O `portMAX_DELAY` impede timeouts e mascara falhas de consumo. Para evitar bloqueios indefinidos:
- Usar timeout finito no `xQueueSend` (ex.: 100 ms) e tratar falha quando a fila estiver cheia.
- Considerar `xQueueOverwrite` para filas de tamanho 1 quando o ultimo estado basta.
- Para mutex, substituir `portMAX_DELAY` por tempo finito e registrar erro quando o recurso nao puder ser obtido.
Essas medidas reduzem a chance de tarefas ficarem permanentemente bloqueadas e facilitam diagnostico.

## Comparacao de Possiveis Deadlocks: Atividade1 vs Atividade2
- Atividade1: somente fila. O bloqueio ocorre se a fila encher e o produtor ficar esperando para sempre (dependencia unidirecional).
- Atividade2: fila + mutex. Ha dois pontos de bloqueio (fila e mutex), mas no codigo atual o mutex e liberado antes do `xQueueSend` e nao ha espera circular.
- O risco adicional na Atividade2 esta no uso de `portMAX_DELAY` em `xSemaphoreTake`: se uma tarefa reter o mutex e nao liberar, a outra pode bloquear indefinidamente.
- O delay longo no consumidor (2000 ms) aumenta a ocupacao da fila e reproduz esperas de ~2 s na escrita, semelhante ao comportamento observado na Atividade1.

## Conclusao
A Atividade1 mostra claramente o bloqueio do produtor pela fila de tamanho 1, com tempos de espera elevados quando o consumidor demora. A Atividade2 adiciona o mutex e reforca o estudo de concorrencia, mas o risco principal continua sendo o bloqueio indefinido pelo uso de `portMAX_DELAY`. O uso de timeouts finitos e estrategias como overwrite na fila reduz o risco de deadlock funcional e melhora a resiliencia do sistema.
