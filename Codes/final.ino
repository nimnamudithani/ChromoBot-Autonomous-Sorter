#include <Wire.h>
#include "Adafruit_TCS34725.h"

// ================= SETUP TCS34725 =================
// 50ms integration time, 1x gain for accurate close-range reading
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_1X);

// ================= STATES =================
enum RobotState {
  LINE_FOLLOW,          // Phase 1: Start -> Tunnel
  TUNNEL,               // Phase 2: Wall Follow
  POST_TUNNEL_ALIGN,    // Phase 3: Exit Alignment
  SEQUENCE_BLUE,        // Phase 3: Navigate to Blue Tower
  COLOR_CHECK_BLUE,     // Phase 4: Identify Blue
  TO_DROP_BLUE,         // Phase 5: Go to Drop Point (Ignore Right)
  DROP_ACTION_BLUE,     // Phase 5: Drop Logic
  TO_GREEN_TOWER,       // Phase 6: Navigate to Green (Left -> Ignore Right -> Right)
  COLOR_CHECK_GREEN,    // Phase 6: Identify Green
  TO_DROP_GREEN,        // Phase 7: Go to Drop Point (Ignore Left)
  DROP_ACTION_GREEN,    // Phase 7: Drop Logic
  MISSION_COMPLETE      // Stop
};

RobotState state = LINE_FOLLOW;

// ================= PINS =================
// MOTORS
const int L_PWM = 2; const int L_IN1 = 22; const int L_IN2 = 23;
const int R_PWM = 3; const int R_IN1 = 24; const int R_IN2 = 25;

// IR SENSORS (Left to Right)
const int S1 = 44; const int S2 = 45; const int S3 = 46; const int S4 = 47; const int S5 = 48;

// ULTRASONICS
const int TRIG_L = 7;   const int ECHO_L = 8;
const int TRIG_R = 11;  const int ECHO_R = 12;
const int TRIG_MID = 10; const int ECHO_MID = 9;

// ================= TUNING =================
#define PID_SPEED   60
#define TURN_SPEED  100
#define ALIGN_SPEED 60

// TIMERS (Adjust these based on robot speed/battery)
#define ALIGN_TIME  350     // Time to drive STRAIGHT before turning
#define BLIND_TIME  400     // Time to ignore junctions (Loophole Fix)
#define UTURN_TIME  800     // Blind spin time for U-Turn

// PID
#define KP 18.0
#define KD 22.0

// TUNNEL
#define TUNNEL_BASE_SPEED 60
#define WALL_KP 3

// DISTANCES
#define PICKUP_MIN 8
#define PICKUP_MAX 20

// LOGIC
#define LINE 1
#define FLOOR 0

// ================= GLOBALS =================
int lastError = 0;
int tJunctionsPassed = 0; // Re-used counter for phases
bool tunnelFinished = false;

// ==========================================

void setup() {
  Serial.begin(9600);
  
  // Motor Pins
  pinMode(L_PWM, OUTPUT); pinMode(L_IN1, OUTPUT); pinMode(L_IN2, OUTPUT);
  pinMode(R_PWM, OUTPUT); pinMode(R_IN1, OUTPUT); pinMode(R_IN2, OUTPUT);
  
  // Sensor Pins
  pinMode(S1, INPUT); pinMode(S2, INPUT); pinMode(S3, INPUT); pinMode(S4, INPUT); pinMode(S5, INPUT);
  pinMode(TRIG_L, OUTPUT); pinMode(ECHO_L, INPUT);
  pinMode(TRIG_R, OUTPUT); pinMode(ECHO_R, INPUT);
  pinMode(TRIG_MID, OUTPUT); pinMode(ECHO_MID, INPUT);

  // Color Sensor Init
  if (tcs.begin()) {
    Serial.println("Sensor Found");
  } else {
    Serial.println("No TCS34725 found ... check connections");
    while (1); // Halt
  }
  
  Serial.println("--- MISSION START ---");
  delay(2000);
}

