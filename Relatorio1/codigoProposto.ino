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
const unsigned long PERIODO_SENSOR_MS = 50;
const unsigned long PERIODO_MOTOR_MS  = 20;
const unsigned long PERIODO_LOG_MS    = 200;

unsigned long tSensorF = 0;
unsigned long tSensorT = 0;
unsigned long tSensorE = 0;
unsigned long tSensorD = 0;
unsigned long tMotores = 0;
unsigned long tLog     = 0;

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


void setup() {
  Serial.begin(115200);

  pinMode(TRIG_F, OUTPUT); pinMode(ECHO_F, INPUT);
  pinMode(TRIG_T, OUTPUT); pinMode(ECHO_T, INPUT);
  pinMode(TRIG_E, OUTPUT); pinMode(ECHO_E, INPUT);
  pinMode(TRIG_D, OUTPUT); pinMode(ECHO_D, INPUT);

  for (int i = 0; i < 4; i++) {
    pinMode(motores[i], OUTPUT);
  }
}

void loop() {
  unsigned long agora = millis();

  // -------- TAREFAS DOS SENSORES (cada sensor e uma tarefa) --------
  if (agora - tSensorF >= PERIODO_SENSOR_MS) {
    dF = medirDistancia(TRIG_F, ECHO_F);
    tSensorF = agora;
  }

  if (agora - tSensorT >= PERIODO_SENSOR_MS) {
    dT = medirDistancia(TRIG_T, ECHO_T);
    tSensorT = agora;
  }

  if (agora - tSensorE >= PERIODO_SENSOR_MS) {
    dE = medirDistancia(TRIG_E, ECHO_E);
    tSensorE = agora;
  }

  if (agora - tSensorD >= PERIODO_SENSOR_MS) {
    dD = medirDistancia(TRIG_D, ECHO_D);
    tSensorD = agora;
  }

  // -------- TAREFA DE CONTROLE DOS MOTORES (20 ms) --------
  if (agora - tMotores >= PERIODO_MOTOR_MS) {
    analogWrite(motores[0], controlePWM(dF));
    analogWrite(motores[1], controlePWM(dT));
    analogWrite(motores[2], controlePWM(dE));
    analogWrite(motores[3], controlePWM(dD));

    tMotores = agora;
  }

  // -------- TELEMETRIA (log agrupado) --------
  if (agora - tLog >= PERIODO_LOG_MS) {
    Serial.print("F: "); Serial.print(dF);
    Serial.print(" | T: "); Serial.print(dT);
    Serial.print(" | E: "); Serial.print(dE);
    Serial.print(" | D: "); Serial.println(dD);
    tLog = agora;
  }
}
