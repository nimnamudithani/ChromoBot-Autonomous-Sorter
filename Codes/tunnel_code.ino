/*
 * ChromoBot - Tunnel Only (Strict AND Logic)
 * ------------------------------------------------
 * PINS: Left(7,8) | Right(11,12)
 * RANGE: 6cm - 20cm
 * BEHAVIOR: 
 * - Robot moves ONLY if BOTH walls are detected.
 * - If one wall is missing/far (>20cm), Robot STOPS.
 * - If one wall is too close (<6cm), Robot STOPS.
 */

// --- MOTOR PINS ---
const int L_PWM = 2; const int L_IN1 = 22; const int L_IN2 = 23;
const int R_PWM = 3; const int R_IN1 = 24; const int R_IN2 = 25;

// --- ULTRASONIC PINS (Corrected) ---
const int TRIG_L = 7;  const int ECHO_L = 8;  
const int TRIG_R = 11; const int ECHO_R = 12; 

// --- TUNING ---
#define BASE_SPEED  60    
#define WALL_KP     2     // Sensitivity

// --- STRICT RANGES ---
#define MIN_WALL_DIST 6   
#define MAX_WALL_DIST 20  

void setup() {
  // Motor Setup
  pinMode(L_PWM, OUTPUT); pinMode(L_IN1, OUTPUT); pinMode(L_IN2, OUTPUT);
  pinMode(R_PWM, OUTPUT); pinMode(R_IN1, OUTPUT); pinMode(R_IN2, OUTPUT);
  
  // Sensor Setup
  pinMode(TRIG_L, OUTPUT); pinMode(ECHO_L, INPUT);
  pinMode(TRIG_R, OUTPUT); pinMode(ECHO_R, INPUT);

  Serial.begin(9600);
  Serial.println("--- STRICT TUNNEL MODE START ---");
  delay(2000);
}

void loop() {
  // 1. READ SENSORS
  int distLeft  = getDistance(TRIG_L, ECHO_L);
  int distRight = getDistance(TRIG_R, ECHO_R);

  Serial.print("L: "); Serial.print(distLeft);
  Serial.print(" | R: "); Serial.println(distRight);

  // 2. CHECK VALIDITY
  bool validLeft  = (distLeft >= MIN_WALL_DIST && distLeft <= MAX_WALL_DIST);
  bool validRight = (distRight >= MIN_WALL_DIST && distRight <= MAX_WALL_DIST);

  // 3. STRICT DECISION (AND Logic)
  if (validLeft && validRight) {
    // CONDITION: BOTH sensors are safe.
    // ACTION: Calculate error and move.
    
    int error = distLeft - distRight;
    int correction = error * WALL_KP;

    move(BASE_SPEED - correction, BASE_SPEED + correction);
  }
  else {
    // CONDITION: One or Both sensors are unsafe.
    // ACTION: Immediate Stop.
    stopMotors();
  }
  
  delay(50); // Loop stability
}

// --- HELPERS ---

int getDistance(int trig, int echo) {
  digitalWrite(trig, LOW); delayMicroseconds(2);
  digitalWrite(trig, HIGH); delayMicroseconds(10);
  digitalWrite(trig, LOW);
  
  long duration = pulseIn(echo, HIGH, 30000); 
  if (duration == 0) return 999; 
  return duration * 0.0343 / 2;
}

void move(int leftSpeed, int rightSpeed) {
  leftSpeed = constrain(leftSpeed, 0, 120); 
  rightSpeed = constrain(rightSpeed, 0, 120);

  if (leftSpeed > 0) {
    digitalWrite(L_IN1, LOW); digitalWrite(L_IN2, HIGH); analogWrite(L_PWM, leftSpeed);
  } else {
    digitalWrite(L_IN1, LOW); digitalWrite(L_IN2, LOW); analogWrite(L_PWM, 0);
  }

  if (rightSpeed > 0) {
    digitalWrite(R_IN1, LOW); digitalWrite(R_IN2, HIGH); analogWrite(R_PWM, rightSpeed);
  } else {
    digitalWrite(R_IN1, LOW); digitalWrite(R_IN2, LOW); analogWrite(R_PWM, 0);
  }
}

void stopMotors() {
  analogWrite(L_PWM, 0); analogWrite(R_PWM, 0);
  digitalWrite(L_IN1, LOW); digitalWrite(L_IN2, LOW);
  digitalWrite(R_IN1, LOW); digitalWrite(R_IN2, LOW);
}