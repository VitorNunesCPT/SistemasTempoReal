# PRD - Controle de Robo Movel com Executivo Ciclico

## Contexto e Objetivo
- Desenvolver sistema embarcado para robo movel com 4 sensores ultrassonicos (frente, tras, esquerda, direita) e 4 motores DC controlados via PWM.
- Ajustar dinamicamente a velocidade das rodas seguindo lei de controle exponencial: reduzir duty cycle ao aproximar de obstaculos e aumentar ao afastar.
- Implementar executivo ciclico deterministic (sem RTOS) para leitura dos sensores e controle dos motores, garantindo previsibilidade temporal.

## Escopo (Incluso)
- Leitura periodica de cada sensor ultrassonico como tarefa independente.
- Calculo de velocidade/duty PWM para os 4 motores em uma tarefa unica de controle.
- Lei de controle exponencial parametrizavel (ganho, faixa de distancias, duty minimo/maximo).
- Definicao de periodos, deadlines e tempos de computacao de cada tarefa; testes de tempo de computacao.
- Esquematizacao temporal (ciclo maior/menor) e verificacao de viabilidade no executivo ciclico.
- Simulacao no Wokwi (Arduino/ESP32) com codigo e resultados.

## Fora do Escopo
- Navegacao autonoma avancada (planejamento de rota).
- Deteccao de colisao fisica alem do alcance dos sensores.
- Uso de RTOS (FreeRTOS ou outros).

## Stakeholders
- Aluno/autor: desenvolvimento e documentacao.
- Professor/avaliador: validacao de requisitos e resultados simulados.

## Requisitos Funcionais
- RF1: Medir distancia dos 4 sensores em periodos definidos (SF, ST, SE, SD) e armazenar em variaveis globais protegidas contra inconsistencia de leitura.
- RF2: Tarefa de Controle dos Motores (CM) le distancias e calcula duty PWM para 4 motores, aplicando lei exponencial com histerese/limite para evitar oscilacoes.
- RF3: Atualizar comandos PWM dentro da periodicidade e deadline da CM.
- RF4: Log/telemetria minima para validar tempos de execucao e duty resultante em simulacao.
- RF5: Inicializacao segura: duty zero ate primeira leitura valida de sensores.

## Requisitos Nao Funcionais
- Determinismo temporal: todas as tarefas cabem no slot do executivo ciclico, sem overrun.
- Tempo real "soft" com deadlines firmes definidos por teste de computacao.
- Portabilidade para Arduino ou ESP32 no Wokwi.
- Simplicidade de codigo (C/C++/Arduino) e ausencia de RTOS.

## Arquitetura / Tarefas
- SF/ST/SE/SD: leitura de sensor ultrassonico; atualizar variavel global; sinalizar erro se leitura invalida (timeout/eco ausente).
- CM: calcular duty para cada motor a partir das 4 distancias; aplicar saturacao [duty_min, duty_max]; suavizacao (filtro exponencial ou limite de variacao por ciclo).
- Executivo ciclico: tabela estatica de chamadas (frame maior/menor) com slots para SF, ST, SE, SD, CM.

## Lei de Controle (parametros a definir/testar)
- Entrada: distancia em cm; Saida: duty (0-100%).
- Forma: `duty = duty_min + (duty_max - duty_min) * exp(-k * (d_ref - d))` ou equivalente, garantindo monotonicidade.
- Parametros: k (ganho), d_ref (distancia alvo/seguranca), duty_min/duty_max, limite de variacao por ciclo (slew rate).
- Tratamento de falha: se leitura invalida -> duty seguro (ex. duty_min ou stop).

## Cronogramas/Periodos (a definir por teste)
- Estimar Tcomp de cada tarefa (sensor/motor) via medicao.
- Escolher periodo de sensores (ex. 20-50 ms) e controle (ex. igual ou multiplo).
- Definir hyperperiod/ciclo maior e frames menores; garantir soma Tcomp_slot <= Tframe.

## Metricas de Sucesso
- T1: tempos de computacao medidos/documentados; periodos e deadlines definidos coerentes com o orcamento.
- T2: diagramas de ciclo maior/menor mostram viabilidade (soma Tcomp_slot <= Tframe sem estouro).
- T3: simulacao do executivo ciclico confirma atendimento de periodicidade/deadline de cada tarefa sem overrun, com logs/telemetria.
- Duty PWM responde monotonicamente a mudancas de distancia (reduz ao aproximar).
- Tarefas de sensor e controle com jitter dentro de tolerancia (a definir).

## Riscos e Mitigacoes
- Overrun por leituras ultrassonicas lentas -> medir tempo e ajustar periodo/frame.
- Oscilacao de duty por ruido no sensor -> filtro simples ou histerese.
- Saturacao de motores em distancias muito curtas -> duty_min seguro (ou parada).
- Calibracao de k inadequada -> ensaios em simulacao com distancias variadas.

## Validacao/Testes
- Medir tempos de computacao individuais (micros/millis) e validar contra orcamento.
- Teste de resposta: variar distancia de um sensor e verificar duty correspondente.
- Teste de falha: simular leitura invalida e verificar duty seguro.
- Verificacao do diagrama temporal (ciclo maior/menor) cobrindo todas as tarefas.

## Entregaveis
- Codigo C/C++/Arduino do executivo ciclico, tarefas de sensor, tarefa de controle e lei exponencial.
- Diagramas de fluxo temporal (ciclo maior/menor) com periodos/deadlines/tempos medidos.
- Relatorio com ajustes de tempo, resultados de simulacao e respostas aos questionamentos do roteiro.