void loop() {
  if (state == MISSION_COMPLETE) {
      stopMotors();
      return;
  }

  // READ SENSORS
  int s1 = digitalRead(S1); int s2 = digitalRead(S2);
  int s3 = digitalRead(S3); int s4 = digitalRead(S4); int s5 = digitalRead(S5);
  int active = s1 + s2 + s3 + s4 + s5;

  // ============================================
  //             PHASE 7: GREEN DROP
  // ============================================
  if (state == TO_DROP_GREEN) {
      // 1. ARRIVAL: Line Ends (All Black)
      if (active == 0) {
          // Double check to ensure it's not a gap
          delay(50);
          if (digitalRead(S3) == FLOOR) { 
             // Logic says active 0 is black line, but confirm it's not a hole
             // Assuming White Line on Black Floor: 0 active means NO LINE (Black).
             stopMotors();
             state = DROP_ACTION_GREEN;
             return;
          }
      }
      
      // 2. LOOPHOLE FIX: Ignore LEFT Turns (Passing 3rd T-Junc)
      // If we see Left Wing (S1+S2) or Junction (4+)
      if (active >= 4 || (s1 && s2)) {
          Serial.println("-> Ignoring Left Turn (Blind Forward)");
          ignoreTurn();
          return; 
      }

      runPID(s1, s2, s3, s4, s5, active);
      return;
  }

  if (state == DROP_ACTION_GREEN) {
      Serial.println("--- DROPPING GREEN CUBE ---");
      delay(1000); // Simulate Drop
      state = MISSION_COMPLETE;
      return;
  }

  // ============================================
  //             PHASE 6: TO GREEN TOWER
  // ============================================
  if (state == TO_GREEN_TOWER) {
      
      // Check Ultrasonic for Arrival (Only valid after the final Right turn)
      if (tJunctionsPassed == 3) {
          int d = getDistance(TRIG_MID, ECHO_MID);
          if (d >= PICKUP_MIN && d <= PICKUP_MAX) {
              stopMotors();
              state = COLOR_CHECK_GREEN;
              return;
          }
      }

      // Intersection Handling
      if (active >= 4 || (s1&&s2) || (s4&&s5)) {
          if (verifyIntersection()) {
              
              // Turn 1: 1st T-Junc -> LEFT
              if (tJunctionsPassed == 0) {
                  executeTurn(-1); // Left
                  tJunctionsPassed++;
                  return;
              }
              
              // Loophole: Passing Tunnel T (Right Side) -> IGNORE
              if (tJunctionsPassed == 1) {
                   Serial.println("-> Crossing Tunnel T (Blind Forward)");
                   ignoreTurn();
                   tJunctionsPassed++; // Increment to signify we passed it
                   return;
              }

              // Turn 2: 3rd T-Junc -> RIGHT
              if (tJunctionsPassed == 2) {
                  executeTurn(1); // Right
                  tJunctionsPassed++;
                  return;
              }
          }
      }
      runPID(s1, s2, s3, s4, s5, active);
      return;
  }

  if (state == COLOR_CHECK_GREEN) {
      checkColorAverage(false); // Check Green
      performUTurn();
      state = TO_DROP_GREEN;
      return;
  }

  // ============================================
  //             PHASE 5: TO BLUE DROP
  // ============================================
  if (state == TO_DROP_BLUE) {
      // 1. ARRIVAL: Line Ends
      if (active == 0) {
          delay(50);
          // Assuming active 0 = All Black (Drop Point)
          stopMotors();
          state = DROP_ACTION_BLUE;
          return;
      }

      // 2. LOOPHOLE FIX: Ignore RIGHT Turns (Passing 1st T-Junc)
      if (active >= 4 || (s4 && s5)) {
          Serial.println("-> Ignoring Right Turn (Blind Forward)");
          ignoreTurn();
          return;
      }
      
      runPID(s1, s2, s3, s4, s5, active);
      return;
  }

  if (state == DROP_ACTION_BLUE) {
      Serial.println("--- DROPPING BLUE CUBE ---");
      delay(1000);
      
      performUTurn();
      tJunctionsPassed = 0; // Reset counter for Phase 6
      state = TO_GREEN_TOWER;
      return;
  }

  // ============================================
  //             PHASE 4: COLOR CHECK 1
  // ============================================
  if (state == COLOR_CHECK_BLUE) {
      checkColorAverage(true); // Check Blue
      
      performUTurn();
      state = TO_DROP_BLUE;
      return;
  }

  // ============================================
  //             PHASE 3: SEQUENCE BLUE
  // ============================================
  if (state == SEQUENCE_BLUE) {
      // Check Arrival
      if (tJunctionsPassed >= 2) {
          int d = getDistance(TRIG_MID, ECHO_MID);
          if (d >= PICKUP_MIN && d <= PICKUP_MAX) {
              stopMotors();
              state = COLOR_CHECK_BLUE;
              return;
          }
      }

      // Check Turns
      if (active >= 4 || (s1&&s2) || (s4&&s5)) {
          if (verifyIntersection()) {
              if (tJunctionsPassed < 2) {
                  executeTurn(-1); // Force Left x2
                  tJunctionsPassed++;
              }
          }
      }
      runPID(s1, s2, s3, s4, s5, active);
      return;
  }

  // ============================================
  //             PHASE 2: TUNNEL EXIT
  // ============================================
  if (state == POST_TUNNEL_ALIGN) {
      // Auto-Balance: Scan if we are crooked
      if (s2 && s3 && s4) {
          stopMotors(); delay(200);
          state = SEQUENCE_BLUE;
          tJunctionsPassed = 0;
          return;
      }
      if (s1 || s2) spinUntilMiddle3(-1); // Spin Left
      else if (s4 || s5) spinUntilMiddle3(1); // Spin Right
      
      // Failsafe: if s3 only is active, we are good
      if (s3) {
          state = SEQUENCE_BLUE; 
          tJunctionsPassed = 0;
      }
      return;
  }

  if (state == TUNNEL) {
      // Exit Condition: Any Line Detected
      if (s1 == LINE || s3 == LINE || s5 == LINE) {
          stopMotors();
          delay(500); // Settle
          state = POST_TUNNEL_ALIGN;
          return;
      }
      // Wall Follow
      int dl = getDistance(TRIG_L, ECHO_L);
      int dr = getDistance(TRIG_R, ECHO_R);
      int error = dl - dr;
      int corr = error * WALL_KP;
      move(TUNNEL_BASE_SPEED - corr, TUNNEL_BASE_SPEED + corr);
      delay(40);
      return;
  }

  // ============================================
  //             PHASE 1: LINE FOLLOW
  // ============================================
  if (state == LINE_FOLLOW) {
      // Check Tunnel Entry
      if (active == 0) {
          delay(50);
          if (digitalRead(S3) == FLOOR) { // Confirm All Black + Floor Ground
              stopMotors(); delay(500);
              state = TUNNEL;
              return;
          }
      }
      
      // Phase 1 Turns (Standard)
      if (active >= 4 || (s1&&s2) || (s4&&s5)) {
          if (verifyIntersection()) {
              if (s1 && s2) executeTurn(-1);      // Left
              else if (s4 && s5) executeTurn(1);  // Right
          }
      }
      runPID(s1, s2, s3, s4, s5, active);
  }
}

