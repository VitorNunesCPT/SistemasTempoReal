# PRD - Controle de Robo Movel com FreeRTOS

## Contexto e Objetivo
- Desenvolver sistema embarcado para robo movel com 4 sensores ultrassonicos (frente, tras, esquerda, direita) e 4 motores DC controlados via PWM.
- Ajustar dinamicamente a velocidade das rodas seguindo lei de controle exponencial: reduzir duty cycle ao aproximar de obstaculos e aumentar ao afastar.
- Implementar com FreeRTOS (tarefas preemptivas com prioridades e tick), medindo tempos de computacao e avaliando escalonamento (ciclo unico, ciclo menor/maior e livre demanda).

## Escopo (Incluso)
- Tarefas FreeRTOS separadas para cada sensor (SF, ST, SE, SD) e uma tarefa unica de controle dos motores (CM).
- Tarefa de log/telemetria para coletar distancias e tempos de computacao (min/med/max) de cada tarefa.
- Lei de controle exponencial parametrizavel (ganho, faixa de distancias, duty minimo/maximo).
- Definicao de periodos, deadlines, prioridades e tempos de computacao de cada tarefa; testes de tempo de computacao com a biblioteca FreeRTOS.
- Esquematizacao temporal (ciclo maior/menor) com habilitacao/desabilitacao de tarefas e avaliacao de viabilidade.
- Avaliacao em modo livre demanda (tarefas sempre ativas, sem suspender), observando escalonamento e misses.
- Simulacao no Wokwi (ESP32) com codigo e resultados.

## Fora do Escopo
- Navegacao autonoma avancada (planejamento de rota).
- Deteccao de colisao fisica alem do alcance dos sensores.
- Uso de RTOS diferente do FreeRTOS.

## Stakeholders
- Aluno/autor: desenvolvimento e documentacao.
- Professor/avaliador: validacao de requisitos e resultados simulados.

## Requisitos Funcionais
- RF1: Medir distancia dos 4 sensores em periodos definidos, cada um em sua tarefa FreeRTOS (SF, ST, SE, SD), armazenando em variaveis globais protegidas (mutex/secoes criticas).
- RF2: Tarefa de Controle dos Motores (CM) le distancias e calcula duty PWM para 4 motores, aplicando lei exponencial com histerese/limite de variacao para evitar oscilacoes.
- RF3: Cumprir periodicidade/deadline de CM e sensores usando `vTaskDelayUntil` (tick de 1 ms), registrando atrasos/misses se ocorrerem.
- RF4: Log/telemetria para validar tempos de execucao (min/med/max por tarefa), duty resultante e atrasos por hiperperiodo/janela.
- RF5: Inicializacao segura: duty zero ate primeira leitura valida de sensores; tratamento de leitura invalida/timeout com duty seguro.
- RF6: Suportar modos de operacao: (a) ciclo unico, (b) ciclo menor/maior com suspender/retomar tarefas de sensor, (c) livre demanda (tarefas sempre ativas).

## Requisitos Nao Funcionais
- Determinismo temporal: tarefas com deadlines firmes (soft/firm) alinhadas ao tick; misses devem ser detectados e registrados.
- Baixo jitter: uso de `vTaskDelayUntil` para minimizar deriva; evitar chamadas bloqueantes longas (`pulseIn` com timeout reduzido).
- Portabilidade para ESP32 no Wokwi com FreeRTOS nativo.
- Simplicidade de codigo (C/C++/Arduino/FreeRTOS), prioridades documentadas e tamanhos de pilha definidos.

## Arquitetura / Tarefas
- SF/ST/SE/SD: leitura de sensor ultrassonico; atualizar variavel global; sinalizar erro se leitura invalida (timeout/eco ausente); prioridade media; periodo ex. 40–60 ms.
- CM: calcular duty para cada motor a partir das 4 distancias; aplicar saturacao [duty_min, duty_max]; suavizacao; prioridade igual/maior que sensores; periodo ex. 10–20 ms.
- LOG: coletar distancias e tempos (min/med/max) e imprimir a cada janela (ex. 200 ms); prioridade baixa.
- Modo ciclo menor/maior: suspender/retomar tarefas de sensor para ilustrar alocacao de folgas; CM continua rodando no ciclo menor.
- Sincronismo: `vTaskDelayUntil`; tick de 1 ms; para medir em us, usar `esp_timer_get_time()`.
- Protecao: mutex ou secoes criticas breves para leitura/atualizacao de distancias.

## Lei de Controle (parametros a definir/testar)
- Entrada: distancia em cm; Saida: duty (0-100%).
- Forma: `duty = duty_min + (duty_max - duty_min) * exp(-k * (d_ref - d))` ou equivalente, garantindo monotonicidade.
- Parametros: k (ganho), d_ref (distancia alvo/seguranca), duty_min/duty_max, limite de variacao por ciclo (slew rate).
- Tratamento de falha: se leitura invalida/timeout -> duty seguro (stop ou duty_min).

## Cronogramas/Periodos (a definir por teste)
- Estimar Tcomp de cada tarefa via `esp_timer_get_time()`.
- Escolher periodo de sensores (ex. 40–60 ms) e controle (ex. 10–20 ms); definir prioridades coerentes.
- Definir ciclo maior/menor para experimentos com suspender/retomar (ex.: sensores a 50 ms, controle a 20 ms).
- Registrar misses: atraso real > periodo esperado.

## Metricas de Sucesso
- T1: tempos de computacao medidos/documentados (min/med/max) por tarefa; periodos e deadlines coerentes com o orcamento.
- T2: diagramas de ciclo maior/menor com FreeRTOS (habilitar/desabilitar) mostram viabilidade sem perdas de deadline.
- T3: modo livre demanda sem suspensao confirma atendimento das tarefas ou registra misses controlados.
- Duty PWM responde monotonicamente a mudancas de distancia (reduz ao aproximar).
- Jitter dentro de tolerancia definida; misses/overruns registrados e analisados.

## Riscos e Mitigacoes
- Overrun/atraso por `pulseIn` longo -> reduzir timeout, ajustar periodo, ou distribuir leituras.
- Preempcao excessiva por prioridades mal definidas -> revisar prioridades/periodos.
- Oscilacao de duty por ruido -> filtro/histerese.
- Saturacao em distancias curtas -> duty_min seguro/parada.
- Falta de stack em tarefa -> dimensionar stack e monitorar uso.

## Validacao/Testes
- Medir tempos de computacao individuais com FreeRTOS (min/med/max) e validar contra periodos.
- Teste de resposta: variar distancia de um sensor e verificar duty correspondente.
- Teste de falha: simular leitura invalida/timeout e verificar duty seguro.
- Verificacao do diagrama temporal (ciclo maior/menor) com habilitar/desabilitar tarefas.
- Avaliacao em modo livre demanda: registrar misses se ocorrerem.

## Entregaveis
- Codigo C/C++/Arduino com FreeRTOS: tarefas de sensor, controle, log e lei exponencial.
- Diagramas de fluxo temporal (ciclo maior/menor) com periodos/deadlines/tempos medidos.
- Relatorio com ajustes de tempo, resultados de simulacao e respostas aos questionamentos do roteiro.
