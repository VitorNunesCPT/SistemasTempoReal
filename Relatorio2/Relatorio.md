# Relatorio – Experimento 2: Controle com FreeRTOS (4 sensores, 4 motores)

## 1. Objetivo e Roteiro
- Problema: controle de distancia com 4 sensores HC-SR04 e 4 motores DC, lei de controle exponencial via PWM.
- Abordagem: FreeRTOS em ESP32 (Wokwi), tarefas preemptivas com prioridades; avaliacao em três modos: ciclo unico continuo, ciclo menor/maior com suspender/retomar sensores, e livre demanda.
- Itens do roteiro:
  - T1: Caracterizar tempo real e medir Tcomp de cada tarefa usando FreeRTOS.
  - T2: Diagramar execucao (ciclo unico e ciclo menor/maior) com habilitacao/desabilitacao de tarefas e avaliar viabilidade.
  - T3: Avaliar desempenho em livre demanda (tarefas sempre ativas, sem suspender).
  - T4: Registrar ajustes de tempo, resultados e respostas aos questionamentos.

## 2. Sistema e Codigo Base
- Arquivo: `Relatorio2/codigoProposto.ino`.
- Tarefas FreeRTOS:
  - Sensores: SF, ST, SE, SD (periodo alvo 50 ms, prioridade 2).
  - Controle: CTRL (periodo alvo 20 ms, prioridade 3).
  - Log: LOG (periodo 200 ms, prioridade 1).
- Temporizacao:
  - Tick FreeRTOS: 1 ms (padrao ESP32).
  - `vTaskDelayUntil` em todas as tarefas periodicas.
  - Timeout de leitura: `TIMEOUT_PULSE_US = 8000` para caber no periodo.
- Lei de controle: exponencial saturada (`DUTY_MIN/MAX`), parada abaixo de `DISTANCIA_STOP_CM`.
- Variaveis compartilhadas protegidas por seccao critica (portMUX) para leituras de distancia.

## 3. Tarefa 1 – Caracteristicas de Tempo Real e Medicoes
- Metodologia: cada tarefa mede duracao com `esp_timer_get_time()`, calcula slack (`periodoUs - dur`) e conta misses (slack < 0). Log a cada 200 ms inclui Tmax, ciclos, misses e slack mais recente.
- Periodos e deadlines assumidos:

| Tarefa | Periodo (ms) | Deadline (ms) | Prioridade | Timeout leitura | Observacoes |
| --- | --- | --- | --- | --- | --- |
| SF | 50 | 50 | 2 | 8000 us | Sensor frente |
| ST | 50 | 50 | 2 | 8000 us | Sensor tras |
| SE | 50 | 50 | 2 | 8000 us | Sensor esquerda |
| SD | 50 | 50 | 2 | 8000 us | Sensor direita |
| CTRL | 20 | 20 | 3 | - | Calcula PWM para 4 motores |
| LOG | 200 | 200 | 1 | - | Telemetria/estatisticas |

- Tabela de resultados (preencher com log):

| Tarefa | Tcomp min (us) | Tcomp med (us) | Tcomp max (us) | Slack ultimo (us) | Misses | Observacoes |
| --- | --- | --- | --- | --- | --- | --- |
| SF |  |  |  |  |  |  |
| ST |  |  |  |  |  |  |
| SE |  |  |  |  |  |  |
| SD |  |  |  |  |  |  |
| CTRL |  |  |  |  |  |  |
| LOG |  |  |  |  |  |  |

- Analise de viabilidade: verificar misses; se `Tcomp max` aproxima o periodo, ajustar timeout ou periodos.

## 4. Tarefa 2 – Diagramas e Modos Ciclo Unico x Ciclo Menor/Maior
- Diagrama 1 (ciclo unico continuo): todas as tarefas ativas; periodos fixos (20/50/200 ms); mostrar linha do tempo com execucoes de CTRL a cada 20 ms e sensores a cada 50 ms; LOG a cada 200 ms.
- Diagrama 2 (ciclo menor/maior com suspender/retomar):
  - Ciclo menor: CTRL a cada 20 ms.
  - Ciclo maior: sensores a cada 50 ms, podendo ser suspensos e retomados para liberar folga (ex.: suspender por 100 ms, retomar por 100 ms).
  - Avaliar viabilidade: comparar slack/misses antes/depois da suspensao; garantir que leituras nao atrasem alem do periodo definido.
- Campos a marcar:
  - [ ] Ciclo unico atende deadlines (sem misses).
  - [ ] Ciclo menor/maior com suspender/retomar atende deadlines (sem misses relevantes).
  - Observacoes sobre jitter e folga.

## 5. Tarefa 3 – Livre Demanda
- Condicao: manter todas as tarefas ativas continuamente (modo atual), sem suspender sensores.
- Avaliar logs de misses/slack; observar se prioridades (CTRL > sensores > LOG) evitam atrasos no controle.
- Resultados a registrar: se ha misses, em qual tarefa e qual o slack negativo; ajustar timeout/periodos se necessario.

## 6. Resultados de Simulacao (Wokwi)
- Incluir:
  - Logs `[LOG]` com distancias, Tmax, ciclos, misses, slack.
  - Se houver overruns/misses, indicar a tarefa e o valor de slack negativo.
  - Screenshots/trechos do serial monitor.
- Topologia: ESP32 + 4 HC-SR04 (frente, tras, esquerda, direita) + atuadores/LEDs conforme montagem.

## 7. Ajustes de Tempo e Parametros
- Ajustes implementados:
  - Timeout de `pulseIn` reduzido para 8000 us.
  - Periodos: sensores 50 ms; controle 20 ms; log 200 ms.
  - Prioridades: CTRL (3) > sensores (2) > LOG (1).
  - Estatisticas: min/max, slack, misses por tarefa a cada janela de 200 ms.
- Ajustes pendentes/avaliar:
  - Se persistirem misses nos sensores, aumentar periodo (ex. 60 ms) ou reduzir timeout.
  - Se CTRL sofrer misses, aumentar prioridade ou revisar carga de PWM.

## 8. Questionamentos e Respostas
- Caracteristicas de tempo real: deadlines iguais aos periodos; jitter limitado pelo `vTaskDelayUntil`; misses registrados em log.
- Viabilidade: preencher marcacoes da secao 4 com base nos logs.
- Seguranca: parada abaixo de `DISTANCIA_STOP_CM`; duty saturado; leituras invalidas resultam em duty zero.

## 9. Conclusoes
- Sintese (preencher apos testes): atendimento de deadlines no modo continuo; impacto de suspender/retomar sensores; ajustes aplicados para remover misses.
- Proximos passos: (se necessario) ajustar periodos/timeout; adicionar filtro/histerese na leitura; documentar resultados finais.

## 10. Anexos
- Codigo: `Relatorio2/codigoProposto.ino`.
- Logs brutos das simulacoes (modo continuo e modo ciclo menor/maior).
- Diagramas temporais dos dois modos.
