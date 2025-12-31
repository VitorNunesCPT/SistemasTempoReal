# Roteiro 3 - Concorrencia e comunicacao entre processos com FreeRTOS

Valor: 2 pontos de 10

## Pre-leitura
- FreeRTOS tem filas FIFO (Queues) para comunicar tarefas. A tarefa produtora coloca dados e a consumidora retira o primeiro que entrou.
- Incluir `queue.h` para usar as filas.

## Atividade 1 - Queue (PORTD -> PORTB)
Objetivo: Tarefa 1 ler botoes no PORTD, enfileirar o estado; Tarefa 2 consumir e acender o LED correspondente no PORTB.

Dispositivos: Arduino ou ESP32 (simulado ou experimental), computador, botoes, LEDs, protoboard, cabos.

Procedimentos:
- Incluir as bibliotecas FreeRTOS e Queue.
- Declarar a fila global antes do `setup()` (ex.: `QueueHandle_t fila;`).
- Criar as tarefas `_Ler_botoes` e `LED_acende` com `xTaskCreate`, definindo prioridades adequadas.
- Inicializar a fila no `setup()` com `xQueueCreate(tamanho_da_fila, tamanho_do_item)`; para o PORT um byte (8 bits) e o tamanho da fila bastam.
- Funcoes uteis:
  - Verificar se a fila existe: `if (fila != NULL) { ... }`
  - Checar itens na fila: `uxQueueMessagesWaiting(fila)` (0 indica vazia).
  - Escrever: `xQueueSend(fila, &valor_porta, tempo_espera_ticks);` (`portMAX_DELAY` espera infinito; cuidado com deadlock).
  - Ler: `if (xQueueReceive(fila, &destino, tempo_espera_ticks) == pdTRUE) { ... }` (equivalente a `pdPASS`).

Tarefas a entregar:
- Implementar compartilhamento via queue.
- Avaliar risco de deadlock conforme tempos de espera de escrita/leitura.

## Atividade 2 - Mutex
Objetivo: Resolver a Atividade 1 usando mutex.

Notas de conceito:
- O mutex em FreeRTOS usa a mesma API de semaforo e aplica heranca de prioridade (a tarefa que detem o mutex pode subir de prioridade ao ser preemptada por quem tambem quer o recurso).
- Bibliotecas: `#include <freertos/FreeRTOS.h>`, `#include <freertos/task.h>`, `#include <freertos/semphr.h>`.

Procedimentos:
- Declarar `SemaphoreHandle_t myMutex;`.
- Criar o mutex (ex.: dentro de uma tarefa ou no setup):
  ```c
  myMutex = xSemaphoreCreateMutex();
  if (myMutex != NULL) {
      Serial.println("mutex pronto para uso");
  } else {
      Serial.println("nao foi possivel criar o mutex");
  }
  ```
- Usar `xSemaphoreTake(myMutex, tempo_espera)` para entrar na secao critica e `xSemaphoreGive(myMutex)` para liberar:
  ```c
  void teste(char *str) {
      xSemaphoreTake(myMutex, portMAX_DELAY);
      string_global = str;
      Serial.println(string_global);
      xSemaphoreGive(myMutex);
  }
  ```

Tarefas a entregar:
- Reimplementar a Atividade 1 com mutex.
- Definir tempos de espera que evitem deadlock (avaliar uso de `portMAX_DELAY`).
- Comparar execucao e possiveis deadlocks versus o metodo com queue.
