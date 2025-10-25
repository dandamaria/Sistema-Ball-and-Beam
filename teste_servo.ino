#include <Servo.h>

Servo servo;

int angulo = 90;   // início - começa no "meio"
int minAng = 0;   // mínimo
int maxAng = 180; // máximo

void setup() {
  Serial.begin(9600);
  servo.attach(9);
  Serial.println("Teste de varredura iniciado: ");
}

void loop() {

  //Vai para o ângulo máximo
  //começa no ângulo 90
  for (angulo = angulo; angulo <= maxAng; angulo += 10) {
    servo.write(angulo);
    Serial.print("Ângulo: ");
    Serial.println(angulo);
    delay(300);
  }
  
  //Vai para o ângulo mínimo
  for (angulo = maxAng; angulo >= minAng; angulo -= 10) {
    servo.write(angulo);
    Serial.print("Ângulo: ");
    Serial.println(angulo);
    delay(300);
  }


}