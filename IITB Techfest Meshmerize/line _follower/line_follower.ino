const uint8_t FAR_LEFT_SENSOR  = A0;
const uint8_t LEFT_SENSOR      = A4;
const uint8_t CENTER_SENSOR    = A1;
const uint8_t RIGHT_SENSOR     = A2;
const uint8_t FAR_RIGHT_SENSOR = A3;

const uint8_t PAUSE_BUTTON = 2;
const uint8_t MODE_SWITCH  = 4;

const uint8_t LEFT_ENABLE  = 5;
const uint8_t RIGHT_ENABLE = 6;

const uint8_t LEFT_IN1  = 7;
const uint8_t LEFT_IN2  = 8;
const uint8_t RIGHT_IN1 = 9;
const uint8_t RIGHT_IN2 = 10;

const uint8_t COMPLETE_LED = 11;

const uint8_t NUM_SENSORS = 5;

#define WHITE_IS_HIGH 1
const int THRESHOLD   = 500;

const int BASE_SPEED  = 150;
const int TURN_SPEED  = 140;
const float KP = 25.0;
const float KI = 0.02;
const float KD = 40.0;
const float INTEGRAL_LIMIT = 200.0;

const int INCH_MS       = 60;
const int STRAIGHT_MS   = 80;
const int TURN_CLEAR_MS = 150;
const int UTURN_CLEAR_MS = 300;

const uint8_t sensorPins[NUM_SENSORS] = {FAR_LEFT_SENSOR, LEFT_SENSOR, CENTER_SENSOR,
                                         RIGHT_SENSOR, FAR_RIGHT_SENSOR};
bool s[NUM_SENSORS];

char path[60];
int  pathLen = 0;
int  pathIdx = 0;
bool mapReady = false;
bool isDry   = true;
float lastError = 0;
float integral = 0;

void setMotor(uint8_t en, uint8_t in1, uint8_t in2, int speed) {
  speed = constrain(speed, -255, 255);
  digitalWrite(in1, speed > 0);
  digitalWrite(in2, speed < 0);
  analogWrite(en, abs(speed));
}

void setMotors(int l, int r) {
  setMotor(LEFT_ENABLE,  LEFT_IN1,  LEFT_IN2,  l);
  setMotor(RIGHT_ENABLE, RIGHT_IN1, RIGHT_IN2, r);
}

void stopMotors() { setMotors(0, 0); }

void forward(int ms) { setMotors(BASE_SPEED, BASE_SPEED); delay(ms); }

void readSensors() {
  for (uint8_t i = 0; i < NUM_SENSORS; i++) {
    bool high = analogRead(sensorPins[i]) > THRESHOLD;
    s[i] = WHITE_IS_HIGH ? high : !high;
  }
}

bool allWhite() { return s[0] && s[1] && s[2] && s[3] && s[4]; }
bool allBlack() { return !(s[0] || s[1] || s[2] || s[3] || s[4]); }

void blinkDone() {
  for (uint8_t i = 0; i < 5; i++) {
    digitalWrite(COMPLETE_LED, HIGH); delay(200);
    digitalWrite(COMPLETE_LED, LOW);  delay(200);
  }
}

void waitForPress() {
  while (digitalRead(PAUSE_BUTTON) == HIGH) {}
  delay(50);
  while (digitalRead(PAUSE_BUTTON) == LOW) {}
  delay(50);
}

void checkPause() {
  if (digitalRead(PAUSE_BUTTON) == LOW) {
    stopMotors();
    delay(50);
    while (digitalRead(PAUSE_BUTTON) == LOW) {}
    delay(50);
    waitForPress();
  }
}

void simplify() {
  bool changed = true;
  while (changed) {
    changed = false;
    for (int i = 1; i < pathLen - 1; i++) {
      if (path[i] != 'B') continue;
      char a = path[i - 1], c = path[i + 1], r;
      if      (a == 'L' && c == 'R') r = 'B';
      else if (a == 'L' && c == 'S') r = 'R';
      else if (a == 'R' && c == 'L') r = 'B';
      else if (a == 'S' && c == 'L') r = 'R';
      else if (a == 'S' && c == 'S') r = 'B';
      else if (a == 'L' && c == 'L') r = 'S';
      else continue;
      path[i - 1] = r;
      for (int j = i; j < pathLen - 2; j++) path[j] = path[j + 2];
      pathLen -= 2;
      changed = true;
      break;
    }
  }
}

void record(char c) {
  if (isDry && pathLen < (int)sizeof(path)) path[pathLen++] = c;
}

void turn(int dir, int clearMs) {
  setMotors(dir * TURN_SPEED, -dir * TURN_SPEED);
  delay(clearMs);
  do { readSensors(); } while (!s[2]);
  stopMotors();
  delay(30);
  lastError = 0;
  integral = 0;
}

bool handleJunction(bool left, bool right) {
  forward(INCH_MS);
  readSensors();
  if (allWhite()) { stopMotors(); return true; }

  bool straight = s[1] || s[2] || s[3];
  bool choice = straight ? (left || right) : (left && right);

  char act = left ? 'L' : (straight ? 'S' : 'R');

  if (!isDry && choice && pathIdx < pathLen) act = path[pathIdx++];

  if (isDry && choice) record(act);

  if (act == 'L')      turn(-1, TURN_CLEAR_MS);
  else if (act == 'R') turn(+1, TURN_CLEAR_MS);
  else {
    forward(STRAIGHT_MS);
    lastError = 0;
    integral = 0;
  }
  return false;
}

void runMode() {
  lastError = 0;
  integral = 0;
  pathIdx = 0;
  if (isDry) pathLen = 0;

  while (true) {
    checkPause();
    readSensors();

    if (allBlack()) {
      stopMotors();
      delay(30);
      record('B');
      turn(-1, UTURN_CLEAR_MS);
      continue;
    }

    bool left  = s[0];
    bool right = s[4];
    if (left || right) {
      if (handleJunction(left, right)) break;
      continue;
    }

    float error = (s[3] - s[1]);
    if (error == 0) integral = 0;
    else integral = constrain(integral + error, -INTEGRAL_LIMIT, INTEGRAL_LIMIT);
    float corr = KP * error + KI * integral + KD * (error - lastError);
    lastError = error;
    setMotors(BASE_SPEED + corr, BASE_SPEED - corr);
  }

  stopMotors();
  if (isDry) { simplify(); mapReady = true; }
  blinkDone();
}

void setup() {
  pinMode(PAUSE_BUTTON, INPUT_PULLUP);
  pinMode(MODE_SWITCH, INPUT_PULLUP);
  pinMode(COMPLETE_LED, OUTPUT);
  pinMode(LEFT_ENABLE, OUTPUT);  pinMode(RIGHT_ENABLE, OUTPUT);
  pinMode(LEFT_IN1, OUTPUT);     pinMode(LEFT_IN2, OUTPUT);
  pinMode(RIGHT_IN1, OUTPUT);    pinMode(RIGHT_IN2, OUTPUT);
  stopMotors();
}

void loop() {
  waitForPress();
  isDry = (digitalRead(MODE_SWITCH) == HIGH);
  delay(500);

  if (!isDry && !mapReady) {
    for (uint8_t i = 0; i < 10; i++) {
      digitalWrite(COMPLETE_LED, !digitalRead(COMPLETE_LED));
      delay(80);
    }
    digitalWrite(COMPLETE_LED, LOW);
    return;
  }
  runMode();
}