// ==========================================
//             HELPER FUNCTIONS
// ==========================================

// ROBUST COLOR CHECKER
void checkColorAverage(bool lookForBlue) {
    long rSum = 0, gSum = 0, bSum = 0;
    
    // Take 5 samples
    for(int i=0; i<5; i++) {
        float r, g, b;
        tcs.getRGB(&r, &g, &b);
        rSum += (long)r; gSum += (long)g; bSum += (long)b;
        delay(20);
    }
    long r = rSum/5; long g = gSum/5; long b = bSum/5;
    
    Serial.print("Avg Color: R"); Serial.print(r); Serial.print(" G"); Serial.print(g); Serial.print(" B"); Serial.println(b);

    // Simple Ratio Logic (Adjust 1.2 multiplier if needed)
    bool isBlue  = (b > r * 1.2 && b > g * 1.2);
    bool isGreen = (g > r * 1.2 && g > b * 1.2);

    if (lookForBlue && isBlue) {
        Serial.println("-> BLUE CONFIRMED: Picking Up...");
        delay(2000); // Simulate Arm
    } 
    else if (!lookForBlue && isGreen) {
        Serial.println("-> GREEN CONFIRMED: Picking Up...");
        delay(2000); // Simulate Arm
    }
    else {
        Serial.println("-> Color Mismatch or Ambiguous");
        delay(500);
    }
}

