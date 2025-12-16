# Plano de Desenvolvimento do Relatório – Experimento 1 (Executivo Cíclico)

## 1. Objetivo e Contexto
- Descrever o problema (4 sensores ultrassônicos, 4 motores DC, lei exponencial de controle de velocidade) e a abordagem em executivo cíclico.
- Referenciar o código base `codigoProposto.ino` e o roteiro `Roteiro1.md`.
- Destacar que a execução será sem RTOS, via frames de tempo fixos (FRAME_MS) formando um hiperperíodo.

## 2. Arquitetura do Executivo Cíclico
- Frames de 10 ms (`FRAME_MS = 10`), hiperperíodo de 200 ms (`FRAMES_HIPER = 20`).
- Distribuição atual (para descrever e ilustrar no diagrama):
  - Sensores: a cada 50 ms (`PERIODO_SENSOR_MS`), escalonados em frames diferentes (SF/T/E/D).
  - Controle de motores: a cada 20 ms (`PERIODO_MOTOR_MS`), executa em frames alternados.
  - Log: a cada 200 ms (`PERIODO_LOG_MS`).
- Inserir uma tabela com: Tarefa, Período, Deadline assumida, Offset no hiperperíodo, Budget do frame.

## 3. Tarefa 1 – Características de Tempo Real e Medições
- Instrumentar o código com medições de `micros()` para cada tarefa: SF, ST, SE, SD, Controle de Motores, Log (se relevante).
- Registrar Tcomp min/médio/máx e pior caso observado (pelo `pulseIn` com timeout de 30 ms).
- Definir deadline (igual ao período) e jitter aceitável; justificar escolha.
- Tabela a preencher no relatório:
  - Colunas: Tarefa | Período (ms) | Deadline (ms) | Tcomp min/med/máx (µs) | Folga no frame (µs) | Observações (p.ex. risco de timeout).
- Analisar viabilidade: soma de Tcomp no frame ≤ `FRAME_MS * 1000`; se houver overrun, propor ajuste (reduzir timeout ou aumentar FRAME_MS).

## 4. Tarefa 2 – Diagramas de Fluxo Temporal
- Diagrama 1: ciclo único maior (hiperperíodo 200 ms dividido em 20 frames de 10 ms) mostrando:
  - Em quais frames cada sensor roda, onde o controle roda (a cada 2 frames) e o log (frame 0).
  - Indicar budgets e folgas em cada slot.
- Diagrama 2: sistema de ciclos menor/maior:
  - Ciclo menor: 10–20 ms para controle de motores.
  - Ciclo maior: 50 ms para sensores.
  - Mostrar alinhamento entre os dois ciclos e discutir viabilidade (se Tcomp_sensor + Tcomp_motor cabe no slot; risco de sobreposição).
- Para cada diagrama, avaliar explicitamente a viabilidade (sem overrun? folga residual? jitter esperado).

## 5. Resultados de Simulação
- Rodar no Wokwi e capturar:
  - Logs `[LOG]` com distâncias e possíveis `[OVERRUN]`/`[ATRASO]`.
  - Caso aplicável, tabela de distância vs duty calculado (lei exponencial) para ilustrar comportamento.
- Incluir trechos de log ou screenshots como evidência.

## 6. Ajustes de Tempo
- Descrever ajustes feitos: valores de `FRAME_MS`, `PERIODO_SENSOR_MS`, `PERIODO_MOTOR_MS`, `PERIODO_LOG_MS`, `K_EXP`, `DUTY_MIN/MAX`.
- Justificar: por que cabem no orçamento temporal; impacto de reduzir timeout do `pulseIn` se necessário.
- Documentar se houve remoção ou redução de jitter/overrun após ajustes.

## 7. Respostas aos Questionamentos do Roteiro
- Responder de forma direta e detalhada:
  - Características de tempo real de cada tarefa (tipo de deadline, período, jitter tolerável, prioridade implícita).
  - Viabilidade dos esquemas de ciclo único maior e ciclos menor/maior.
  - Observações sobre limites de segurança (parada se `d <= DISTANCIA_STOP_CM`) e comportamento do controle exponencial.

## 8. Conclusões
- Síntese da viabilidade temporal, atendimento aos requisitos e limitações observadas (p.ex., dependência do tempo de eco).
- Próximos passos sugeridos (se houver): filtro de ruído, histerese, ajuste fino de ganhos.

## 9. Checklist de Entrega
- [ ] Código final em `Relatorio1/codigoProposto.ino` instrumentado (ou com seção de resultados da instrumentação).
- [ ] Tabelas de Tcomp preenchidas.
- [ ] Diagramas temporais (ciclo único e ciclos menor/maior).
- [ ] Logs/resultados de simulação incluídos.
- [ ] Respostas aos questionamentos do roteiro.
- [ ] Ajustes de tempo descritos.
