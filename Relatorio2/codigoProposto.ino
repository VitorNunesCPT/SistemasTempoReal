#include <Arduino.h>
#include "esp_timer.h"

// ---------------- PINOS ----------------
#define TRIG_F 4
#define ECHO_F 16
#define TRIG_T 17
#define ECHO_T 5
#define TRIG_E 18
#define ECHO_E 19
#define TRIG_D 21
#define ECHO_D 22

int motores[4] = {25, 26, 27, 14};
static const int PWM_FREQ = 1000;
static const int PWM_RES  = 8;
struct TaskTiming {
  volatile uint32_t last_us = 0;
  volatile uint32_t max_us  = 0;
  volatile uint64_t sum_us  = 0;
  volatile uint32_t samples = 0;
};
// ---------------- TEMPOS: CICLO MENOR/MAIOR ----------------
const TickType_t CICLO_MENOR_TICKS = pdMS_TO_TICKS(50);
const int SUBCICLOS_POR_CICLO_MAIOR = 4; // 4*50ms = 200ms

// ---------------- DADOS COMPARTILHADOS ----------------
volatile float dF = -1, dT = -1, dE = -1, dD = -1;
SemaphoreHandle_t mDist;

// ---------------- CONTROLE ----------------
const float DISTANCIA_STOP_CM = 5.0;
const float DISTANCIA_MAX_CM  = 100.0;
const float K_EXP             = 0.05;

int controlePWM(float d) {
  if (d <= DISTANCIA_STOP_CM) return 0;
  float x = constrain(d, DISTANCIA_STOP_CM, DISTANCIA_MAX_CM) - DISTANCIA_STOP_CM;
  int pwm = (int)(255 * (1.0f - expf(-K_EXP * x)) + 0.5f);
  return constrain(pwm, 0, 255);
}

// ============================================================
//  TAREFA 1: ESTRUTURA DE TIMING (DECLARE ANTES DAS FUNÇÕES!)
// ============================================================


TaskTiming tmSensorF, tmSensorT, tmSensorE, tmSensorD, tmMotores, tmLog, tmSupervisor;

static inline void timing_begin(int64_t &t0) { t0 = esp_timer_get_time(); }

static inline void timing_end(TaskTiming &tm, int64_t t0) {
  uint32_t dur = (uint32_t)(esp_timer_get_time() - t0);
  tm.last_us = dur;
  if (dur > tm.max_us) tm.max_us = dur;
  tm.sum_us += dur;
  tm.samples++;
}

// ---------------- ULTRASSOM ----------------
float medirDistanciaCM(int trig, int echo) {
  digitalWrite(trig, LOW); delayMicroseconds(2);
  digitalWrite(trig, HIGH); delayMicroseconds(10);
  digitalWrite(trig, LOW);

  long dur = pulseIn(echo, HIGH, 25000); // 25ms timeout
  return (dur <= 0) ? -1.0f : (dur * 0.034f / 2.0f);
}

// ---------------- HANDLES ----------------
TaskHandle_t thSensorF = nullptr;
TaskHandle_t thSensorT = nullptr;
TaskHandle_t thSensorE = nullptr;
TaskHandle_t thSensorD = nullptr;
TaskHandle_t thMotores = nullptr;
TaskHandle_t thLog     = nullptr;
TaskHandle_t thSupervisor = nullptr;

// ---------------- TASK SENSOR ----------------
struct SensorCfg { int trig; int echo; volatile float *dst; TaskTiming *tm; };

void taskSensor(void *arg) {
  SensorCfg *cfg = (SensorCfg*)arg;

  for (;;) {
    vTaskSuspend(NULL); // acorda quando o Supervisor libera

    int64_t t0; timing_begin(t0);

    float d = medirDistanciaCM(cfg->trig, cfg->echo);

    // Não zera em leitura inválida
    if (d > 0) {
      xSemaphoreTake(mDist, portMAX_DELAY);
      *(cfg->dst) = d;
      xSemaphoreGive(mDist);
    }

    timing_end(*(cfg->tm), t0);
  }
}

// ---------------- TASK MOTORES ----------------
void taskMotores(void *arg) {
  (void)arg;

  for (;;) {
    vTaskSuspend(NULL); // liberado todo subciclo

    int64_t t0; timing_begin(t0);

    float lf, lt, le, ld;
    xSemaphoreTake(mDist, portMAX_DELAY);
    lf = dF; lt = dT; le = dE; ld = dD;
    xSemaphoreGive(mDist);

    ledcWrite(motores[0], controlePWM(lf));
    ledcWrite(motores[1], controlePWM(lt));
    ledcWrite(motores[2], controlePWM(le));
    ledcWrite(motores[3], controlePWM(ld));

    timing_end(tmMotores, t0);
  }
}

