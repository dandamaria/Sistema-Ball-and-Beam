#include <Servo.h>

// ---------- CONFIGURAÇÕES DE PINOS ----------
const int TRIG_SETPOINT = 3;
const int ECHO_SETPOINT = 4;
const int TRIG_BALL = 6;
const int ECHO_BALL = 7;
const int SERVO_PIN = 9;

// ---------- LIMITES FÍSICOS DO SERVO ----------
const int SERVO_UP_ANGLE = 20;
const int SERVO_DOWN_ANGLE = 180;
const int SERVO_MIN = 0;
const int SERVO_MAX = 180;

// ---------- CONSTANTES DO CONTROLADOR ----------
double Kp = 1.8;   // ganho proporcional (graus/cm)
double Ki = 0.02;   // ganho integral (graus/cm·s)

// ---------- VARIÁVEIS DO CONTROLE ----------
double setpoint_cm = 20.0;
double integral = 0.0;
unsigned long lastTime;

// ---------- CONFIGURAÇÕES DE LEITURA ----------
const unsigned long PULSE_TIMEOUT = 30000UL;
const int SAMPLE_COUNT = 3;
const int LOOP_DELAY_MS = 90;

Servo servo;

// ---------- FUNÇÃO DE LEITURA DO SENSOR ----------
double readUltrasonic(int trigPin, int echoPin) {
  double sum = 0.0;
  int valid = 0;

  for (int i = 0; i < SAMPLE_COUNT; ++i) {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    unsigned long duration = pulseIn(echoPin, HIGH, PULSE_TIMEOUT);
    if (duration != 0) {
      double dist_cm = duration / 58.2;
      sum += dist_cm;
      valid++;
    }
    delay(10);
  }

  if (valid == 0) return -1.0;
  return sum / valid;
}

// ---------- FUNÇÃO PARA PAUSA DE EMERGÊNCIA ----------
void pausaEmergencial() {
  if (Serial.available() > 0) {
    char c = Serial.read();
    if (c == 'p' || c == 'P') {
      Serial.println("PAUSADO: servo desligado. Reinicie o Arduino para retomar.");
      servo.detach();
      while (true) {} // trava até reset manual
    }
  }
}

// ---------- SETUP ----------
void setup() {
  Serial.begin(9600);
  pinMode(TRIG_SETPOINT, OUTPUT);
  pinMode(ECHO_SETPOINT, INPUT);
  pinMode(TRIG_BALL, OUTPUT);
  pinMode(ECHO_BALL, INPUT);

  servo.attach(SERVO_PIN);
  servo.write((SERVO_UP_ANGLE + SERVO_DOWN_ANGLE) / 2);

  lastTime = millis();
  Serial.println("=== Controle PI (Ball & Beam) ===");
  Serial.println("Envie 'p' no Serial para pausar o servo.");
}

// ---------- LOOP PRINCIPAL ----------
void loop() {
  pausaEmergencial();

  // Ler setpoint
  double sp = readUltrasonic(TRIG_SETPOINT, ECHO_SETPOINT);
  if (sp > 0) setpoint_cm = sp;

  // Ler posição da bola
  double position = readUltrasonic(TRIG_BALL, ECHO_BALL);
  if (position < 0) {
    Serial.println("Leitura inválida (bola). Mantendo servo.");
    delay(LOOP_DELAY_MS);
    return;
  }

  // Calcular erro e intervalo de tempo
  double error = setpoint_cm - position;
  unsigned long now = millis();
  double dt = (now - lastTime) / 1000.0; // em segundos
  lastTime = now;

  // -------- TERMO INTEGRAL --------
  integral += error * dt; 

  // Anti-windup (evita valor muito grande)
  //esses valores 30 e -30 posso mudar depois
  if (integral > 30) integral = 30; 
  if (integral < -30) integral = -30;

  // -------- SAÍDA DO CONTROLADOR --------
  double control_signal = (Kp * error) + (Ki * integral);

  // Converter para ângulo do servo
  double neutral_angle = (SERVO_UP_ANGLE + SERVO_DOWN_ANGLE) / 2.0;
  double servo_angle = neutral_angle + control_signal;

  // Limites físicos
  if (servo_angle > SERVO_DOWN_ANGLE) servo_angle = SERVO_DOWN_ANGLE;
  if (servo_angle < SERVO_UP_ANGLE) servo_angle = SERVO_UP_ANGLE;

  servo.write((int)round(servo_angle));

  // -------- MONITOR SERIAL --------
  Serial.print("SP(cm): ");
  Serial.print(setpoint_cm, 2);
  Serial.print(" | Pos(cm): ");
  Serial.print(position, 2);
  Serial.print(" | Erro(cm): ");
  Serial.print(error, 3);
  Serial.print(" | Int: ");
  Serial.print(integral, 3);
  Serial.print(" | Saída(°): ");
  Serial.println((int)round(servo_angle));

  delay(LOOP_DELAY_MS);

  //abrir o Serial Plotter (fica em Tools) e adicionar esse código lá: 
  /*
  Serial.print(error);
  Serial.print(",");
  Serial.println(control_signal);
  */
}
