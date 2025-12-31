// ---------- PINOS ----------
#define TRIG_F 4
#define ECHO_F 16

#define TRIG_T 17
#define ECHO_T 5

#define TRIG_E 18
#define ECHO_E 19

#define TRIG_D 21
#define ECHO_D 22

int motores[4] = {25, 26, 27, 14};

// ---------- TEMPOS ----------
const unsigned long PERIODO_SENSOR_MS = 50;  // alvo de cada tarefa de sensor
const unsigned long PERIODO_MOTOR_MS  = 20;  // alvo da tarefa unica de controle
const unsigned long PERIODO_LOG_MS    = 200; // telemetria para verificacao
const unsigned long FRAME_MS          = 10;  // frame do executivo ciclico
const unsigned int FRAMES_HIPER       = 20;  // hiperperiodo 200 ms (20 frames)
const unsigned int SENSOR_PERIOD_FRAMES = PERIODO_SENSOR_MS / FRAME_MS; // 5
const unsigned int MOTOR_PERIOD_FRAMES  = PERIODO_MOTOR_MS  / FRAME_MS; // 2
const unsigned int LOG_PERIOD_FRAMES    = PERIODO_LOG_MS    / FRAME_MS; // 20

unsigned long proximoFrameMs = 0;
unsigned int frameIndex = 0;

// ---------- DISTANCIAS ----------
float dF, dT, dE, dD;

// ---------- CONTROLE ----------
const float DISTANCIA_STOP_CM = 5.0;    // distancia minima de seguranca
const float DISTANCIA_MAX_CM  = 100.0;  // distancia maxima considerada
const float K_EXP             = 0.05;   // ganho da curva exponencial
const int DUTY_MIN            = 0;      // limite inferior de duty
const int DUTY_MAX            = 255;    // limite superior de duty

// ---------- FUNCAO GENERICA ----------
float medirDistancia(int trig, int echo) {
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);

  long dur = pulseIn(echo, HIGH, 30000);
  return dur * 0.034 / 2;
}

// Cada sensor e tratado como uma tarefa separada dentro do frame.
void tarefaSensor(int trig, int echo, float &distancia) {
  distancia = medirDistancia(trig, echo);
}

// ---------- LEI DE CONTROLE ----------
int controlePWM(float d) {
  // Duty cresce exponencialmente ao afastar e decresce ao aproximar.
  if (d <= 0) {
    return 0; // leitura invalida ou muito proxima
  }

  float dClamped = constrain(d, DISTANCIA_STOP_CM, DISTANCIA_MAX_CM);
  float x = dClamped - DISTANCIA_STOP_CM;          // afastamento em relacao ao limite seguro
  float fatorExp = 1.0 - exp(-K_EXP * x);          // 0 ate a distancia de parada, tende a 1 em distancia alta
  int pwm = DUTY_MIN + (int)((DUTY_MAX - DUTY_MIN) * fatorExp + 0.5f);

  if (d <= DISTANCIA_STOP_CM) {
    return 0; // parada total no limite de seguranca
  }
  return constrain(pwm, DUTY_MIN, DUTY_MAX);
}

// Tarefa unica de controle para os 4 motores (aplica duty de uma so vez)
void tarefaControleMotores() {
  analogWrite(motores[0], controlePWM(dF));
  analogWrite(motores[1], controlePWM(dT));
  analogWrite(motores[2], controlePWM(dE));
  analogWrite(motores[3], controlePWM(dD));
}


void setup() {
  Serial.begin(115200);
  proximoFrameMs = millis() + FRAME_MS;

  pinMode(TRIG_F, OUTPUT); pinMode(ECHO_F, INPUT);
  pinMode(TRIG_T, OUTPUT); pinMode(ECHO_T, INPUT);
  pinMode(TRIG_E, OUTPUT); pinMode(ECHO_E, INPUT);
  pinMode(TRIG_D, OUTPUT); pinMode(ECHO_D, INPUT);

  for (int i = 0; i < 4; i++) {
    pinMode(motores[i], OUTPUT);
  }
}