// ---------------- TASK LOG (CICLO MAIOR) ----------------
void taskLog(void *arg) {
  (void)arg;

  for (;;) {
    vTaskSuspend(NULL); // liberado 1x por ciclo maior (200ms)

    int64_t t0; timing_begin(t0);

    float lf, lt, le, ld;
    xSemaphoreTake(mDist, portMAX_DELAY);
    lf = dF; lt = dT; le = dE; ld = dD;
    xSemaphoreGive(mDist);

    int p0 = controlePWM(lf), p1 = controlePWM(lt), p2 = controlePWM(le), p3 = controlePWM(ld);

    Serial.printf("\n================ [ CICLO MAIOR 200ms ] ================\n");
    Serial.printf("[DIST] F:%.2f | T:%.2f | E:%.2f | D:%.2f\n", lf, lt, le, ld);
    Serial.printf("[PWM ] M0:%d | M1:%d | M2:%d | M3:%d\n", p0, p1, p2, p3);
    Serial.println("---------------- [ TEMPOS (us) ] ----------------");

    auto pr = [](const char* n, const TaskTiming& tm){
      uint32_t avg = (tm.samples > 0) ? (uint32_t)(tm.sum_us / tm.samples) : 0;
      Serial.printf("  %-10s last:%5u | max:%5u | avg:%5u | n:%u\n",
                    n, tm.last_us, tm.max_us, avg, tm.samples);
    };

    pr("SenF", tmSensorF);
    pr("SenT", tmSensorT);
    pr("SenE", tmSensorE);
    pr("SenD", tmSensorD);
    pr("Motores", tmMotores);
    pr("Supervisor", tmSupervisor);
    pr("LogTask", tmLog);

    timing_end(tmLog, t0);
  }
}

// ---------------- SUPERVISOR (EXECUTIVO CÍCLICO) ----------------
// Ciclo menor (50ms): libera 1 sensor (RR) + motores
// Ciclo maior (200ms): no último subciclo, libera log
void taskSupervisor(void *arg) {
  (void)arg;

  TickType_t lastWake = xTaskGetTickCount();
  int subciclo = 0;

  for (;;) {
    int64_t t0; timing_begin(t0);

    // 1) Libera 1 sensor por subciclo (viável com pulseIn)
    switch (subciclo) {
      case 0: vTaskResume(thSensorF); break;
      case 1: vTaskResume(thSensorT); break;
      case 2: vTaskResume(thSensorE); break;
      case 3: vTaskResume(thSensorD); break;
    }

    // 2) Motores em todo ciclo menor
    vTaskResume(thMotores);

    // 3) Log no fim do ciclo maior (a cada 200ms)
    if (subciclo == (SUBCICLOS_POR_CICLO_MAIOR - 1)) {
      vTaskResume(thLog);
    }

    timing_end(tmSupervisor, t0);

    // avança subciclo
    subciclo = (subciclo + 1) % SUBCICLOS_POR_CICLO_MAIOR;

    // cadência do ciclo menor
    vTaskDelayUntil(&lastWake, CICLO_MENOR_TICKS);
  }
}

// ---------------- SETUP ----------------
void setup() {
  Serial.begin(115200);

  pinMode(TRIG_F, OUTPUT); pinMode(ECHO_F, INPUT);
  pinMode(TRIG_T, OUTPUT); pinMode(ECHO_T, INPUT);
  pinMode(TRIG_E, OUTPUT); pinMode(ECHO_E, INPUT);
  pinMode(TRIG_D, OUTPUT); pinMode(ECHO_D, INPUT);

  mDist = xSemaphoreCreateMutex();

  // PWM (Arduino-ESP32 v3.x)
  for (int i = 0; i < 4; i++) {
    ledcAttach(motores[i], PWM_FREQ, PWM_RES);
    ledcWrite(motores[i], 0);
  }

  // Configs estáticas
  static SensorCfg cF = {TRIG_F, ECHO_F, &dF, &tmSensorF};
  static SensorCfg cT = {TRIG_T, ECHO_T, &dT, &tmSensorT};
  static SensorCfg cE = {TRIG_E, ECHO_E, &dE, &tmSensorE};
  static SensorCfg cD = {TRIG_D, ECHO_D, &dD, &tmSensorD};

  // Cria tasks escravas
  xTaskCreatePinnedToCore(taskSensor, "SenF", 4096, &cF, 2, &thSensorF, 1);
  xTaskCreatePinnedToCore(taskSensor, "SenT", 4096, &cT, 2, &thSensorT, 1);
  xTaskCreatePinnedToCore(taskSensor, "SenE", 4096, &cE, 2, &thSensorE, 1);
  xTaskCreatePinnedToCore(taskSensor, "SenD", 4096, &cD, 2, &thSensorD, 1);

  xTaskCreatePinnedToCore(taskMotores, "Mot", 4096, NULL, 3, &thMotores, 1);
  xTaskCreatePinnedToCore(taskLog,     "Log", 4096, NULL, 1, &thLog,     1);

  // Supervisor (maior prioridade)
  xTaskCreatePinnedToCore(taskSupervisor, "Sup", 4096, NULL, 4, &thSupervisor, 1);

  Serial.println("OK: ciclo menor 50ms / ciclo maior 200ms (enable/disable).");
}

void loop() {
  vTaskDelay(portMAX_DELAY);
}
