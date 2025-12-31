# PRD - Concorrencia e comunicacao entre tarefas com FreeRTOS (Roteiro 3)

## Contexto e Objetivo
- Implementar demonstracao de concorrencia e comunicacao entre tarefas FreeRTOS usando filas (queues) e, em fase posterior, mutex.
- Caso base: PORTD com botoes (entrada) produz eventos; PORTB com LEDs (saida) consome eventos, mantendo correspondencia pino-a-pino.
- Avaliar riscos de deadlock e tempos de espera em operacoes de escrita e leitura em fila (Atividade 1). Atividade 2 (mutex) sera tratada apos concluir Atividade 1.

## Escopo (Incluso)
- Duas tarefas FreeRTOS: produtora (`_Ler_botoes`) lendo PORTD e consumidora (`LED_acende`) escrevendo em PORTB.
- Fila FIFO para compartilhar um byte (estado da porta) entre as tarefas.
- Ajuste e documentacao de prioridades das tarefas, tamanhos de pilha, tempos de espera em `xQueueSend` e `xQueueReceive`.
- Avaliacao de risco de deadlock associada ao tempo de bloqueio (incluindo uso de `portMAX_DELAY`).
- Ambiente alvo: Arduino/ESP32 em simulador ou hardware real.

## Fora do Escopo
- Debounce analogico/filtragem de hardware dos botoes.
- Tratamento de multitouch complexo ou multiplexacao de portas alem de PORTD/PORTB.
- Sincronizacao via outros mecanismos (event groups, stream buffers) nesta fase.

## Stakeholders
- Aluno/autor: desenvolvimento e documentacao.
- Professor/avaliador: validacao de requisitos e resultados de teste/simulacao.

## Requisitos Funcionais
- RF1: Inicializar fila global (`QueueHandle_t`) com capacidade adequada (minimo 1 item de 1 byte) antes de iniciar o scheduler.
- RF2: Tarefa `_Ler_botoes` deve ler o estado de PORTD e publicar na fila com `xQueueSend` (ou `xQueueOverwrite` se usar fila de tamanho 1), respeitando um tempo maximo de espera configurado.
- RF3: Tarefa `LED_acende` deve consumir da fila com `xQueueReceive`, aplicar o byte recebido diretamente a PORTB e registrar se houve timeout ou falha de leitura.
- RF4: Definir prioridades relativas das tarefas para evitar inversao indevida e documentar a escolha (ex.: consumidora igual ou maior que produtora).
- RF5: Instrumentar verificacao de fila vazia/cheia (`uxQueueMessagesWaiting`) para apoiar a analise de deadlock ou starvation.

## Requisitos Nao Funcionais
- Determinismo temporal: tempos de espera em fila devem ser finitos e justificaveis; bloquear indefinidamente so se demonstrado seguro.
- Baixo overhead: fila de byte unico, operacoes curtas por ciclo.
- Portabilidade: codigo compativel com ambiente Arduino/ESP32 e FreeRTOS padrao.
- Legibilidade: comentarios sucintos sobre configuracao de fila, prioridades e tempos de espera.

## Arquitetura / Tarefas
- `_Ler_botoes` (produtora): le PORTD, publica byte; pode usar periodo fixo com `vTaskDelayUntil`.
- `LED_acende` (consumidora): bloqueia em `xQueueReceive`, aplica byte em PORTB; opcionalmente registra estatisticas de fila.
- Opcional LOG: imprimir eventos/erros para auxiliar nos testes (prioridade baixa).
- Sincronismo: fila como mecanismo de passagem de mensagens; sem mutex nesta fase (Atividade 2 depois).

## Metricas de Sucesso
- Eventos de botoes refletem corretamente nos LEDs correspondentes (PORTD -> PORTB).
- Nenhuma tarefa permanece bloqueada sem progresso indevido (avaliar tempos de espera vs. `portMAX_DELAY`).
- Uso de CPU e fila observados sem overrun: fila nao cresce indefinidamente, consumidor acompanha produtor.
- Documentacao de prioridades, tempos de espera e comportamento observado (logs/observacoes).

## Riscos e Mitigacoes
- Deadlock por `portMAX_DELAY` com produtor/consumidor parados -> limitar tempo de espera ou garantir que outra tarefa sempre libera.
- Fila cheia por excesso de producao -> definir tamanho adequado ou usar `xQueueOverwrite`.
- Fila vazia frequente causando latencia -> ajustar periodos/prioridades ou permitir bloqueio com timeout maior.
- Bounce de botoes gerando eventos falsos -> opcional debounce via software rapido ou ignorar para escopo basico.

## Validacao/Testes
- Teste funcional: pressionar cada botao de PORTD e verificar LED correspondente em PORTB.
- Teste de bloqueio: variar tempos de espera de `xQueueSend`/`xQueueReceive` (incluindo `portMAX_DELAY`) e observar se tarefas ficam presas.
- Teste de throughput: produzir eventos rapidos e checar se fila satura ou se consumidor acompanha.
- Logar contagem de mensagens na fila e ocorrencias de timeout para analise.

## Entregaveis
- Codigo C/C++ (Arduino/ESP32) com duas tarefas e fila configurada (Atividade 1).
- Notas/observacoes sobre tempos de espera, prioridades e evidencias de ausencia/presenca de deadlock.
- (Futuro) Extensao com mutex para Atividade 2, apos conclusao da fase com queue.
