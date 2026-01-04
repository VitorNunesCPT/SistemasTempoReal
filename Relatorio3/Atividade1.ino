#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

// ================= MAPEAMENTO DE PINOS =================
static const uint8_t BUTTON_PINS[8] = { 2, 4, 5, 18, 19, 21, 22, 23 };
static const uint8_t LED_PINS[8]    = { 12, 13, 14, 15, 25, 26, 27, 32 };

// ======================== FILA =========================
QueueHandle_t xQueue_LED;

// ================= TAREFA PRODUTORA ====================
void tarefa_Ler_botoes(void *pv) {
  uint8_t sLEDs = 0;
  uint8_t ultimoValor = 0xFF;

  for (;;) {
    // 1. MEDIÇÃO DO TEMPO DE COMPUTAÇÃO (Processamento)
    uint32_t iniComp = micros();
    
    sLEDs = 0;
    for (uint8_t i = 0; i < 8; i++) {
      if (digitalRead(BUTTON_PINS[i]) == LOW) sLEDs |= (1 << i);
    }
    
    uint32_t fimComp = micros();

    // Enviar apenas se houver mudança
    if (sLEDs != ultimoValor) {
      if (xQueue_LED != NULL) {
        
        // 2. MEDIÇÃO DO TEMPO DE ESPERA (Bloqueio da Fila)
        uint32_t iniWait = millis();
        
        Serial.println("\n[PRODUTOR] Tentando enviar para a fila...");
        
        // portMAX_DELAY: Se a fila encher, a tarefa trava aqui (Risco de Deadlock)
        if (xQueueSend(xQueue_LED, &sLEDs, portMAX_DELAY) == pdPASS) {
          uint32_t fimWait = millis();
          
          Serial.print(">> [INFO] Computacao: "); Serial.print(fimComp - iniComp); Serial.println(" us");
          Serial.print(">> [INFO] Espera p/ Escrita: "); Serial.print(fimWait - iniWait); Serial.println(" ms");
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
      
      // 1. MEDIÇÃO DO TEMPO DE ESPERA (Leitura)
      uint32_t iniWait = millis();
      
      // Espera de 10 ticks conforme slide
      if (xQueueReceive(xQueue_LED, &rLEDs, (TickType_t)10) == pdPASS) {
        uint32_t fimWait = millis();

        // 2. MEDIÇÃO DO TEMPO DE COMPUTAÇÃO (Atualizar Hardware)
        uint32_t iniComp = micros();
        for (uint8_t i = 0; i < 8; i++) {
          digitalWrite(LED_PINS[i], (rLEDs & (1 << i)) ? HIGH : LOW);
        }
        uint32_t fimComp = micros();

        Serial.println("\n[CONSUMIDOR] Dado recebido e processado!");
        Serial.print("<< [INFO] Espera p/ Leitura: "); Serial.print(fimWait - iniWait); Serial.println(" ms");
        Serial.print("<< [INFO] Computacao: "); Serial.print(fimComp - iniComp); Serial.println(" us");

        // ATRASO PROPOSITAL: Ative para simular lentidão e ver a fila encher (e o produtor esperar)
        //vTaskDelay(pdMS_TO_TICKS(2000)); 
      }
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

// ======================== SETUP =========================
void setup() {
  Serial.begin(115200);

  for (uint8_t i = 0; i < 8; i++) {
    pinMode(BUTTON_PINS[i], INPUT_PULLUP);
    pinMode(LED_PINS[i], OUTPUT);
    digitalWrite(LED_PINS[i], LOW);
  }

  // Fila tamanho 1 (Máximo risco de bloqueio/deadlock)
  xQueue_LED = xQueueCreate(1, sizeof(uint8_t));

  if (xQueue_LED == NULL) {
    Serial.println("Erro ao criar a fila!");
    while (true);
  }

  // Criação das tarefas
  xTaskCreate(tarefa_Ler_botoes, "Ler_Botoes", 2048, NULL, 1, NULL);
  xTaskCreate(LED_acende,       "LED_Acende",  2048, NULL, 1, NULL);

  Serial.println("Sistema Iniciado. Monitore os tempos abaixo:");
}

void loop() {}