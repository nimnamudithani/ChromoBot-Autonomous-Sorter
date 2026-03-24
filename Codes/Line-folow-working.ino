/*
 * ChromoBot - Final Logic (Longer Alignment Fix)
 * ------------------------------------------------
 * SPEEDS: Straight = 60 | Rotate = 100
 * ALIGN TIME: Increased to 1200ms for 26cm robot length.
 * * 1. BEND: Stop 1s -> Fwd 60 (1.2s) -> Stop 1s -> Rotate -> Wait for Mid 3.
 * 2. DRIFT: Stop 1s -> Rotate [Limit] -> Wait for Mid 3.
 * 3. NORMAL: PID Speed 60.
 */

// --- PIN DEFINITIONS ---
const int L_PWM = 2; const int L_IN1 = 22; const int L_IN2 = 23;
const int R_PWM = 3; const int R_IN1 = 24; const int R_IN2 = 25;
const int S1 = 44; const int S2 = 45; const int S3 = 46; const int S4 = 47; const int S5 = 48;

// --- TUNING (UPDATED) ---
#define PID_SPEED   60      
#define ALIGN_SPEED 60      
#define TURN_SPEED  100     

#define DRIFT_LIMIT 400     // Max time to rotate for drift (ms)

// INCREASED THIS TO 1200 (1.2 Seconds)
#define ALIGN_TIME  1200    

// --- LOGIC MAP ---
#define LINE 1   // White
#define FLOOR 0  // Black

int lastError = 0;
bool isRunning = true;

#define KP 18.0
#define KD 22.0

void setup() {
  pinMode(L_PWM, OUTPUT); pinMode(L_IN1, OUTPUT); pinMode(L_IN2, OUTPUT);
  pinMode(R_PWM, OUTPUT); pinMode(R_IN1, OUTPUT); pinMode(R_IN2, OUTPUT);
  pinMode(S1, INPUT); pinMode(S2, INPUT); pinMode(S3, INPUT); pinMode(S4, INPUT); pinMode(S5, INPUT);

  Serial.begin(9600);
  Serial.println("--- READY: 1.2s Align Time ---");
  delay(2000);
}

void loop() {
  if (!isRunning) return;

  int s1 = digitalRead(S1);
  int s2 = digitalRead(S2);
  int s3 = digitalRead(S3);
  int s4 = digitalRead(S4);
  int s5 = digitalRead(S5);
  int activeSensors = s1 + s2 + s3 + s4 + s5;

  // CHECK FOR LINE LOSS (Pink Section)
  if (activeSensors == 0) {
    delay(100); // 100ms Debounce
    if (digitalRead(S3) == FLOOR) {
        stopMotors();
        isRunning = false;
        return;
    }
  }

  // ==========================================
  // PRIORITY 1: BEND DETECTION (4 or 5 Sensors)
  // ==========================================
  if (activeSensors >= 4) {
    if (s1 == LINE && s2 == LINE && s3 == LINE && s4 == LINE) {
      handleBend(-1); // Left Bend
      return;
    }
    if (s2 == LINE && s3 == LINE && s4 == LINE && s5 == LINE) {
      handleBend(1);  // Right Bend
      return;
    }
  }

  // ==========================================
  // PRIORITY 2: DRIFT DETECTION (3 Sensors)
  // ==========================================
  if (activeSensors == 3) {
    if (s1 == LINE && s2 == LINE && s3 == LINE) {
      handleDrift(-1); // Left Drift
      return;
    }
    if (s3 == LINE && s4 == LINE && s5 == LINE) {
      handleDrift(1); // Right Drift
      return;
    }
  }

  // ==========================================
  // PRIORITY 3: NORMAL PID
  // ==========================================
  if (activeSensors > 0) {
    int error = (s1 * -4) + (s2 * -2) + (s3 * 0) + (s4 * 2) + (s5 * 4);
    error = error / activeSensors;

    int P = error;
    int D = error - lastError;
    int correction = (KP * P) + (KD * D);
    lastError = error;

    move(PID_SPEED + correction, PID_SPEED - correction);
  }
}

// --- MANEUVERS ---

void handleBend(int direction) {
  // 1. Stop 1s
  stopMotors(); delay(1000);

  // 2. Blind Forward (1200ms now)
  // Main loop is paused here, so it ignores black floor.
  move(ALIGN_SPEED, ALIGN_SPEED);
  delay(ALIGN_TIME);

  // 3. Stop 1s
  stopMotors(); delay(1000);

  // 4. Rotate until Center Found
  spinUntilMiddle3(direction, 6000); 
}

void handleDrift(int direction) {
  stopMotors(); delay(1000);
  spinUntilMiddle3(direction, DRIFT_LIMIT);
}

void spinUntilMiddle3(int direction, int timeoutMs) {
  unsigned long startTime = millis();
  
  // SPIN LOGIC (Speed 100)
  if (direction == -1) {
     // Spin Left (Left Back, Right Fwd)
     digitalWrite(L_IN1, LOW); digitalWrite(L_IN2, HIGH); analogWrite(L_PWM, TURN_SPEED);
     digitalWrite(R_IN1, HIGH); digitalWrite(R_IN2, LOW); analogWrite(R_PWM, TURN_SPEED);
  } else {
     // Spin Right
     digitalWrite(L_IN1, HIGH); digitalWrite(L_IN2, LOW); analogWrite(L_PWM, TURN_SPEED);
     digitalWrite(R_IN1, LOW); digitalWrite(R_IN2, HIGH); analogWrite(R_PWM, TURN_SPEED);
  }

  delay(100); // Clear state

  while (millis() - startTime < timeoutMs) {
    int s2 = digitalRead(S2);
    int s3 = digitalRead(S3);
    int s4 = digitalRead(S4);

    // Target: Middle 3 White
    if (s2 == LINE && s3 == LINE && s4 == LINE) {
      stopMotors();
      delay(200); 
      return;     
    }
  }
  stopMotors();
}

// --- MOTORS ---
void move(int leftSpeed, int rightSpeed) {
  leftSpeed = constrain(leftSpeed, -255, 255);
  rightSpeed = constrain(rightSpeed, -255, 255);

  if (leftSpeed >= 0) {
    digitalWrite(L_IN1, LOW); digitalWrite(L_IN2, HIGH); analogWrite(L_PWM, leftSpeed);
  } else {
    digitalWrite(L_IN1, HIGH); digitalWrite(L_IN2, LOW); analogWrite(L_PWM, abs(leftSpeed));
  }

  if (rightSpeed >= 0) {
    digitalWrite(R_IN1, LOW); digitalWrite(R_IN2, HIGH); analogWrite(R_PWM, rightSpeed);
  } else {
    digitalWrite(R_IN1, HIGH); digitalWrite(R_IN2, LOW); analogWrite(R_PWM, abs(rightSpeed));
  }
}

void stopMotors() {
  analogWrite(L_PWM, 0); analogWrite(R_PWM, 0);
  digitalWrite(L_IN1, LOW); digitalWrite(L_IN2, LOW);
  digitalWrite(R_IN1, LOW); digitalWrite(R_IN2, LOW);
}