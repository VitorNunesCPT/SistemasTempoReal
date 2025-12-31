#include <Arduino.h>
#include <FreeRTOS.h>
#include <semphr.h>

// Ajuste pinos conforme seu hardware.
static const uint8_t BUTTON_PINS[8] = {2, 3, 4, 5, 6, 7, 8, 9};      // PORTD (entrada)
static const uint8_t LED_PINS[8]    = {10, 11, 12, 13, 14, 15, 16, 17}; // PORTB (saida)

SemaphoreHandle_t myMutex;
volatile uint8_t estadoPorta = 0; // compartilhado entre produtor e consumidor

uint8_t lerPortD() {
  uint8_t valor = 0;
  for (uint8_t i = 0; i < 8; i++) {
    int leitura = digitalRead(BUTTON_PINS[i]);
    if (leitura == LOW) { // usando pull-up: LOW significa pressionado
      valor |= (1 << i);
    }
  }
  return valor;
}

void escreverPortB(uint8_t valor) {
  for (uint8_t i = 0; i < 8; i++) {
    bool bitLigado = valor & (1 << i);
    digitalWrite(LED_PINS[i], bitLigado ? HIGH : LOW);
  }
}

void _Ler_botoes(void *pv) {
  const TickType_t periodo = pdMS_TO_TICKS(20);       // ajuste conforme necessidade
  const TickType_t esperaMutex = pdMS_TO_TICKS(10);   // quanto esperar para escrever
  TickType_t t0 = xTaskGetTickCount();
  for (;;) {
    uint8_t valor = lerPortD();
    if (xSemaphoreTake(myMutex, esperaMutex) == pdTRUE) {
      estadoPorta = valor;
      xSemaphoreGive(myMutex);
    } else {
      // timeout ao tentar escrever; opcional: logar erro
    }
    vTaskDelayUntil(&t0, periodo);
  }
}

void LED_acende(void *pv) {
  const TickType_t esperaMutex = pdMS_TO_TICKS(50); // ajustar para estudar deadlock/timeout
  for (;;) {
    uint8_t valorLocal;
    if (xSemaphoreTake(myMutex, esperaMutex) == pdTRUE) {
      valorLocal = estadoPorta;
      xSemaphoreGive(myMutex);
      escreverPortB(valorLocal);
    } else {
      // timeout ao tentar ler; opcional: logar ou definir estado seguro
    }
  }
}

void setup() {
  Serial.begin(115200);
  for (uint8_t i = 0; i < 8; i++) {
    pinMode(BUTTON_PINS[i], INPUT_PULLUP);
    pinMode(LED_PINS[i], OUTPUT);
    digitalWrite(LED_PINS[i], LOW);
  }

  myMutex = xSemaphoreCreateMutex();
  if (myMutex == NULL) {
    Serial.println("Erro ao criar mutex");
    for (;;) {
      delay(1000);
    }
  }

  // Prioridades iguais; elevar consumidor se quiser menor latencia.
  xTaskCreate(_Ler_botoes, "ler_btn", 256, NULL, 2, NULL);
  xTaskCreate(LED_acende, "leds",    256, NULL, 2, NULL);
}

void loop() {
  // Nao usado; FreeRTOS executa as tarefas.
}
