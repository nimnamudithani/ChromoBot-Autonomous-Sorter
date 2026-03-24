/*
 * ChromoBot - 3 Ultrasonic Sensor Test
 * ------------------------------------------------
 * Upload this to verify all sensors are working.
 * Open Serial Monitor at 9600 baud.
 */

// --- PINS ---
const int TRIG_L = 7;   const int ECHO_L = 8;
const int TRIG_MID = 10; const int ECHO_MID = 9;
const int TRIG_R = 11;  const int ECHO_R = 12;

void setup() {
  Serial.begin(9600);

  pinMode(TRIG_L, OUTPUT); pinMode(ECHO_L, INPUT);
  pinMode(TRIG_MID, OUTPUT); pinMode(ECHO_MID, INPUT);
  pinMode(TRIG_R, OUTPUT); pinMode(ECHO_R, INPUT);

  Serial.println("--- ULTRASONIC TEST STARTED ---");
  Serial.println("L = Left | M = Middle | R = Right");
  delay(1000);
}

void loop() {
  // Read all three
  long distL = readDistance(TRIG_L, ECHO_L);
  delay(10); // Small pause to prevent interference
  long distM = readDistance(TRIG_MID, ECHO_MID);
  delay(10);
  long distR = readDistance(TRIG_R, ECHO_R);

  // Print nicely
  Serial.print("L: ");
  Serial.print(distL);
  Serial.print(" cm  |  M: ");
  Serial.print(distM);
  Serial.print(" cm  |  R: ");
  Serial.print(distR);
  Serial.println(" cm");

  delay(500); // Update every half second
}

// Helper function
long readDistance(int trig, int echo) {
  digitalWrite(trig, LOW); delayMicroseconds(2);
  digitalWrite(trig, HIGH); delayMicroseconds(10);
  digitalWrite(trig, LOW);

  long duration = pulseIn(echo, HIGH, 30000); // 30ms timeout (~5m)
  
  if (duration == 0) return 999; // Return 999 if no echo (out of range/disconnected)
  return duration * 0.0343 / 2;
}
