#include <Arduino.h>
#include <Wire.h>
#include <RTClib.h>
#include <Stepper.h>

// === Forward declaration ===
void disableCoils();

// === RTC + Stepper setup ===
RTC_DS3231 rtc;

const int stepsPerRevolution = 2038;          // 360 °
const int halfTurnSteps      = stepsPerRevolution / 2; // ≈ 1019 steps ≈ 180 °
const int stepIncrement      = 21;            // one daylight nudge (adjust to taste)
const int direction          = -1;            // ‑1 = East→West daylight sweep

Stepper myStepper(stepsPerRevolution, 8, 9, 10, 11);

int lastSecond = -1;         // guard so we only trigger once per 15 s slot
int stepsMoved = 0;          // magnitude moved since sunrise
bool resetDone = false;      // prevents multiple resets in the same minute

void setup() {
  Serial.begin(115200);
  myStepper.setSpeed(5);     // RPM — slow & steady

  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  // Uncomment ONCE to set RTC to your PC compile time
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
}

void loop() {
  DateTime now = rtc.now();
  int second = now.second();
  int minute = now.minute();

  // === DAYLIGHT SWEEP ===
  // Move every 15 s until we reach ~180 ° (half turn)
  if ((second % 15 == 0) && (second != lastSecond) && !resetDone &&
      stepsMoved < halfTurnSteps) {
    myStepper.step(direction * stepIncrement);   // = ‑21 each nudge
    stepsMoved += stepIncrement;
    lastSecond = second;
    disableCoils();
  }

  // === SUNSET RESET ===
  // Example: trigger at every odd minute (simulate sunset)
  if ((minute % 2 == 1) && !resetDone) {
    Serial.println("Resetting mirror to East (opposite direction)");

    // One continuous move the opposite way
    myStepper.step(-direction * stepsMoved);     // back home
    disableCoils();

    // House‑keeping for next day
    stepsMoved = 0;
    resetDone  = true;
    lastSecond = -1;
  }

  // Arm the next cycle at the next even minute
  if (minute % 2 == 0) {
    resetDone = false;
  }

  delay(500);
}

// === DISABLE STEPPER COILS (prevents heating) ===
void disableCoils() {
  digitalWrite(8, LOW);
  digitalWrite(9, LOW);
  digitalWrite(10, LOW);
  digitalWrite(11, LOW);
}
