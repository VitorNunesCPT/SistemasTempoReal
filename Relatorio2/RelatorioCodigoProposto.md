# Relatorio - Experimento 2: Codigo Proposto (FreeRTOS)

## 1. Contextualizacao do Problema
O sistema controla um robo movel com 4 sensores ultrassonicos (frente, tras, esquerda, direita) e 4 motores DC. A cada leitura de distancia, a velocidade de cada roda deve ser ajustada por PWM com lei exponencial: aproxima -> reduz duty, afastamento -> aumenta duty. O experimento exige um escalonamento em executivo ciclico e a verificacao das caracteristicas de tempo real das tarefas.

## 2. Objetivos
Desenvolver e documentar um sistema com FreeRTOS que execute a leitura dos sensores e o controle dos motores de forma periodica e previsivel, medindo tempos de computacao e validando a viabilidade do escalonamento.

## 3. Objetivos Especificos
- Separar leituras de sensores em tarefas distintas e controlar os motores em tarefa unica.
- Definir periodos, prioridades e deadlines para cada tarefa.
- Medir tempos de execucao (last/max/avg) e registrar logs simulados.
- Esquematizar a execucao em ciclo maior/menor usando habilitar/desabilitar tarefas.
- Registrar ajustes de tempo e responder aos questionamentos do roteiro.

## 4. Arquitetura Geral do Sistema
- Sensores (4 tarefas) -> variaveis compartilhadas (mutex) -> controle PWM -> motores.
- Supervisor (executivo ciclico) libera tarefas em slots de tempo.
- Log/telemetria registra distancias, PWM e tempos de execucao.

```
[SenF]   [SenT]   [SenE]   [SenD]
   \        |        |        /
    \       |        |       /
     ---> (dF,dT,dE,dD) --[controlePWM]--> [Motores]
                 ^                       ^
                 |                       |
             [Supervisor]            [LogTask]
```

## 5. Definicao das Tarefas e Caracteristicas de Tempo Real
As tarefas sao liberadas por um supervisor ciclico. O ciclo menor e 50 ms e o ciclo maior e 200 ms (4 subciclos).

| Tarefa | Periodo efetivo | Deadline | Prioridade | Tipo de tempo real | Tempo max (us) |
| --- | --- | --- | --- | --- | --- |
| SenF | 200 ms (slot 50 ms) | 200 ms | 2 | firm | 3274 |
| SenT | 200 ms (slot 50 ms) | 200 ms | 2 | firm | 2437 |
| SenE | 200 ms (slot 50 ms) | 200 ms | 2 | firm | 11739 |
| SenD | 200 ms (slot 50 ms) | 200 ms | 2 | firm | 6385 |
| Motores | 50 ms | 50 ms | 3 | firm | 159 |
| Supervisor | 50 ms | 50 ms | 4 | hard | 15 |
| LogTask | 200 ms | 200 ms | 1 | soft | 46927 |

Observacoes:
- Cada sensor roda uma vez por ciclo maior, mas precisa caber no slot de 50 ms do subciclo correspondente.
- LogTask e soft real-time: pode extrapolar sem afetar controle, pois tem baixa prioridade.

## 6. Ajustes de Tempo (descricao detalhada)
- Ciclo menor: `CICLO_MENOR_TICKS = 50 ms` com `vTaskDelayUntil` no supervisor.
- Ciclo maior: `SUBCICLOS_POR_CICLO_MAIOR = 4` (200 ms).
- Sensores por rodizio: 1 sensor por subciclo para reduzir bloqueio do `pulseIn`.
- Timeout do `pulseIn`: 25 ms para limitar tempo de bloqueio.
- Log a cada ciclo maior para reduzir overhead de `Serial.printf`.
- Prioridades: Supervisor (4) > Motores (3) > Sensores (2) > Log (1).

## 7. Esquematizacao do Fluxo Temporal

### 7.1 Ciclo maior (200 ms)
```
Tempo(ms): 0          50         100        150        200
           |----------|----------|----------|----------|
Subciclo:     0          1          2          3
Tarefas:    SenF+Mot   SenT+Mot   SenE+Mot   SenD+Mot+Log
```

