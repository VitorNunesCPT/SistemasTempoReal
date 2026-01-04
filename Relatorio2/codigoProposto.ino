

// ---------------- PINOS (Ultrassom) ----------------
#define TRIG_F 4
#define ECHO_F 16

#define TRIG_T 17
#define ECHO_T 5

#define TRIG_E 18
#define ECHO_E 19

#define TRIG_D 21
#define ECHO_D 22

// ---------------- MOTORES (PWM LEDC por PINO) ----------------
// Use um LED (com resistor) em um desses pinos para teste.
int motores[4] = {25, 26, 27, 14};

// PWM
static const int PWM_FREQ = 1000; // para teste visual com LED, 1kHz é melhor
static const int PWM_RES  = 8;    // 8 bits -> 0..255

// ---------------- TEMPOS ----------------
const TickType_t PERIODO_SENSOR_TICKS = pdMS_TO_TICKS(50);
const TickType_t PERIODO_MOTOR_TICKS  = pdMS_TO_TICKS(20);
const TickType_t PERIODO_LOG_TICKS    = pdMS_TO_TICKS(200);

// ---------------- DISTANCIAS COMPARTILHADAS ----------------
volatile float dF = -1, dT = -1, dE = -1, dD = -1;
SemaphoreHandle_t mDist;

// ---------------- CONTROLE ----------------
const float DISTANCIA_STOP_CM = 5.0;
const float DISTANCIA_MAX_CM  = 100.0;
const float K_EXP             = 0.05;
const int DUTY_MIN            = 0;
const int DUTY_MAX            = 255;

// ---------------- HANDLES DAS TASKS ----------------
TaskHandle_t thSensorF = nullptr;
TaskHandle_t thSensorT = nullptr;
TaskHandle_t thSensorE = nullptr;
TaskHandle_t thSensorD = nullptr;
TaskHandle_t thMotores = nullptr;
TaskHandle_t thLog     = nullptr;

// ---------------- MEDIÇÃO DE TEMPO (por task) ----------------
typedef struct {
  volatile uint32_t last_us = 0;
  volatile uint32_t max_us  = 0;
  volatile uint64_t sum_us  = 0;
  volatile uint32_t samples = 0;
} TaskTiming;

TaskTiming tmSensorF, tmSensorT, tmSensorE, tmSensorD, tmMotores, tmLog;

static inline void timing_begin(int64_t &t0) {
  t0 = esp_timer_get_time(); // micros
}

static inline void timing_end(TaskTiming &tm, int64_t t0) {
  int64_t t1 = esp_timer_get_time();
  uint32_t dur = (uint32_t)(t1 - t0);
  tm.last_us = dur;
  if (dur > tm.max_us) tm.max_us = dur;
  tm.sum_us += dur;
  tm.samples++;
}

// ---------------- ULTRASSOM ----------------
// pulseIn é bloqueante; ok para experimento, mas pode gerar jitter.
float medirDistanciaCM(int trig, int echo) {
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);

  long dur = pulseIn(echo, HIGH, 30000); // timeout 30ms
  if (dur <= 0) return -1.0f;
  return dur * 0.034f / 2.0f;
}

int controlePWM(float d) {
  if (d <= 0) return 0; // inválida
  if (d <= DISTANCIA_STOP_CM) return 0;

  float dClamped = constrain(d, DISTANCIA_STOP_CM, DISTANCIA_MAX_CM);
  float x = dClamped - DISTANCIA_STOP_CM;
  float fatorExp = 1.0f - expf(-K_EXP * x);
  int pwm = DUTY_MIN + (int)((DUTY_MAX - DUTY_MIN) * fatorExp + 0.5f);
  return constrain(pwm, DUTY_MIN, DUTY_MAX);
}

static inline void aplicarPWM(int pino, int duty) {
  duty = constrain(duty, 0, 255);
  ledcWrite(pino, duty); // Arduino-ESP32 v3.x -> escreve por PINO
}

// ---------------- TASK SENSOR (genérica) ----------------
struct SensorCfg {
  int trig;
  int echo;
  volatile float *dst;
  TaskTiming *tm;
};

void taskSensor(void *arg) {
  SensorCfg *cfg = (SensorCfg*)arg;
  TickType_t lastWake = xTaskGetTickCount();

  for (;;) {
    int64_t t0;
    timing_begin(t0);

    float d = medirDistanciaCM(cfg->trig, cfg->echo);

    // IMPORTANTE: se leitura inválida, NÃO zera; mantém última válida
    if (d > 0) {
      xSemaphoreTake(mDist, portMAX_DELAY);
      *(cfg->dst) = d;
      xSemaphoreGive(mDist);
    }

    timing_end(*(cfg->tm), t0);
    vTaskDelayUntil(&lastWake, PERIODO_SENSOR_TICKS);
  }
}