void performUTurn() {
    Serial.println("-> U-TURN");
    stopMotors(); delay(500);
    move(TURN_SPEED, -TURN_SPEED); // Spin
    delay(UTURN_TIME); // Blind Spin to clear line
    spinUntilMiddle3(1); // Seek Line (Spin Right)
}

void ignoreTurn() {
    // Blindly move forward to clear the intersection
    move(PID_SPEED, PID_SPEED);
    delay(BLIND_TIME);
}

bool verifyIntersection() {
    move(PID_SPEED, PID_SPEED); 
    delay(50); // Move into the intersection slightly
    stopMotors();
    
    int r1 = digitalRead(S1); int r2 = digitalRead(S2);
    int r3 = digitalRead(S3); int r4 = digitalRead(S4); int r5 = digitalRead(S5);
    int active = r1 + r2 + r3 + r4 + r5;
    
    return (active >= 3); 
}

void executeTurn(int dir) {
    stopMotors(); delay(200);
    move(ALIGN_SPEED, ALIGN_SPEED); 
    delay(ALIGN_TIME); // Align wheels with center of turn
    stopMotors();
    spinUntilMiddle3(dir);
}

void spinUntilMiddle3(int dir) {
    if (dir == -1) move(-TURN_SPEED, TURN_SPEED); // Left
    else move(TURN_SPEED, -TURN_SPEED); // Right
    
    // Safety timeout to prevent infinite loop
    unsigned long start = millis();
    while (millis() - start < 5000) {
        if (digitalRead(S2) && digitalRead(S3) && digitalRead(S4)) {
            stopMotors(); delay(150); return;
        }
    }
    stopMotors(); // Timeout stop
}

void runPID(int s1, int s2, int s3, int s4, int s5, int active) {
    if (active == 0) return; 
    int error = (s1 * -4) + (s2 * -2) + (s3 * 0) + (s4 * 2) + (s5 * 4);
    error /= active;
    
    int P = error;
    int D = error - lastError;
    int corr = (KP * P) + (KD * D);
    lastError = error;
    
    move(PID_SPEED + corr, PID_SPEED - corr);
}

void move(int l, int r) {
    l = constrain(l, -255, 255); r = constrain(r, -255, 255);
    if (l >= 0) { digitalWrite(L_IN1, LOW); digitalWrite(L_IN2, HIGH); }
    else { digitalWrite(L_IN1, HIGH); digitalWrite(L_IN2, LOW); }
    analogWrite(L_PWM, abs(l));
    
    if (r >= 0) { digitalWrite(R_IN1, LOW); digitalWrite(R_IN2, HIGH); }
    else { digitalWrite(R_IN1, HIGH); digitalWrite(R_IN2, LOW); }
    analogWrite(R_PWM, abs(r));
}

void stopMotors() {
    analogWrite(L_PWM, 0); analogWrite(R_PWM, 0);
    digitalWrite(L_IN1, LOW); digitalWrite(L_IN2, LOW);
    digitalWrite(R_IN1, LOW); digitalWrite(R_IN2, LOW);
}

int getDistance(int trig, int echo) {
    digitalWrite(trig, LOW); delayMicroseconds(2);
    digitalWrite(trig, HIGH); delayMicroseconds(10);
    digitalWrite(trig, LOW);
    long dur = pulseIn(echo, HIGH, 30000);
    if (dur == 0) return 999;
    return dur * 0.0343 / 2;
}