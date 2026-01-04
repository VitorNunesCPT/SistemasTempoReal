#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

static const uint8_t BUTTON_PINS[8] = { 2, 4, 5, 18, 19, 21, 22, 23 };
static const uint8_t LED_PINS[8]    = { 12, 13, 14, 15, 25, 26, 27, 32 };

QueueHandle_t xQueue_LED;

// ================= TAREFA PRODUTORA ====================
void tarefa_Ler_botoes(void *pv) {
  uint8_t sLEDs = 0;
  uint8_t ultimoValor = 0xFF;

  for (;;) {
    sLEDs = 0;
    for (uint8_t i = 0; i < 8; i++) {
      if (digitalRead(BUTTON_PINS[i]) == LOW) sLEDs |= (1 << i);
    }

    if (sLEDs != ultimoValor) {
      if (xQueue_LED != NULL) {
        // --- INÍCIO DA MEDIÇÃO ---
        TickType_t tempoInicio = xTaskGetTickCount();
        
        Serial.println(">>> Tentando ENVIAR...");
        
        // portMAX_DELAY fará a tarefa esperar o tempo que for preciso
        if (xQueueSend(xQueue_LED, &sLEDs, portMAX_DELAY) == pdPASS) {
          TickType_t tempoFim = xTaskGetTickCount();
          Serial.print("[ESCRITA] Sucesso! Tempo de espera: ");
          Serial.print(tempoFim - tempoInicio);
          Serial.println(" ticks.");
        }
      }
      ultimoValor = sLEDs;
    }
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

// ================= TAREFA CONSUMIDORA ==================
void LED_acende(void *pv) {
  uint8_t rLEDs = 0;

  for (;;) {
    if (xQueue_LED != NULL) {
      // --- INÍCIO DA MEDIÇÃO ---
      TickType_t tempoInicio = xTaskGetTickCount();

      // Tentativa de leitura com timeout de 10 ticks (conforme slide)
      if (xQueueReceive(xQueue_LED, &rLEDs, (TickType_t)10) == pdPASS) {
        TickType_t tempoFim = xTaskGetTickCount();
        
        Serial.print("[LEITURA] Recebido: ");
        Serial.print(rLEDs, BIN);
        Serial.print(" | Esperou: ");
        Serial.print(tempoFim - tempoInicio);
        Serial.println(" ticks.");

        for (uint8_t i = 0; i < 8; i++) {
          digitalWrite(LED_PINS[i], (rLEDs & (1 << i)) ? HIGH : LOW);
        }

        // Simulação de processamento lento para forçar a fila a encher
        // Se você comentar a linha abaixo, a espera de escrita será quase 0.
        vTaskDelay(pdMS_TO_TICKS(1000)); 
      } else {
        // Se entrar aqui, significa que a fila estava vazia e os 10 ticks passaram
        // Serial.println("[LEITURA] Fila vazia (timeout 10 ticks)");
      }
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void setup() {
  Serial.begin(115200);
  for (uint8_t i = 0; i < 8; i++) {
    pinMode(BUTTON_PINS[i], INPUT_PULLUP);
    pinMode(LED_PINS[i], OUTPUT);
  }

  // Fila de tamanho 1 para facilitar a observação do bloqueio
  xQueue_LED = xQueueCreate(1, sizeof(uint8_t));

  xTaskCreate(tarefa_Ler_botoes, "Botoes", 2048, NULL, 1, NULL);
  xTaskCreate(LED_acende,       "LEDs",   2048, NULL, 1, NULL);
}

void loop() {}