// ---------------- TASK MOTORES ----------------
void taskMotores(void *arg) {
  (void)arg;
  TickType_t lastWake = xTaskGetTickCount();

  for (;;) {
    int64_t t0;
    timing_begin(t0);

    float lf, lt, le, ld;
    xSemaphoreTake(mDist, portMAX_DELAY);
    lf = dF; lt = dT; le = dE; ld = dD;
    xSemaphoreGive(mDist);

    int p0 = controlePWM(lf);
    int p1 = controlePWM(lt);
    int p2 = controlePWM(le);
    int p3 = controlePWM(ld);

    aplicarPWM(motores[0], p0);
    aplicarPWM(motores[1], p1);
    aplicarPWM(motores[2], p2);
    aplicarPWM(motores[3], p3);

    timing_end(tmMotores, t0);
    vTaskDelayUntil(&lastWake, PERIODO_MOTOR_TICKS);
  }
}

// ---------------- TASK LOG ----------------
void taskLog(void *arg) {
  (void)arg;
  TickType_t lastWake = xTaskGetTickCount();

  for (;;) {
    int64_t t0;
    timing_begin(t0);

    float lf, lt, le, ld;
    xSemaphoreTake(mDist, portMAX_DELAY);
    lf = dF; lt = dT; le = dE; ld = dD;
    xSemaphoreGive(mDist);

    int p0 = controlePWM(lf);
    int p1 = controlePWM(lt);
    int p2 = controlePWM(le);
    int p3 = controlePWM(ld);

    Serial.printf("\n[DIST] F=%.2f T=%.2f E=%.2f D=%.2f\n", lf, lt, le, ld);
    Serial.printf("[PWM ] M0=%d M1=%d M2=%d M3=%d\n", p0, p1, p2, p3);

    auto pr = [](const char* n, const TaskTiming& tm){
      uint32_t avg = (tm.samples > 0) ? (uint32_t)(tm.sum_us / tm.samples) : 0;
      Serial.printf("  %-8s last=%4u us | max=%4u us | avg=%4u us | n=%u\n",
                    n, tm.last_us, tm.max_us, avg, tm.samples);
    };

    pr("SenF", tmSensorF);
    pr("SenT", tmSensorT);
    pr("SenE", tmSensorE);
    pr("SenD", tmSensorD);
    pr("Mot",  tmMotores);
    pr("Log",  tmLog);

    timing_end(tmLog, t0);
    vTaskDelayUntil(&lastWake, PERIODO_LOG_TICKS);
  }
}

// ---------------- SETUP/LOOP ----------------
void setup() {
  Serial.begin(115200);

  // Ultrassom
  pinMode(TRIG_F, OUTPUT); pinMode(ECHO_F, INPUT);
  pinMode(TRIG_T, OUTPUT); pinMode(ECHO_T, INPUT);
  pinMode(TRIG_E, OUTPUT); pinMode(ECHO_E, INPUT);
  pinMode(TRIG_D, OUTPUT); pinMode(ECHO_D, INPUT);

  // Mutex
  mDist = xSemaphoreCreateMutex();

  // PWM (Arduino-ESP32 v3.x): anexa PWM diretamente ao PINO
  ledcAttach(motores[0], PWM_FREQ, PWM_RES);
  ledcAttach(motores[1], PWM_FREQ, PWM_RES);
  ledcAttach(motores[2], PWM_FREQ, PWM_RES);
  ledcAttach(motores[3], PWM_FREQ, PWM_RES);

  // Para garantir que existe “sinal” visível no começo (teste LED):
  // comente se não quiser.
  ledcWrite(motores[0], 64); // ~25%
  delay(300);
  ledcWrite(motores[0], 0);

  // Configs estáticas (não podem ser locais da função)
  static SensorCfg cfgF = {TRIG_F, ECHO_F, &dF, &tmSensorF};
  static SensorCfg cfgT = {TRIG_T, ECHO_T, &dT, &tmSensorT};
  static SensorCfg cfgE = {TRIG_E, ECHO_E, &dE, &tmSensorE};
  static SensorCfg cfgD = {TRIG_D, ECHO_D, &dD, &tmSensorD};

  // Cria tasks (pode ajustar prioridades)
  // Sugestão: motores prioridade maior que sensores; log menor.
  xTaskCreatePinnedToCore(taskSensor, "SensorF", 4096, &cfgF, 2, &thSensorF, 1);
  xTaskCreatePinnedToCore(taskSensor, "SensorT", 4096, &cfgT, 2, &thSensorT, 1);
  xTaskCreatePinnedToCore(taskSensor, "SensorE", 4096, &cfgE, 2, &thSensorE, 1);
  xTaskCreatePinnedToCore(taskSensor, "SensorD", 4096, &cfgD, 2, &thSensorD, 1);

  xTaskCreatePinnedToCore(taskMotores, "Motores", 4096, nullptr, 3, &thMotores, 1);
  xTaskCreatePinnedToCore(taskLog,     "Log",     4096, nullptr, 1, &thLog,     1);

  Serial.println("Sistema FreeRTOS iniciado (sem Supervisor).");
}

void loop() {
  // FreeRTOS roda tudo; loop vazio.
  vTaskDelay(pdMS_TO_TICKS(1000));
}