### 7.2 Ciclo menor (50 ms) - exemplo do subciclo 3
```
[Supervisor] -> resume SenD -> resume Motores -> resume Log (apenas no subciclo 3)
                |--------- execucao ---------| (ate completar 50 ms)
```

## 8. Codigo Proposto (trechos relevantes)

Lei de controle exponencial:
```cpp
int controlePWM(float d) {
  if (d <= DISTANCIA_STOP_CM) return 0;
  float x = constrain(d, DISTANCIA_STOP_CM, DISTANCIA_MAX_CM) - DISTANCIA_STOP_CM;
  int pwm = (int)(255 * (1.0f - expf(-K_EXP * x)) + 0.5f);
  return constrain(pwm, 0, 255);
}
```

Supervisor ciclico (libera tarefas por subciclo):
```cpp
for (;;) {
  switch (subciclo) {
    case 0: vTaskResume(thSensorF); break;
    case 1: vTaskResume(thSensorT); break;
    case 2: vTaskResume(thSensorE); break;
    case 3: vTaskResume(thSensorD); break;
  }
  vTaskResume(thMotores);
  if (subciclo == (SUBCICLOS_POR_CICLO_MAIOR - 1)) {
    vTaskResume(thLog);
  }
  subciclo = (subciclo + 1) % SUBCICLOS_POR_CICLO_MAIOR;
  vTaskDelayUntil(&lastWake, CICLO_MENOR_TICKS);
}
```

## 9. Resultados Simulados (logs)
Trecho de `Relatorio2/logs.md`:
```
================ [ CICLO MAIOR 200ms ] ================
[DIST] F:48.93 | T:1.92 | E:1.92 | D:101.90
[PWM ] M0:227 | M1:0 | M2:0 | M3:253
---------------- [ TEMPOS (us) ] ----------------
  SenF       last: 3265 | max: 3274 | avg:  682 | n:36
  SenT       last:  502 | max: 2437 | avg:  552 | n:35
  SenE       last:  497 | max:11739 | avg: 3701 | n:35
  SenD       last: 6381 | max: 6385 | avg: 4258 | n:36
  Motores    last:  137 | max:  159 | avg:  114 | n:140
  Supervisor last:   15 | max:   15 | avg:   10 | n:172
  LogTask    last:46923 | max:46927 | avg:46224 | n:34
```

Analise resumida:
- Sensores e motores ficam abaixo do slot de 50 ms, mantendo folga.
- LogTask consome ~46 ms e e executado apenas no ultimo subciclo, reduzindo impacto no controle.

## 10. Questionamentos e Respostas (Roteiro 2)
1) Caracteristicas de tempo real e Tcomp:
- Periodos e deadlines estao definidos na Tabela da secao 5.
- Tempos maximos medidos atendem o slot de 50 ms para sensores/motores.
- Supervisor possui menor tempo e maior prioridade, garantindo cadencia do ciclo menor.

2) Diagrama de fluxo temporal (ciclo unico maior e ciclos menores):
- Diagrama apresentado na secao 7.
- O uso de `vTaskSuspend/vTaskResume` implementa o executivo ciclico e evita concorrencia de `pulseIn`.
- A viabilidade e indicada pelos tempos maximos abaixo do slot de 50 ms (com excecao do log, que e soft).

3) Desempenho em livre demanda:
- No codigo proposto, as tarefas sao liberadas pelo supervisor (modo ciclo menor/maior).
- Em livre demanda, removeria-se o `vTaskSuspend` e cada tarefa teria sua propria espera periodica (ex.: `vTaskDelayUntil` em sensores e motores). O impacto esperado seria aumento de jitter por `pulseIn`, exigindo timeout menor e possivel ajuste de prioridades.
- A analise de logs atuais mostra que a distribuicao por subciclos e uma estrategia segura para manter folga.

## 11. Conclusao
O codigo proposto aplica um executivo ciclico com ciclo menor de 50 ms e ciclo maior de 200 ms, distribuindo leituras de sensores e preservando o tempo para controle dos motores. Os logs simulados demonstram tempos de execucao compativeis com o escalonamento, e os ajustes de tempo documentados justificam a configuracao escolhida.

