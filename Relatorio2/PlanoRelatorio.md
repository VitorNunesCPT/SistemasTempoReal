# Plano de Desenvolvimento do Relatório – Experimento 2 (FreeRTOS)

## 1) Objetivo e Escopo
- Implementar controle de distância (4 sensores HC-SR04) e velocidade (4 motores) usando tarefas FreeRTOS.
- Atender ao `Roteiro2.md`: medir tempos de computação, esquematizar execução com habilitação/desabilitação de tarefas, avaliar desempenho em livre demanda, documentar resultados e ajustes.

## 2) Arquitetura de Tarefas (proposta)
- Tarefas de sensor: `TSF`, `TST`, `TSE`, `TSD` (uma por sensor), prioridade média.
- Tarefa de controle de motores: `TCM`, prioridade igual/maior que sensores (a definir).
- Tarefa de log/telemetria: baixa prioridade, janela ampla.
- Sincronismo: usar `vTaskDelayUntil` para periodicidade; tick de 1 ms (config padrão Wokwi/ESP32). Para medir em µs, usar `esp_timer_get_time()`.
- Comunicação: variáveis compartilhadas protegidas por mutex ou seção crítica curta (as leituras são rápidas, mas medir custo do `pulseIn`).

## 3) Tarefa 1 – Medição de Tempo de Computação
- Instrumentar cada tarefa com marcação de início/fim (`esp_timer_get_time()`):
  - Registrar min/méd/max por janela (ex.: a cada 1 s ou N períodos) e imprimir no log.
  - Colunas no relatório: Tarefa | Período (ms) | Deadline (ms) | Tcomp min/med/máx (µs) | Folga (µs) | Observações.
- Avaliar overhead do `pulseIn` (timeout reduzido para caber no período). Ajustar se necessário.

## 4) Tarefa 2 – Diagramas e Habilitar/Desabilitar
- Diagrama 1: ciclo único maior (todas as tarefas ativas, periodicidade fixa). Mostrar slots/ticks e folgas.
- Diagrama 2: ciclo menor/maior usando habilitação/desabilitação:
  - Ex.: sensores em ciclo maior (50 ms), controle em ciclo menor (20 ms); suspender/retomar tarefas de sensor (`vTaskSuspend/vTaskResume`) para ilustrar viabilidade e folga.
- Avaliar viabilidade: se cada tarefa cumpre deadline com o tick configurado e sem preempção excessiva.
- Implementação no código (ajuste necessário):
  - Adicionar modo “contínuo” (todas as tarefas ativas) e modo “ciclo menor/maior” (suspender/retomar sensores conforme janela).
  - Opção simples: usar um comando serial ou `#define MODO_CICLO_MM` para alternar; no modo MM, suspender sensores durante uma janela curta e retomar na janela seguinte, logando início/fim e folga observada.

## 5) Tarefa 3 – Livre Demanda
- Executar sem suspender tarefas, apenas com prioridades e `vTaskDelayUntil` (ou sem delay, se for testar “consumo livre”) para observar escalonamento.
- Logar tempos de execução e eventos de preempção/atraso (usar contadores de “miss” quando atraso > período).

## 6) Resultados de Simulação
- Ambiente: Wokwi (ESP32 + 4 HC-SR04 + atuadores simulados).
- Coletar logs: distâncias, duty calculado, Tcomp min/méd/máx, misses/overruns por tarefa.
- Incluir capturas do serial ou tabelas derivadas dos logs.

## 7) Ajustes de Tempo e Parâmetros
- Tick/periodicidade: definir períodos dos sensores (ex. 40–60 ms) e do controle (ex. 10–20 ms).
- Timeout de leitura: ajustar para não estourar o período.
- Prioridades: documentar as escolhas (ex.: controle > sensores > log) e justificar.
- Stack size: registrar valores usados em `xTaskCreate` e qualquer ajuste feito.

## 8) Checklist de Entrega
- [ ] Código FreeRTOS com tarefas de sensor, controle e log.
- [ ] Logs com tempos de computação (min/méd/máx) por tarefa.
- [ ] Diagramas ciclo único e ciclo menor/maior (habilitar/desabilitar).
- [ ] Avaliação em livre demanda (sem forçar habilitação/desabilitação).
- [ ] Ajustes de tempo descritos (tick, período, timeout, prioridades, stack).
- [ ] Respostas aos questionamentos do roteiro incluídas no relatório final.