void loop() {
  unsigned long inicioFrameUs = micros();

  // ---------- EXECUTIVO CICLICO POR FRAME ----------
  // Distribuicao das tarefas de sensor ao longo dos frames (offsets diferentes para periodos de 50 ms).
  switch (frameIndex % SENSOR_PERIOD_FRAMES) {
    case 0: tarefaSensor(TRIG_F, ECHO_F, dF); break;
    case 1: tarefaSensor(TRIG_T, ECHO_T, dT); break;
    case 2: tarefaSensor(TRIG_E, ECHO_E, dE); break;
    case 3: tarefaSensor(TRIG_D, ECHO_D, dD); break;
    default: break; // frame 4: descanso, fecha o periodo de 50 ms
  }

  // Controle de motores a cada 20 ms (um frame sim, outro nao).
  if (frameIndex % MOTOR_PERIOD_FRAMES == 0) {
    tarefaControleMotores();
  }

  // Telemetria a cada 200 ms (hiperperiodo)
  if (frameIndex % LOG_PERIOD_FRAMES == 0) {
    Serial.print("[LOG] F: "); Serial.print(dF);
    Serial.print(" | T: "); Serial.print(dT);
    Serial.print(" | E: "); Serial.print(dE);
    Serial.print(" | D: "); Serial.println(dD);
  }

  // ---------- VERIFICACAO DE DEADLINE DO FRAME ----------
  unsigned long duracaoUs = micros() - inicioFrameUs;
  if (duracaoUs > FRAME_MS * 1000UL) {
    Serial.print("[OVERRUN] frame "); Serial.print(frameIndex);
    Serial.print(" dur(us)="); Serial.print(duracaoUs);
    Serial.print(" budget(us)="); Serial.println(FRAME_MS * 1000UL);
  }

  // ---------- ESPERA ATE O PROXIMO FRAME ----------
  long atrasoMs = (long)(proximoFrameMs - millis());
  if (atrasoMs > 0) {
    delay(atrasoMs);
  } else if (atrasoMs < 0) {
    Serial.print("[ATRASO] frame "); Serial.print(frameIndex);
    Serial.print(" atraso(ms)="); Serial.println(-atrasoMs);
    // nao fazemos catch-up agressivo; seguimos para o proximo frame para evitar jitter acumulado
  }

  frameIndex = (frameIndex + 1) % FRAMES_HIPER;
  proximoFrameMs += FRAME_MS;
}
// ---------- CONFIGURACAO GERAL ----------
#include <Arduino.h>

// ---------- PINOS ----------
#define TRIG_F 4
#define ECHO_F 16
#define TRIG_T 17
#define ECHO_T 5
#define TRIG_E 18
#define ECHO_E 19
#define TRIG_D 21
#define ECHO_D 22

int motores[4] = {25, 26, 27, 14};

// ---------- PERIODOS (MS) ----------
const uint32_t PERIODO_SENSOR_MS = 50;   // alvo de cada tarefa de sensor
const uint32_t PERIODO_CONTROLE_MS = 20; // alvo da tarefa de controle
const uint32_t PERIODO_LOG_MS = 200;     // janela de telemetria

// ---------- PARAMETROS DE LEITURA ----------
const uint32_t TIMEOUT_PULSE_US = 8000; // timeout reduzido para caber no periodo

// ---------- CONTROLE ----------
const float DISTANCIA_STOP_CM = 5.0;    // distancia minima de seguranca
const float DISTANCIA_MAX_CM = 100.0;   // distancia maxima considerada
const float K_EXP = 0.05;               // ganho da curva exponencial
const int DUTY_MIN = 0;                 // limite inferior de duty
const int DUTY_MAX = 255;               // limite superior de duty

// ---------- ESTATISTICAS ----------
enum EstatId { EST_SF, EST_ST, EST_SE, EST_SD, EST_CTRL, EST_LOG, EST_COUNT };
uint64_t estatMin[EST_COUNT];
uint64_t estatMax[EST_COUNT];
bool estatSeen[EST_COUNT];
uint64_t inicioRelUs = 0;
uint32_t ciclos[EST_COUNT];
uint32_t misses[EST_COUNT];
int64_t lastSlackUs[EST_COUNT];

void resetEstat() {
  for (int i = 0; i < EST_COUNT; i++) {
    estatMin[i] = UINT64_MAX;
    estatMax[i] = 0;
    estatSeen[i] = false;
    ciclos[i] = 0;
    misses[i] = 0;
    lastSlackUs[i] = 0;
  }
}

inline void atualizaEstat(int id, uint64_t durUs) {
  estatSeen[id] = true;
  if (durUs < estatMin[id]) estatMin[id] = durUs;
  if (durUs > estatMax[id]) estatMax[id] = durUs;
}

