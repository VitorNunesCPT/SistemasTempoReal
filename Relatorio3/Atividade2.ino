#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h" // Biblioteca necessária

// ================= MAPEAMENTO E GLOBAIS =================
static const uint8_t BUTTON_PINS[8] = { 2, 4, 5, 18, 19, 21, 22, 23 };
static const uint8_t LED_PINS[8]    = { 12, 13, 14, 15, 25, 26, 27, 32 };

char *string_global; // Variável global para seção crítica

QueueHandle_t xQueue_LED;
SemaphoreHandle_t myMutex; // Identificador do mutex

// ================= FUNÇÃO TESTE COM AVALIAÇÃO DE ESPERA =================
void teste(char *str) {
    // Início da medição de espera pelo Mutex
    uint32_t iniWaitMutex = millis(); 
    
    // Tenta obter o mutex. O uso de portMAX_DELAY é o ponto crítico para Deadlock
    if (xSemaphoreTake(myMutex, portMAX_DELAY) == pdTRUE) {
        uint32_t fimWaitMutex = millis();
        uint32_t tempoEspera = fimWaitMutex - iniWaitMutex;

        // Início da Seção Crítica
        string_global = str;
        
        Serial.print("\n[MUTEX] Recurso obtido para: ");
        Serial.println(string_global);
        
        // Avaliação do tempo de bloqueio
        Serial.print("[AVALIAÇÃO] Tempo de espera pelo Mutex: ");
        Serial.print(tempoEspera);
        Serial.println(" ms");
        
        vTaskDelay(pdMS_TO_TICKS(5)); // Simula processamento na seção crítica
        
        // Liberação do recurso
        xSemaphoreGive(myMutex);
    }
}

// ================= TAREFA PRODUTORA (BOTÕES) ====================
void tarefa_Ler_botoes(void *pv) {
    uint8_t sLEDs = 0;
    uint8_t ultimoValor = 0xFF;

    for (;;) {
        // Medição de Computação
        uint32_t iniComp = micros();
        sLEDs = 0;
        for (uint8_t i = 0; i < 8; i++) {
            if (digitalRead(BUTTON_PINS[i]) == LOW) sLEDs |= (1 << i);
        }
        uint32_t fimComp = micros();

        if (sLEDs != ultimoValor) {
            // Chama a função protegida pelo Mutex
            teste((char*)"Mudanca de Botao"); 

            if (xQueue_LED != NULL) {
                uint32_t iniWaitFila = millis();
                // Envio para fila com portMAX_DELAY
                if (xQueueSend(xQueue_LED, &sLEDs, portMAX_DELAY) == pdPASS) {
                    uint32_t fimWaitFila = millis();
                    Serial.print("[PRODUTOR] Computacao: "); Serial.print(fimComp - iniComp); Serial.println(" us");
                    Serial.print("[PRODUTOR] Espera Fila: "); Serial.print(fimWaitFila - iniWaitFila); Serial.println(" ms");
                }
            }
            ultimoValor = sLEDs;
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// ================= TAREFA CONSUMIDORA (LEDs) ==================
void LED_acende(void *pv) {
    uint8_t rLEDs = 0;
    for (;;) {
        if (xQueue_LED != NULL) {
            // Recebimento da fila com timeout de 10 ticks
            if (xQueueReceive(xQueue_LED, &rLEDs, (TickType_t)10) == pdPASS) {
                
                teste((char*)"Atualizando Saida");

                for (uint8_t i = 0; i < 8; i++) {
                    digitalWrite(LED_PINS[i], (rLEDs & (1 << i)) ? HIGH : LOW);
                }
            }
          vTaskDelay(pdMS_TO_TICKS(2000)); 
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
    }

    // Criação do Mutex associado à estrutura de semáforo
    myMutex = xSemaphoreCreateMutex();

    if (myMutex != NULL) {
        Serial.println("mutex pronto para uso"); //
    } else {
        Serial.println("Nao pude criar o mutex"); //
        while(1);
    }

    // Criação da fila com tamanho 1 (1 byte)
    xQueue_LED = xQueueCreate(1, sizeof(uint8_t));

    // Adição das tarefas com prioridades para observar a herança de prioridade
    xTaskCreate(tarefa_Ler_botoes, "Ler_Botoes", 2048, NULL, 2, NULL); 
    xTaskCreate(LED_acende,       "LED_Acende",  2048, NULL, 1, NULL);
}

void loop() {}