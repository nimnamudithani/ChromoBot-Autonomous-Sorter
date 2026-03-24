# ChromoBot: Autonomous Color-Sorting & Navigation Robot

ChromoBot is an advanced autonomous robotics system developed for the NIBM HND in Software Engineering (25.2F). It features a sophisticated multi-stage mission logic that enables precise line following, wall following (tunnel navigation), and object color identification using a State Machine architecture.

## 🚀 Key Features
* **PID-Controlled Line Following:** Uses a Proportional-Derivative (PD) algorithm for high-speed, smooth navigation with weighted error calculation.
* **Intelligent State Machine:** Manages complex mission phases (Tunnel, Alignment, Sequence, and Sorting) automatically using `enum` states.
* **Dual-Mode Navigation:** Automatically switches between IR-based line tracking and Ultrasonic-based wall following for tunnel sections.
* **Precision Color Recognition:** Uses the TCS34725 RGB sensor with digital signal averaging to accurately identify and sort Blue and Green objects.
* **Obstacle & Environment Awareness:** Triple-ultrasonic sensor array (Left, Right, Mid) for 180-degree sensing and distance-based objective triggers.

## 🛠️ Hardware Architecture
The robot is built on a custom-designed PCB and a high-clearance 4WD chassis:
* **Controller:** Arduino Mega 2560 (to handle high I/O and multiple I2C/Serial buses).
* **Motor Control:** TB6612FNG Dual DC Motor Driver for efficient, high-frequency PWM delivery.
* **Power Management:** LM2596 Buck Converter for regulated 5V/6V logic and motor rails.
* **Sensor Suite:**
    * 5-Channel IR Reflectance Array (Line Tracking).
    * TCS34725 I2C RGB Color Sensor (Object Identification).
    * 3x HC-SR04 Ultrasonic Sensors (Tunnel & Obstacle Detection).
    * PCA9685 16-channel PWM Driver (Servo Gripper Control).

## 💻 Software Intelligence

### 1. Finite State Machine (FSM)
The firmware is structured to ensure the robot applies the correct logic for each specific part of the track map:
`LINE_FOLLOW` ➔ `TUNNEL` ➔ `POST_TUNNEL_ALIGN` ➔ `COLOR_CHECK` ➔ `DROP_ACTION`

### 2. PID Control Logic
The robot calculates its position error using a weighted average of the 5 IR sensors. This allows for proportional motor response—the further the robot drifts, the harder it turns back.

```cpp
// Logic for centered navigation
int error = (s1 * -4) + (s2 * -2) + (s3 * 0) + (s4 * 2) + (s5 * 4);
error /= active; // Normalizes for thick lines

int P = error;
int D = error - lastError;
int correction = (KP * P) + (KD * D);

move(PID_SPEED + correction, PID_SPEED - correction);