// ---------- SINCRONIZACAO ----------
portMUX_TYPE muxDist = portMUX_INITIALIZER_UNLOCKED;
float dF = 0, dT = 0, dE = 0, dD = 0;

// ---------- LEITURA DE DISTANCIA ----------
float medirDistancia(int trig, int echo) {
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);

  long dur = pulseIn(echo, HIGH, TIMEOUT_PULSE_US);
  return dur * 0.034f / 2.0f;
}

// ---------- LEI DE CONTROLE ----------
int controlePWM(float d) {
  if (d <= 0) return 0;
  float dClamped = constrain(d, DISTANCIA_STOP_CM, DISTANCIA_MAX_CM);
  float x = dClamped - DISTANCIA_STOP_CM;
  float fatorExp = 1.0f - expf(-K_EXP * x);
  int pwm = DUTY_MIN + (int)((DUTY_MAX - DUTY_MIN) * fatorExp + 0.5f);
  if (d <= DISTANCIA_STOP_CM) return 0;
  return constrain(pwm, DUTY_MIN, DUTY_MAX);
}

// ---------- PARAMETROS DE TAREFA ----------
struct SensorParam {
  int trig;
  int echo;
  float* distRef;
  const char* nome;
  EstatId estatId;
};

// ---------- TAREFAS ----------
void tarefaSensor(void* pv) {
  SensorParam* p = (SensorParam*)pv;
  TickType_t xLastWake = xTaskGetTickCount();
  const TickType_t periodo = pdMS_TO_TICKS(PERIODO_SENSOR_MS);
  const uint32_t periodoUs = PERIODO_SENSOR_MS * 1000UL;
  for (;;) {
    uint64_t t0 = esp_timer_get_time();
    float d = medirDistancia(p->trig, p->echo);
    portENTER_CRITICAL(&muxDist);
    *(p->distRef) = d;
    portEXIT_CRITICAL(&muxDist);
    uint64_t dur = esp_timer_get_time() - t0;
    atualizaEstat(p->estatId, dur);
    ciclos[p->estatId]++;
    int64_t slack = (int64_t)periodoUs - (int64_t)dur;
    lastSlackUs[p->estatId] = slack;
    if (slack < 0) misses[p->estatId]++;
    vTaskDelayUntil(&xLastWake, periodo);
  }
}

void tarefaControle(void* pv) {
  (void)pv;
  TickType_t xLastWake = xTaskGetTickCount();
  const TickType_t periodo = pdMS_TO_TICKS(PERIODO_CONTROLE_MS);
  const uint32_t periodoUs = PERIODO_CONTROLE_MS * 1000UL;
  for (;;) {
    portENTER_CRITICAL(&muxDist);
    float lf = dF, lt = dT, le = dE, ld = dD;
    portEXIT_CRITICAL(&muxDist);
    uint64_t t0 = esp_timer_get_time();
    analogWrite(motores[0], controlePWM(lf));
    analogWrite(motores[1], controlePWM(lt));
    analogWrite(motores[2], controlePWM(le));
    analogWrite(motores[3], controlePWM(ld));
    uint64_t dur = esp_timer_get_time() - t0;
    atualizaEstat(EST_CTRL, dur);
    ciclos[EST_CTRL]++;
    int64_t slack = (int64_t)periodoUs - (int64_t)dur;
    lastSlackUs[EST_CTRL] = slack;
    if (slack < 0) misses[EST_CTRL]++;
    vTaskDelayUntil(&xLastWake, periodo);
  }
}

