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

### Registro de testes e ajustes de tempo
- Teste 1 (log coletado): `FRAME_MS=10 ms`, timeout padrão do `pulseIn` (30 ms). Resultado: `Tcomp` de sensor chegou a ~24 ms (frames 1, 6, 11, 16 com `[OVERRUN]` de ~24.0 ms), não cabe no frame de 10 ms → atraso acumulado ~1,5 s.
- Ajuste proposto: reduzir timeout do `pulseIn` (ex.: 8000–10000 µs) ou aumentar `FRAME_MS` (ex.: 25–30 ms) e recalcular hiperperíodo para evitar overrun.
- Próximo passo de medição: repetir com novo timeout/frame e atualizar a tabela de Tcomp (min/méd/máx) para cada tarefa.

## 4. Tarefa 2 – Diagramas de Fluxo Temporal e Viabilidade
- Ciclo único maior (hiperperíodo 200 ms, frames de 10 ms):
  - Tabela do hiperperíodo (cada coluna = frame de 10 ms):

    ```
    Frame:   0    1    2    3    4    5    6    7    8    9   10   11   12   13   14   15   16   17   18   19
    Sensor:  F         E         -    F         E         -    F         E         -    F         E         -
    Sensor:       T         D         T         D              T         D              T         D
    Controle: C         C         C         C         C         C         C         C         C         C
    Log:     LOG                                                                                              
    ```

    - Sensor: cada um a cada 50 ms (offsets diferentes).
    - Controle: a cada 20 ms (frames pares).
    - Log: a cada 200 ms (frame 0 do hiperperíodo).
    - A preencher: folga por frame = `10.000 µs - Tcomp(frame)`.

- Sistema de ciclos menor/maior:
  - Ciclo menor (controle): 20 ms (ou 10 ms se reduzido), roda 2–5 vezes entre leituras dos sensores.
  - Ciclo maior (sensores): 50 ms, com offsets SF, ST, SE, SD em 4 subslots de ~10 ms cada; log a cada 200 ms.
  - Visualização simplificada (50 ms):

    ```
    t=0ms    t=10ms   t=20ms   t=30ms   t=40ms   t=50ms
    SF       ST       SE       SD       (folga/log opcional)   -> repete
    Cn roda a cada 20 ms (ou 10 ms) dentro do ciclo: C0, C1, C2...
    ```

  - Avaliação: verificar se Tcomp_sensor + Tcomp_controle cabem nos slots sem bloquear o ciclo menor; documentar folga.

- Viabilidade (marcar após medições):
  - [ ] Ciclo único maior atende deadlines sem overrun.
  - [ ] Combinação ciclo menor/maior atende deadlines sem interferência.
  - Observações: registrar jitter observado e margem de folga em µs para cada slot.

## 5. Resultados de Simulação (Wokwi)
- Evidências a incluir:
  - Logs `[LOG]` com distâncias e `[OVERRUN]/[ATRASO]` se ocorrerem.
  - Tabela de distância vs duty aplicado para mostrar resposta da lei exponencial.
  - Screenshots ou trechos do Serial Monitor.
- Estado da simulação: preencher após execução.
- Topologia simulada (imagem anexada pelo aluno):
  - Placa ESP32 com 4 sensores HC-SR04 distribuídos (frente, trás, esquerda, direita) e LEDs de indicação por setor.
  - Ligações padrão HC-SR04 (VCC=5V, GND comum, TRIG/ECHO nos pinos definidos no código).
  - Alimentação e GND compartilhados; fios verdes para sinais, vermelho VCC, preto GND.

### Log coletado (recorte)
```
[LOG] F: 2.04 | T: 0.00 | E: 0.00 | D: 0.00
[OVERRUN] frame 1 dur(us)=23877 budget(us)=10000
[ATRASO] frame 1 atraso(ms)=1514
...
[LOG] F: 2.09 | T: 399.92 | E: 1.92 | D: 2.01
[OVERRUN] frame 1 dur(us)=23920 budget(us)=10000
...
[OVERRUN] frame 16 dur(us)=24075 budget(us)=10000
```

### Observações do log
- Overrun recorrente em frames com leitura de sensor (`pulseIn` chegando a ~24 ms) excedendo o budget de 10 ms.
- Atrasos acumulados ~1,4–1,5 s após sequência de overruns, indicando que o frame de 10 ms não comporta o pior caso de leitura.
- Próximo ajuste sugerido: aumentar `FRAME_MS` (ex.: 25–30 ms) ou reduzir timeout do `pulseIn` para caber no slot; re-escalar o hiperperíodo e recalcular a tabela de frames.

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
