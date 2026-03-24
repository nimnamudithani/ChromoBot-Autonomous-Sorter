/*
 * IR Sensor Truth Test
 * --------------------
 * READ THIS OUTPUT TO KNOW YOUR LOGIC:
 * - Put Sensor on WHITE -> Note the number (0 or 1).
 * - Put Sensor on BLACK -> Note the number (0 or 1).
 */

// PINS (Based on your previous code)
const int S1 = 44; 
const int S2 = 45; 
const int S3 = 46; 
const int S4 = 47; 
const int S5 = 48;

void setup() {
  pinMode(S1, INPUT);
  pinMode(S2, INPUT);
  pinMode(S3, INPUT);
  pinMode(S4, INPUT);
  pinMode(S5, INPUT);

  Serial.begin(9600);
  Serial.println("--- IR SENSOR TEST STARTED ---");
  delay(1000);
}

void loop() {
  // Read all sensors
  int v1 = digitalRead(S1);
  int v2 = digitalRead(S2);
  int v3 = digitalRead(S3);
  int v4 = digitalRead(S4);
  int v5 = digitalRead(S5);

  // Print to Serial Monitor
  Serial.print("S1:"); Serial.print(v1); Serial.print("  ");
  Serial.print("S2:"); Serial.print(v2); Serial.print("  ");
  Serial.print("S3:"); Serial.print(v3); Serial.print("  ");
  Serial.print("S4:"); Serial.print(v4); Serial.print("  ");
  Serial.print("S5:"); Serial.println(v5);

  delay(200); // Slow down so you can read it
}