#include <Arduino.h>
#include <Wire.h>
#include <RTClib.h>
#include <AccelStepper.h>

// Define motor control pins
const int IN1 = 8;
const int IN2 = 9;
const int IN3 = 10;
const int IN4 = 11;

// Disable motor coils to prevent heating
void disableCoils() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

// Stepper motor setup
const int stepsPerRevolution = 2048; // 360 degrees
const int maxDegrees = 180;
const int totalDaySteps = 96;
const int maxTotalSteps = (stepsPerRevolution * maxDegrees) / 360;
const int stepsPerMove = maxTotalSteps / totalDaySteps;

AccelStepper stepper(AccelStepper::HALF4WIRE, IN1, IN3, IN2, IN4);

// RTC setup
RTC_DS3231 rtc;

// Sunrise/Sunset times per month (in minutes since midnight)
const int sunriseTable[12] = {450, 420, 375, 345, 315, 300, 330, 360, 390, 420, 435, 450};
const int sunsetTable[12]  = {990, 1035, 1095, 1140, 1185, 1200, 1200, 1155, 1110, 1065, 1005, 990};

// Simulation tracking
unsigned long simStartMillis;
unsigned long lastStepMillis = 0;
int stepsMoved = 0;

void setup() {
  Serial.begin(115200);

  // Initialize stepper motor
  stepper.setMaxSpeed(500);
  stepper.setSpeed(300); // Constant speed for simulation

  // Initialize RTC
  if (!rtc.begin()) {
    Serial.println("RTC not found!");
    while (1);
  }

  // Uncomment to set RTC to current time
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  simStartMillis = millis();
}

void loop() {
  DateTime now = rtc.now();
  int month = now.month() - 1;

  unsigned long simElapsed = millis() - simStartMillis;

  // End of simulated day
  if (simElapsed > 144000) {
    Serial.println("Simulated sunset. Resetting for next cycle.");
    stepper.moveTo(0); // Return to starting position
    while (stepper.distanceToGo() != 0) {
      stepper.run();
    }
    disableCoils();

    stepsMoved = 0;
    simStartMillis = millis();
    return;
  }

  // Calculate simulated minutes since midnight
  float simMinutes = (simElapsed / 144000.0) * 1440.0;
  int currentMinuteOfDay = (int)simMinutes;

  int sunrise = sunriseTable[month];
  int sunset = sunsetTable[month];

  // Move during daylight hours
  if (currentMinuteOfDay >= sunrise && currentMinuteOfDay < sunset) {
    if (millis() - lastStepMillis >= 1500 && stepsMoved + stepsPerMove <= maxTotalSteps) {
      stepper.moveTo(stepsMoved + stepsPerMove);
      while (stepper.distanceToGo() != 0) {
        stepper.run();
      }
      disableCoils();

      stepsMoved += stepsPerMove;
      lastStepMillis = millis();

      Serial.print("Sim minute: ");
      Serial.print(currentMinuteOfDay);
      Serial.print(" | Step moved. Total steps: ");
      Serial.println(stepsMoved);
    }
  }
}
