#define trigPin 3
#define echoPin 4

void setup() {
  Serial.begin(9600);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  delay(2000);
  Serial.println("Diagnóstico HC-SR04");
}

void loop() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Timeout em microssegundos: 30000us = 30ms -> distância máxima teórica ~ (30000/58.2) ≈ 515 cm
  unsigned long duration = pulseIn(echoPin, HIGH, 30000UL);

  if (duration == 0) {
    Serial.println("Timeout (sem eco) — objeto muito longe / sem reflexão / mau contato");
  } else {
    float distance_cm = duration / 58.2;
    Serial.print("Duration (us): ");
    Serial.print(duration);
    Serial.print("  -> Distância: ");
    Serial.print(distance_cm);
    Serial.println(" cm");
  }

  delay(300);
}