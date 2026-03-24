#include <Wire.h>
#include "Adafruit_TCS34725.h"

Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_1X);

void setup() {
  Serial.begin(9600);
  if (!tcs.begin()) {
    Serial.println("No TCS34725 found ... check your connections");
    while (1);
  }
}

void loop() {
  float r, g, b;
  
  // This function gets the RGB data tailored for standard RGB (0-255)
  tcs.getRGB(&r, &g, &b);

  Serial.print("R: "); Serial.print((int)r); Serial.print(" ");
  Serial.print("G: "); Serial.print((int)g); Serial.print(" ");
  Serial.print("B: "); Serial.print((int)b); Serial.print(" ");
  
  // Simple logic test
  if ((int)r > (int)g && (int)r > (int)b) {
    Serial.println("-> Detected: RED");
  } 
  else if ((int)g > (int)r && (int)g > (int)b) {
    Serial.println("-> Detected: GREEN");
  } 
  else if ((int)b > (int)r && (int)b > (int)g) {
    Serial.println("-> Detected: BLUE");
  } 
  else {
    Serial.println("-> Ambiguous");
  }

  delay(100); 
}
