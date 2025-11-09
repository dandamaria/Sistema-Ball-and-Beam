#include <Servo.h>

// -------- pinos ----------
const int TRIG_SETPOINT = 3; // sensor do alvo
const int ECHO_SETPOINT = 4;

const int TRIG_BALL = 6; // sensor da bola
const int ECHO_BALL = 7;

const int SERVO_PIN = 9;

// -------- parâmetros do servo / segurança ----------
const int SERVO_UP_ANGLE   = 20;  // "subir" -> 20°
const int SERVO_DOWN_ANGLE = 180; // "baixar" -> 180°
const int SERVO_MIN = 0;
const int SERVO_MAX = 180;

// -------- controle P ----------
double Kp = 1.8; 
double setpoint_cm = 20.0; // valor inicial do alvo em cm que é atualizado

// -------- outras configurações ----------
const unsigned long PULSE_TIMEOUT = 30000UL; // microssegundos (timeout para pulseIn)
const int SAMPLE_COUNT = 3; // média de N leituras para reduzir ruído
const int LOOP_DELAY_MS = 90; // taxa de amostragem

Servo servo;

// -------- funções auxiliares ----------
double readUltrasonic(int trigPin, int echoPin) {
  // média de SAMPLE_COUNT leituras
  double sum = 0.0;
  int valid = 0;
  for (int i = 0; i < SAMPLE_COUNT; ++i) {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    unsigned long duration = pulseIn(echoPin, HIGH, PULSE_TIMEOUT);
    if (duration == 0) {
      // timeout -> retorna -1 como sinal de leitura inválida
      // não soma essa leitura
    } else {
      double dist_cm = (double)duration / 58.2; // conversão para cm
      sum += dist_cm;
      valid++;
    }
    delay(10);
  }
  if (valid == 0) return -1.0; // nenhuma leitura válida
  return sum / valid;
}

void emergencyDetachIfCommanded() {
  // Pausa por 'p' no monitor serial
  if (Serial.available() > 0) {
    char c = Serial.read();
    if (c == 'p' || c == 'P') {
      Serial.println("EMERGENCY PAUSE: detach servo and stop (reset Arduino to resume).");
      servo.detach(); // libera o servo
      while (true) {
        // trava aqui até reset manual — evita continuar e forçar o servo
      }
    }
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(TRIG_SETPOINT, OUTPUT);
  pinMode(ECHO_SETPOINT, INPUT);
  pinMode(TRIG_BALL, OUTPUT);
  pinMode(ECHO_BALL, INPUT);

  servo.attach(SERVO_PIN);
  // coloca em posição neutra próxima ao centro do range permitido
  int neutral = (SERVO_UP_ANGLE + SERVO_DOWN_ANGLE) / 2;
  servo.write(neutral);

  delay(1000);
  Serial.println("=== Controle P (Ball & Beam) - inicializado ===");
  Serial.print("Pinos: setpoint TRIG/ECHO = "); Serial.print(TRIG_SETPOINT); Serial.print("/"); Serial.println(ECHO_SETPOINT);
  Serial.print("       ball     TRIG/ECHO = "); Serial.print(TRIG_BALL); Serial.print("/"); Serial.println(ECHO_BALL);
  Serial.print("Servo pin = "); Serial.println(SERVO_PIN);
  Serial.print("Kp inicial = "); Serial.println(Kp);
  Serial.print("Comandos: enviar 'p' no Serial para pausa de emergência."); Serial.println();
}

void loop() {
  emergencyDetachIfCommanded();

  // Lê a posição do bloco
  double setpoint_raw = readUltrasonic(TRIG_SETPOINT, ECHO_SETPOINT);
  if (setpoint_raw > 0) {
    setpoint_cm = setpoint_raw; // atualiza setpoint automaticamente com a leitura do sensor do bloco
  } // se leitura inválida, mantemos o último setpoint conhecido

  // Lê a posição da bola
  double ball_pos_cm = readUltrasonic(TRIG_BALL, ECHO_BALL);

  if (ball_pos_cm < 0) {
    // leitura inválida da bola -> não alterar servo, apenas manda mensagem
    Serial.println("Leitura da bola inválida (timeout). Servo não modificado.");
    delay(LOOP_DELAY_MS);
    return;
  }

  // Erro (cm): setpoint - atual
  double error_cm = setpoint_cm - ball_pos_cm;

  // Saída proporcional (em graus) - Kp [graus por cm]
  double delta_deg = Kp * error_cm;

  // Mapeamos essa variação a partir de um ângulo neutro.
  // Usamos o ponto neutro como média entre subir e descer.
  double neutral_angle = (SERVO_UP_ANGLE + SERVO_DOWN_ANGLE) / 2.0;
  double commanded_angle = neutral_angle + delta_deg;

  // Saturações de segurança
  if (commanded_angle > SERVO_MAX) commanded_angle = SERVO_MAX;
  if (commanded_angle < SERVO_MIN) commanded_angle = SERVO_MIN;

  // limitação (20 a 180 graus)
  if (commanded_angle > SERVO_DOWN_ANGLE) commanded_angle = SERVO_DOWN_ANGLE;
  if (commanded_angle < SERVO_UP_ANGLE) commanded_angle = SERVO_UP_ANGLE;

  // Envia comando ao servo
  servo.write((int)round(commanded_angle));

  // Telemetria
  Serial.print("Setpoint(cm): "); Serial.print(setpoint_cm, 2);
  Serial.print(" | Ball(cm): "); Serial.print(ball_pos_cm, 2);
  Serial.print(" | Erro(cm): "); Serial.print(error_cm, 3);
  Serial.print(" | Delta(deg): "); Serial.print(delta_deg, 3);
  Serial.print(" | ServoCmd(°): "); Serial.println((int)round(commanded_angle));

  delay(LOOP_DELAY_MS);
}