void tarefaLog(void* pv) {
  (void)pv;
  TickType_t xLastWake = xTaskGetTickCount();
  const TickType_t periodo = pdMS_TO_TICKS(PERIODO_LOG_MS);
  const uint32_t periodoUs = PERIODO_LOG_MS * 1000UL;
  for (;;) {
    uint64_t t0 = esp_timer_get_time();
    portENTER_CRITICAL(&muxDist);
    float lf = dF, lt = dT, le = dE, ld = dD;
    portEXIT_CRITICAL(&muxDist);
    Serial.print("[LOG] F: "); Serial.print(lf);
    Serial.print(" | T: "); Serial.print(lt);
    Serial.print(" | E: "); Serial.print(le);
    Serial.print(" | D: "); Serial.print(ld);
    Serial.print(" | Tmax(us) SF/ ST/ SE/ SD/ CTRL: ");
    if (estatSeen[EST_SF]) { Serial.print(estatMax[EST_SF]); } else { Serial.print("-"); }
    Serial.print(" / ");
    if (estatSeen[EST_ST]) { Serial.print(estatMax[EST_ST]); } else { Serial.print("-"); }
    Serial.print(" / ");
    if (estatSeen[EST_SE]) { Serial.print(estatMax[EST_SE]); } else { Serial.print("-"); }
    Serial.print(" / ");
    if (estatSeen[EST_SD]) { Serial.print(estatMax[EST_SD]); } else { Serial.print("-"); }
    Serial.print(" / ");
    if (estatSeen[EST_CTRL]) { Serial.print(estatMax[EST_CTRL]); } else { Serial.print("-"); }
    Serial.print(" | ciclos SF/ST/SE/SD/CTRL: ");
    Serial.print(ciclos[EST_SF]); Serial.print("/");
    Serial.print(ciclos[EST_ST]); Serial.print("/");
    Serial.print(ciclos[EST_SE]); Serial.print("/");
    Serial.print(ciclos[EST_SD]); Serial.print("/");
    Serial.print(ciclos[EST_CTRL]);
    Serial.print(" | misses SF/ST/SE/SD/CTRL: ");
    Serial.print(misses[EST_SF]); Serial.print("/");
    Serial.print(misses[EST_ST]); Serial.print("/");
    Serial.print(misses[EST_SE]); Serial.print("/");
    Serial.print(misses[EST_SD]); Serial.print("/");
    Serial.print(misses[EST_CTRL]);
    Serial.print(" | slack(us) ultimo SF/ST/SE/SD/CTRL: ");
    Serial.print(lastSlackUs[EST_SF]); Serial.print("/");
    Serial.print(lastSlackUs[EST_ST]); Serial.print("/");
    Serial.print(lastSlackUs[EST_SE]); Serial.print("/");
    Serial.print(lastSlackUs[EST_SD]); Serial.print("/");
    Serial.print(lastSlackUs[EST_CTRL]);
    Serial.println();
    resetEstat(); // zera min/max/misses/ciclos/slack para a proxima janela
    uint64_t dur = esp_timer_get_time() - t0;
    atualizaEstat(EST_LOG, dur);
    ciclos[EST_LOG]++;
    int64_t slack = (int64_t)periodoUs - (int64_t)dur;
    lastSlackUs[EST_LOG] = slack;
    if (slack < 0) misses[EST_LOG]++;
    vTaskDelayUntil(&xLastWake, periodo);
  }
}

// ---------- SETUP ----------
void setup() {
  Serial.begin(115200);
  resetEstat();
  inicioRelUs = esp_timer_get_time();

  pinMode(TRIG_F, OUTPUT); pinMode(ECHO_F, INPUT);
  pinMode(TRIG_T, OUTPUT); pinMode(ECHO_T, INPUT);
  pinMode(TRIG_E, OUTPUT); pinMode(ECHO_E, INPUT);
  pinMode(TRIG_D, OUTPUT); pinMode(ECHO_D, INPUT);
  for (int i = 0; i < 4; i++) pinMode(motores[i], OUTPUT);

  static SensorParam params[] = {
    {TRIG_F, ECHO_F, &dF, "SF", EST_SF},
    {TRIG_T, ECHO_T, &dT, "ST", EST_ST},
    {TRIG_E, ECHO_E, &dE, "SE", EST_SE},
    {TRIG_D, ECHO_D, &dD, "SD", EST_SD},
  };

  xTaskCreatePinnedToCore(tarefaSensor, "SF", 2048, &params[0], 2, nullptr, 1);
  xTaskCreatePinnedToCore(tarefaSensor, "ST", 2048, &params[1], 2, nullptr, 1);
  xTaskCreatePinnedToCore(tarefaSensor, "SE", 2048, &params[2], 2, nullptr, 1);
  xTaskCreatePinnedToCore(tarefaSensor, "SD", 2048, &params[3], 2, nullptr, 1);
  xTaskCreatePinnedToCore(tarefaControle, "CTRL", 2048, nullptr, 3, nullptr, 1);
  xTaskCreatePinnedToCore(tarefaLog, "LOG", 2048, nullptr, 1, nullptr, 1);
}

// ---------- LOOP ----------
void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}
