/*
 * ChromoBot - Motor Test
 * ----------------------
 * 1. Forward (2s)
 * 2. Backward (2s)
 * 3. Spin Left (2s)
 * 4. Spin Right (2s)
 * 5. Stop (2s)
 */

// --- MOTOR PINS ---
const int L_PWM = 2; const int L_IN1 = 22; const int L_IN2 = 23;
const int R_PWM = 3; const int R_IN1 = 24; const int R_IN2 = 25;

void setup() {
  // Set all motor pins as outputs
  pinMode(L_PWM, OUTPUT); pinMode(L_IN1, OUTPUT); pinMode(L_IN2, OUTPUT);
  pinMode(R_PWM, OUTPUT); pinMode(R_IN1, OUTPUT); pinMode(R_IN2, OUTPUT);
  
  Serial.begin(9600);
  Serial.println("--- MOTOR TEST STARTING ---");
  delay(2000);
}

void loop() {
  // 1. FORWARD
  Serial.println("FORWARD");
  move(100, 100); 
  delay(2000);
  stopMotors(); delay(1000);

  // 2. BACKWARD
  Serial.println("BACKWARD");
  move(-100, -100); 
  delay(2000);
  stopMotors(); delay(1000);

  // 3. SPIN LEFT
  Serial.println("SPIN LEFT");
  move(-100, 100); 
  delay(2000);
  stopMotors(); delay(1000);

  // 4. SPIN RIGHT
  Serial.println("SPIN RIGHT");
  move(100, -100); 
  delay(2000);
  stopMotors(); delay(2000);
}

// --- HELPER FUNCTION ---
void move(int l, int r) {
  // Left Motor Logic
  if(l > 0) {
    digitalWrite(L_IN1, LOW); digitalWrite(L_IN2, HIGH); analogWrite(L_PWM, l);
  } else {
    digitalWrite(L_IN1, HIGH); digitalWrite(L_IN2, LOW); analogWrite(L_PWM, abs(l));
  }

  // Right Motor Logic
  if(r > 0) {
    digitalWrite(R_IN1, LOW); digitalWrite(R_IN2, HIGH); analogWrite(R_PWM, r);
  } else {
    digitalWrite(R_IN1, HIGH); digitalWrite(R_IN2, LOW); analogWrite(R_PWM, abs(r));
  }
}

void stopMotors() {
  analogWrite(L_PWM, 0); analogWrite(R_PWM, 0);
  digitalWrite(L_IN1, LOW); digitalWrite(L_IN2, LOW);
  digitalWrite(R_IN1, LOW); digitalWrite(R_IN2, LOW);
}