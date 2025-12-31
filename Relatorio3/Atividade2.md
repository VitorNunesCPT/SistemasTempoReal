# Atividade 2 - Mutex (PORTD -> PORTB) com FreeRTOS

## Objetivo
- Resolver a Atividade 1 usando mutex em vez de fila.
- Evitar inversao de prioridade e avaliar risco de deadlock ao usar tempos de espera (`xSemaphoreTake`/`xSemaphoreGive`), incluindo o caso `portMAX_DELAY`.

## Estrutura das tarefas
- `_Ler_botoes` (produtora): le PORTD e atualiza uma variavel global `estadoPorta` protegida por mutex.
- `LED_acende` (consumidora): le a mesma variavel protegida e escreve em PORTB.
- Opcional: tarefa de LOG para imprimir bloqueios/timeouts.

## Setup minimo
```c
#include <Arduino.h>
#include <FreeRTOS.h>
#include <semphr.h>

SemaphoreHandle_t myMutex;
volatile uint8_t estadoPorta = 0;
```

## Pseudocodigo das tarefas
```c
void _Ler_botoes(void *pv) {
  const TickType_t periodo = pdMS_TO_TICKS(20);
  TickType_t t0 = xTaskGetTickCount();
  for (;;) {
    uint8_t valor = lerPortD();
    if (xSemaphoreTake(myMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
      estadoPorta = valor;
      xSemaphoreGive(myMutex);
    } else {
      // timeout ao tentar escrever
    }
    vTaskDelayUntil(&t0, periodo);
  }
}

void LED_acende(void *pv) {
  const TickType_t espera = pdMS_TO_TICKS(50); // tempo maximo para tentar ler
  for (;;) {
    uint8_t valor;
    if (xSemaphoreTake(myMutex, espera) == pdTRUE) {
      valor = estadoPorta;
      xSemaphoreGive(myMutex);
      escreverPortB(valor);
    } else {
      // timeout ao tentar ler (avaliar risco de deadlock)
    }
  }
}
```

## Variacoes para estudo de deadlock
- Usar `portMAX_DELAY` em `xSemaphoreTake` para observar bloqueio indefinido caso a outra tarefa nunca libere.
- Reduzir tempos de espera (10–50 ms) e contar timeouts para avaliar se o mutex esta causando starvation.
- Testar prioridades diferentes (consumidora maior que produtora) para ver o efeito da heranca de prioridade.

## Testes sugeridos
- Funcional: pressionar botoes e checar LEDs correspondentes.
- Bloqueio: inserir caminhos que omitem `xSemaphoreGive` (para estudo controlado) e observar que a outra tarefa fica presa.
- Carga: gerar eventos rapidos e verificar se ha contencao excessiva no mutex.
- Log: contar numero de `xSemaphoreTake` bem-sucedidos e timeouts por janela de tempo.

## Observacoes
- Heranca de prioridade do mutex ajuda a evitar inversao: mantenha prioridades coerentes (consumidora igual ou maior que produtora).
- Mantenha secoes criticas curtas (so leitura/escrita do byte).
- Atencao ao tamanho de pilha e aos includes corretos (`semphr.h`).
