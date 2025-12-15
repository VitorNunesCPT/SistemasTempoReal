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
