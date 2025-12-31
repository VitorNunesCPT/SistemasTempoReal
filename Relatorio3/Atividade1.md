# Atividade 1 - Queue (PORTD -> PORTB) com FreeRTOS

## Objetivo
- Ler estado dos botoes no PORTD (tarefa produtora) e acender LEDs correspondentes no PORTB (tarefa consumidora) usando fila FIFO da FreeRTOS.
- Configurar tempos de espera de escrita/leitura avaliando risco de deadlock.

## Estrutura das tarefas
- `_Ler_botoes` (produtora): le PORTD, envia byte para fila.
- `LED_acende` (consumidora): bloqueia em `xQueueReceive`, aplica byte recebido em PORTB.
- Opcional: tarefa de LOG para imprimir eventos/timeouts.

## Setup minimo
```c
#include <Arduino.h>
#include <FreeRTOS.h>
#include <queue.h>

QueueHandle_t fila;

void setup() {
  // Configurar GPIOs de PORTD como entrada e PORTB como saida conforme seu MCU.
  fila = xQueueCreate(1, sizeof(uint8_t)); // fila de 1 byte
  if (fila == NULL) {
    // erro de criacao; abortar ou registrar
  }
  xTaskCreate(_Ler_botoes, "ler_btn", 256, NULL, 2, NULL);
  xTaskCreate(LED_acende, "leds", 256, NULL, 2, NULL);
}
```

## Pseudocodigo das tarefas
```c
void _Ler_botoes(void *pv) {
  const TickType_t espera = pdMS_TO_TICKS(10);   // tempo maximo para enfileirar
  const TickType_t periodo = pdMS_TO_TICKS(20);  // ajuste conforme necessidade
  TickType_t t0 = xTaskGetTickCount();
  for (;;) {
    uint8_t estado = lerPortD(); // funcao que retorna byte de PORTD
    // xQueueOverwrite evita fila cheia quando tamanho=1
    if (xQueueOverwrite(fila, &estado) != pdPASS) {
      // opcional: log de erro
    }
    vTaskDelayUntil(&t0, periodo);
  }
}

void LED_acende(void *pv) {
  const TickType_t espera = pdMS_TO_TICKS(50); // ajustar para avaliar deadlock/timeout
  for (;;) {
    uint8_t valor;
    if (xQueueReceive(fila, &valor, espera) == pdTRUE) {
      escreverPortB(valor); // aplica byte nos LEDs
    } else {
      // timeout: opcional definir estado seguro ou logar
    }
  }
}
```

## Variacoes para estudo de deadlock
- Usar `portMAX_DELAY` em `xQueueSend` e/ou `xQueueReceive` e observar se alguma tarefa fica bloqueada caso a outra pare.
- Reduzir tempo de espera para valores finitos (ex.: 10-50 ms) e medir ocorrencia de timeouts.
- Testar `xQueueSend` versus `xQueueOverwrite` (quando a fila tem tamanho 1).

## Testes sugeridos
- Pressionar cada botao de PORTD e verificar LED correspondente em PORTB.
- Gerar pulsos rapidos e checar se o consumidor acompanha sem perder eventos (ou se isso e aceitavel com fila de tamanho 1).
- Medir `uxQueueMessagesWaiting` em execucao para verificar se a fila satura ou esvazia.
- Registrar timeouts/erros para documentar riscos de deadlock.

## Observacoes
- Manter prioridades das tarefas iguais ou ligeiramente maiores para a consumidora se desejar menor latencia.
- Ajustar tamanhos de pilha conforme uso real; valores acima sao referencia minima.
- Atividade 2 (mutex) sera tratada apos fechar esta fase com queue.
