# Relatório – Experimento 1: Executivo Cíclico com 4 Sensores e 4 Motores

## 1. Objetivo e Roteiro
- Problema: controle de distância com 4 sensores ultrassônicos e 4 motores DC, lei de controle exponencial via PWM.
- Abordagem: executivo cíclico sem RTOS, com frames de tempo fixo (Arduino/ESP32 no Wokwi).
- Tarefas do roteiro:
  - T1: Caracterizar tempo real de cada tarefa e medir tempo de computação.
  - T2: Diagramar fluxo temporal (ciclo único maior e combinação ciclo menor/maior) e avaliar viabilidade.
  - T3: Implementar executivo cíclico e verificar periodicidade/deadlines em simulação.
  - T4: Documentar resultados, ajustes de tempo e respostas aos questionamentos.
- Requisitos de avaliação: apresentar códigos com resultados simulados, descrever ajustes de tempo e responder aos questionamentos de forma detalhada.

## 2. Sistema e Código Base
- Arquivo avaliado: `Relatorio1/codigoProposto.ino`.
- Pinos: 4 sensores (TRIG/ECHO F, T, E, D) e 4 saídas PWM para motores `{25, 26, 27, 14}`.
- Lei de controle: exponencial, saturada em `DUTY_MIN/MAX`, parada abaixo de `DISTANCIA_STOP_CM`.
- Parâmetros temporais atuais:
  - `FRAME_MS = 10 ms` (frame do executivo); `FRAMES_HIPER = 20` (hiperperíodo 200 ms).
  - Sensores: `PERIODO_SENSOR_MS = 50 ms` → 1 sensor por frame (offsets distintos).
  - Controle: `PERIODO_MOTOR_MS = 20 ms` → roda a cada 2 frames.
  - Log: `PERIODO_LOG_MS = 200 ms` → 1 vez por hiperperíodo.

## 3. Tarefa 1 – Características de Tempo Real e Medições
- Definições:
  - Deadline assumida = período de cada tarefa (hard/firm conforme análise).
  - Jitter aceitável: pequeno (ordem de 1 frame) para sensores e controle; avaliar na simulação.
  - Risco: `pulseIn` pode bloquear até ~30 ms; precisa caber no budget do frame.
- Instrumentação sugerida: medir `micros()` no início/fim de cada tarefa (SF, ST, SE, SD, Controle, Log) e registrar no log o pior caso por hiperperíodo.
- Tabela a ser preenchida com medições:

| Tarefa | Período (ms) | Deadline (ms) | Tcomp min (µs) | Tcomp méd (µs) | Tcomp máx (µs) | Folga no frame (µs) | Observações |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Sensor F | 50 | 50 |  |  |  |  | Timeout do `pulseIn` define pior caso |
| Sensor T | 50 | 50 |  |  |  |  |  |
| Sensor E | 50 | 50 |  |  |  |  |  |
| Sensor D | 50 | 50 |  |  |  |  |  |
| Controle Motores | 20 | 20 |  |  |  |  | Execução depende de 4 chamadas `controlePWM` |
| Log/Telemetria | 200 | 200 |  |  |  |  | Pode ser deslocado para fora do caminho crítico |

- Cálculo de viabilidade (preencher após medições): `Tcomp_total_frame` ≤ `FRAME_MS * 1000`. Se overrun: ajustar `FRAME_MS` ou reduzir timeout do `pulseIn`.

## 4. Tarefa 2 – Diagramas de Fluxo Temporal e Viabilidade
- Ciclo único maior (hiperperíodo 200 ms, frames de 10 ms):
  - Frame 0: Sensor F + Controle + Log
  - Frame 1: Sensor T
  - Frame 2: Sensor E + Controle
  - Frame 3: Sensor D
  - Frame 4: (folga) + Controle
  - Frames 5–9: repetição do padrão de sensores a cada 5 frames; controle em frames pares; log no início do próximo hiperperíodo.
  - Verificar se cada slot suporta o Tcomp medido; registrar folga/reserva por frame.
- Sistema de ciclos menor/maior:
  - Ciclo menor (ex.: 10–20 ms): controle de motores.
  - Ciclo maior (50 ms): cada sensor uma vez por ciclo; log no alinhamento do ciclo maior (200 ms).
  - Avaliar alinhamento: controle roda 2–5 vezes entre leituras de sensor; verificar se leituras com `pulseIn` cabem no slot do ciclo maior sem bloquear o menor.
- Viabilidade (a preencher após medições):
  - [ ] Ciclo único maior atende deadlines sem overrun.
  - [ ] Combinação ciclo menor/maior atende deadlines sem interferência.
  - Observações sobre jitter e folga.

## 5. Resultados de Simulação (Wokwi)
- Evidências a incluir:
  - Logs `[LOG]` com distâncias e `[OVERRUN]/[ATRASO]` se ocorrerem.
  - Tabela de distância vs duty aplicado para mostrar resposta da lei exponencial.
  - Screenshots ou trechos do Serial Monitor.
- Estado da simulação: preencher após execução.

## 6. Ajustes de Tempo
- Ajustes realizados (preencher):
  - `FRAME_MS`: 10 ms (ou novo valor se alterado).
  - `PERIODO_SENSOR_MS`: 50 ms (ou novo valor).
  - `PERIODO_MOTOR_MS`: 20 ms (ou novo valor).
  - Timeout do `pulseIn`: 30000 µs (ou reduzido para caber no frame).
  - Outros: `K_EXP`, `DISTANCIA_STOP_CM`, limites de duty.
- Justificativas: por que cada ajuste garante que Tcomp ≤ budget e mantém o comportamento de controle.

## 7. Questionamentos e Respostas
- Características de tempo real:
  - Tipo de deadline de cada tarefa, periodicidade e jitter tolerável.
  - Estratégia de escalonamento (tabela fixa) e prioridade implícita.
- Viabilidade dos esquemas:
  - Conclusão para ciclo único maior.
  - Conclusão para ciclos menor/maior.
- Considerações de segurança:
  - Parada abaixo de `DISTANCIA_STOP_CM` e saturação de duty.
  - Impacto de leituras inválidas ou timeout.

## 8. Conclusões
- Síntese da viabilidade temporal, atendimento aos requisitos e limitações observadas (sensibilidade ao tempo de eco).
- Próximos passos sugeridos: filtro de ruído/histerese, ajuste fino de ganhos, eventual aumento do frame ou redução de timeout se necessário.

## 9. Anexos
- Código final: `Relatorio1/codigoProposto.ino`.
- Dados de medição de tempo (log bruto).
- Diagramas (ciclo único e ciclos menor/maior).
