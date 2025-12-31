#include <Arduino.h>
#include <FreeRTOS.h>
#include <queue.h>

// Ajuste os pinos conforme seu hardware (ex.: ESP32 ou Arduino UNO).
static const uint8_t BUTTON_PINS[8] = {2, 3, 4, 5, 6, 7, 8, 9};   // PORTD (entrada)
static const uint8_t LED_PINS[8]    = {10, 11, 12, 13, 14, 15, 16, 17}; // PORTB (saida)

// Ajuste para testar bloqueio:
// - BLOQUEIO_INFINITO = true usa portMAX_DELAY em send/receive (avaliar deadlock).
// - BLOQUEIO_INFINITO = false usa tempos finitos para ver timeouts.
static const bool BLOQUEIO_INFINITO = false;
static const TickType_t ESPERA_ENVIO = BLOQUEIO_INFINITO ? portMAX_DELAY : pdMS_TO_TICKS(50);
static const TickType_t ESPERA_RECEPCAO = BLOQUEIO_INFINITO ? portMAX_DELAY : pdMS_TO_TICKS(50);

QueueHandle_t fila;
volatile uint32_t sendOk = 0, sendTimeouts = 0;
volatile uint32_t recvOk = 0, recvTimeouts = 0;

// Le os 8 bits dos botoes como um byte.
uint8_t lerPortD() {
  uint8_t valor = 0;
  for (uint8_t i = 0; i < 8; i++) {
    int leitura = digitalRead(BUTTON_PINS[i]);
    // Considera nivel baixo como pressionado se usar pull-up.
    if (leitura == LOW) {
      valor |= (1 << i);
    }
  }
  return valor;
}

// Escreve o byte nos 8 LEDs.
void escreverPortB(uint8_t valor) {
  for (uint8_t i = 0; i < 8; i++) {
    bool bitLigado = valor & (1 << i);
    digitalWrite(LED_PINS[i], bitLigado ? HIGH : LOW);
  }
}

void _Ler_botoes(void *pv) {
  const TickType_t periodo = pdMS_TO_TICKS(20);  // ajuste conforme necessidade
  TickType_t t0 = xTaskGetTickCount();
  for (;;) {
    uint8_t estado = lerPortD();
    // Para avaliar bloqueio: usar xQueueSend com espera configuravel.
    BaseType_t ok = xQueueSend(fila, &estado, ESPERA_ENVIO);
    if (ok == pdPASS) {
      sendOk++;
    } else {
      sendTimeouts++;
    }
    vTaskDelayUntil(&t0, periodo);
  }
}

void LED_acende(void *pv) {
  for (;;) {
    uint8_t valor;
    if (xQueueReceive(fila, &valor, ESPERA_RECEPCAO) == pdTRUE) {
      escreverPortB(valor);
      recvOk++;
    } else {
      recvTimeouts++;
      // Timeout opcional: manter estado anterior ou definir seguro.
    }
  }
}

// Tarefa de monitoramento para observar deadlocks/timeouts.
void monitorFila(void *pv) {
  const TickType_t periodo = pdMS_TO_TICKS(1000);
  for (;;) {
    UBaseType_t pend = uxQueueMessagesWaiting(fila);
    Serial.print("fila=");
    Serial.print(pend);
    Serial.print(" send_ok=");
    Serial.print(sendOk);
    Serial.print(" send_to=");
    Serial.print(sendTimeouts);
    Serial.print(" recv_ok=");
    Serial.print(recvOk);
    Serial.print(" recv_to=");
    Serial.println(recvTimeouts);
    vTaskDelay(periodo);
  }
}

void setup() {
  Serial.begin(115200);
  for (uint8_t i = 0; i < 8; i++) {
    pinMode(BUTTON_PINS[i], INPUT_PULLUP); // pull-up interno para botoes
    pinMode(LED_PINS[i], OUTPUT);
    digitalWrite(LED_PINS[i], LOW);
  }

  fila = xQueueCreate(1, sizeof(uint8_t)); // fila de 1 byte
  if (fila == NULL) {
    Serial.println("Erro ao criar fila");
    for (;;) {
      delay(1000);
    }
  }

  // Prioridades iguais; ajuste se precisar de menor latencia no consumidor.
  xTaskCreate(_Ler_botoes, "ler_btn", 256, NULL, 2, NULL);
  xTaskCreate(LED_acende, "leds", 256, NULL, 2, NULL);
  xTaskCreate(monitorFila, "mon", 256, NULL, 1, NULL);
}

void loop() {
  // Nao usado; FreeRTOS executa as tarefas.
}
