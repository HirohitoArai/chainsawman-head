
#include <Servo.h>

Servo esc;


const byte hallPin = 2; //hallpin
const byte triggerPin = 12;  // signal
const byte detectPin  = 11;  // detect
const byte escPin  = 9; //escpin

const byte potPin  = A0;

const unsigned long thresholdUs = 30000;

const int minPulseUs = 869;
const int maxPulseUs = 2114;

volatile unsigned long previousPulseUs = 0;
volatile unsigned long measuredPeriodUs = 0;

int throttle = -1;
bool autoThrottle = false;

void setThrottle(int value);
void hallPulse();

void setup() {
  pinMode(hallPin, INPUT_PULLUP);
  pinMode(triggerPin, INPUT_PULLUP);
  pinMode(detectPin, INPUT_PULLUP);

  Serial.begin(9600);

  esc.attach(escPin, 800, 2200);
  setThrottle(0);

  // ESCのアーム待ち
  delay(3000);

  attachInterrupt(
    digitalPinToInterrupt(hallPin),
    hallPulse,
    FALLING
  );
}

void loop() {
  unsigned long period;

  noInterrupts();
  period = measuredPeriodUs;
  interrupts();

  int analogValue = analogRead(potPin);

  delay(100);
  // スターター条件を満たしたら50%固定
  if (!autoThrottle &&
      analogValue <= 5 &&
      period != 0 &&
      period <= thresholdUs) {
    autoThrottle = true;
  }

  // 可変抵抗を明確に上げたら50%固定を解除
  if (autoThrottle && analogValue > 10) {
    autoThrottle = false;

    noInterrupts();
    previousPulseUs = 0;
    measuredPeriodUs = 0;
    interrupts();
  }

  if (!isRunAllowed()) {
    setThrottle(0);
  } else if (autoThrottle) {
    setThrottle(40);
  } else {
    setThrottle(map(analogValue, 0, 1023, 0, 100));
  }
}

void setThrottle(int value) {
  value = constrain(value, 0, 100);

  // スロットル値が変わったときだけ更新
  if (value == throttle) {
    return;
  }

  throttle = value;

  int pulseUs =
    map(throttle, 0, 100, minPulseUs, maxPulseUs);

  esc.writeMicroseconds(pulseUs);

  Serial.print("Throttle: ");
  Serial.print(throttle);
  Serial.print("%  Pulse: ");
  Serial.print(pulseUs);
  Serial.println(" us");
}

void hallPulse() {
  unsigned long nowUs = micros();

  if (previousPulseUs != 0) {
    measuredPeriodUs = nowUs - previousPulseUs;
  }

  previousPulseUs = nowUs;
}

bool isRunAllowed() {
  bool connectorAttached = digitalRead(detectPin) == LOW;
  bool triggerPressed     = digitalRead(triggerPin) == LOW;

  // 抜けているなら許可。
  // 刺さっている場合は、押されているときだけ許可。
  return !connectorAttached || triggerPressed;
}


