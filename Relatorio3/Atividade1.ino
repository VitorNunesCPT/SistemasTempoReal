#include <Arduino.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

// ================= PINOS =================
static const uint8_t BUTTON_PINS[8] = {
  2, 4, 5, 18, 19, 21, 22, 23
};

static const uint8_t LED_PINS[8] = {
  12, 13, 14, 15, 25, 26, 27, 32
};

// Ajuste para avaliar bloqueio:
// - true usa portMAX_DELAY (pode travar se nenhuma tarefa consumir/produzir).
// - false usa tempos finitos para observar timeouts.
static const bool BLOQUEIO_INFINITO = false;
static const TickType_t ESPERA_ENVIO =
  BLOQUEIO_INFINITO ? portMAX_DELAY : pdMS_TO_TICKS(50);
static const TickType_t ESPERA_RECEPCAO =
  BLOQUEIO_INFINITO ? portMAX_DELAY : pdMS_TO_TICKS(10);

// ============== FILA ======================
QueueHandle_t xQueue_LED;
volatile uint32_t sendOk = 0;
volatile uint32_t sendTimeouts = 0;
volatile uint32_t recvOk = 0;
volatile uint32_t recvTimeouts = 0;

// ============ TAREFA PRODUTORA ============
void _Ler_botoes(void *pv) {
  for (;;) {
    uint8_t valor = 0;

    // Le os botoes e monta o byte
    for (uint8_t i = 0; i < 8; i++) {
      if (digitalRead(BUTTON_PINS[i]) == LOW) {
        valor |= (1 << i);
      }
    }

    if (xQueue_LED != NULL) {
      UBaseType_t pendentes = uxQueueMessagesWaiting(xQueue_LED);
      BaseType_t ok = xQueueSend(xQueue_LED, &valor, ESPERA_ENVIO);
      if (ok == pdPASS) {
        sendOk++;
      } else {
        sendTimeouts++;
        Serial.print("send_timeout=");
        Serial.print(sendTimeouts);
        Serial.print(" pendentes=");
        Serial.println(pendentes);
      }
    }

    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

// ============ TAREFA CONSUMIDORA ==========
void LED_acende(void *pv) {
  uint8_t valorLocal = 0;

  for (;;) {
    if (xQueue_LED != NULL) {
      UBaseType_t pendentes = uxQueueMessagesWaiting(xQueue_LED);
      if (pendentes == 0) {
        // Fila vazia; ainda assim podemos esperar pela leitura.
      }

      if (xQueueReceive(xQueue_LED, &valorLocal, ESPERA_RECEPCAO) == pdPASS) {
        recvOk++;
        for (uint8_t i = 0; i < 8; i++) {
          digitalWrite(LED_PINS[i], (valorLocal & (1 << i)) ? HIGH : LOW);
        }
      } else {
        recvTimeouts++;
        Serial.print("recv_timeout=");
        Serial.print(recvTimeouts);
        Serial.print(" pendentes=");
        Serial.println(pendentes);
      }
    }

    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);
  // Configura botoes
  for (uint8_t i = 0; i < 8; i++) {
    pinMode(BUTTON_PINS[i], INPUT_PULLUP);
  }

  // Configura LEDs
  for (uint8_t i = 0; i < 8; i++) {
    pinMode(LED_PINS[i], OUTPUT);
    digitalWrite(LED_PINS[i], LOW);
  }

  // Inicializa a fila
  xQueue_LED = xQueueCreate(1, sizeof(uint8_t));
  if (xQueue_LED == NULL) {
    while (true) {}
  }

  // Consumidor com prioridade maior para reduzir latencia nos LEDs.
  xTaskCreate(_Ler_botoes, "Botoes", 2048, NULL, 1, NULL);
  xTaskCreate(LED_acende,  "LEDs",   2048, NULL, 2, NULL);
}

void loop() {
  // FreeRTOS controla tudo
}
