// ================= PINS =================
#define m1 7   // Right Motor MA1
#define m2 8   // Right Motor MA2
#define m3 10  // Left Motor MB1
#define m4 9   // Left Motor MB2
#define e1 5   // Right Motor Enable EA
#define e2 6   // Left Motor Enable EB

#define ir1 A0  // Left most
#define ir2 A1  // Left
#define ir3 A2  // Middle
#define ir4 A3  // Right
#define ir5 A4  // Right most

#define switch1 2
#define switch2 4

#define red_led 11  // lights up when the bot stops in the white box

// ================= TUNING =================
const bool SWITCH_ACTIVE_LOW = true;  // true: switch wired to GND (internal pull-up)
                                      // false: switch wired to 5V with external pull-down
const bool PREFER_LEFT = true;        // when only turns exist (no straight), which side first

const int  BASE_SPEED  = 170;         // normal line-following speed (0-255)
const int  KP          = 90;          // steering strength
const int  TURN_SPEED  = 170;         // pivot speed at corners/junctions
const int  CREEP_SPEED = 150;         // speed while probing a junction

const unsigned long CREEP_MS         = 120;   // drive forward over a junction to see if straight exists
const unsigned long JUNCTION_COOL_MS = 300;   // ignore new junctions right after handling one
const unsigned long WINDOW_MS        = 4000;  // after a junction, how long all-white counts as the white box
const unsigned long WHITE_CONFIRM_MS = 250;   // all-white must last this long to confirm the box
const unsigned long LOST_TIMEOUT_MS  = 3000;  // max time to search for a lost line

// ================= STATE =================
bool running = false;
bool prevA = false, prevB = false;

bool timerActive = false;
unsigned long timerStart = 0;
unsigned long whiteSince = 0;
unsigned long lastJunctionTime = 0;
int  lastDir = 0;                 // -1 line was last on left, +1 right

// ================= HELPERS =================
bool sw(int pin) {
  return digitalRead(pin) == (SWITCH_ACTIVE_LOW ? LOW : HIGH);
}

void setMotor(int in1, int in2, int en, int spd) {
  spd = constrain(spd, -255, 255);
  digitalWrite(in1, spd > 0);
  digitalWrite(in2, spd < 0);
  analogWrite(en, abs(spd));
}

void drive(int rightSpd, int leftSpd) {
  setMotor(m1, m2, e1, rightSpd);
  setMotor(m3, m4, e2, leftSpd);
}

void stopMotors() {
  drive(0, 0);
}

// dir: -1 = pivot left, +1 = pivot right, until middle sensor finds the line
void pivotUntilLine(int dir, unsigned long timeoutMs) {
  if (dir < 0) drive(TURN_SPEED, -TURN_SPEED);
  else         drive(-TURN_SPEED, TURN_SPEED);
  delay(80);  // get off whatever the sensors are currently over
  unsigned long t = millis();
  while (millis() - t < timeoutMs) {
    if (digitalRead(ir3)) break;
  }
  stopMotors();
  lastDir = dir;
}

void startRun() {
  digitalWrite(red_led, LOW);
  timerActive = false;
  whiteSince = 0;
  lastDir = 0;
  lastJunctionTime = 0;
  running = true;
}

void endRun(bool reachedBox) {
  stopMotors();
  running = false;
  if (reachedBox) {
    digitalWrite(red_led, HIGH);
  }
  // so a switch that is still ON does not retrigger a run
  prevA = sw(switch1);
  prevB = sw(switch2);
}

// ================= JUNCTIONS =================
void handleJunction(bool left, bool right) {
  // creep forward across the junction, latching side sensors
  drive(CREEP_SPEED, CREEP_SPEED);
  unsigned long t = millis();
  while (millis() - t < CREEP_MS) {
    if (digitalRead(ir1)) left = true;
    if (digitalRead(ir5)) right = true;
  }
  stopMotors();

  bool straight = digitalRead(ir2) || digitalRead(ir3) || digitalRead(ir4);

  // 3-way (straight+left+right), 3-way (straight+one side), 2-way (left+right)
  bool isBranch = (left && right) || (straight && (left || right));
  if (isBranch) {
    timerActive = true;          // start timer to look for the white box
    timerStart = millis();
    whiteSince = 0;
  }

  lastJunctionTime = millis();

  if (straight) {                // priority 1: go straight
    lastDir = 0;
    return;
  }

  int dir = 0;                   // priority 2: turn
  if (left && right) dir = PREFER_LEFT ? -1 : 1;
  else if (left)     dir = -1;
  else if (right)    dir = 1;

  if (dir != 0) pivotUntilLine(dir, 2000);
  // dir == 0: nothing found ahead -> main loop handles it (white box / lost line)
}

// ================= MAIN STEP =================
void runStep() {
  int s1 = digitalRead(ir1);
  int s2 = digitalRead(ir2);
  int s3 = digitalRead(ir3);
  int s4 = digitalRead(ir4);
  int s5 = digitalRead(ir5);
  unsigned long now = millis();

  if (timerActive && (now - timerStart > WINDOW_MS)) timerActive = false;

  bool allWhite = !(s1 || s2 || s3 || s4 || s5);

  if (allWhite) {
    if (timerActive) {
      // possible white box: keep rolling forward and confirm
      if (whiteSince == 0) whiteSince = now;
      if (now - whiteSince >= WHITE_CONFIRM_MS) {
        endRun(true);            // reached the endpoint -> LED on, stop
        return;
      }
      drive(CREEP_SPEED, CREEP_SPEED);
      return;
    }
    // line lost (dead end / gap): spin toward where the line was last seen
    int dir = (lastDir == 0) ? -1 : lastDir;
    pivotUntilLine(dir, LOST_TIMEOUT_MS);
    if (!digitalRead(ir3)) endRun(false);   // failsafe: could not find line, stop
    return;
  }
  whiteSince = 0;

  // junction / wide-line detection
  bool junction = (s1 && s5) || (s3 && (s1 || s5));
  if (junction && (now - lastJunctionTime > JUNCTION_COOL_MS)) {
    handleJunction(s1, s5);
    return;
  }

  // normal line following (weighted steering)
  int count = s1 + s2 + s3 + s4 + s5;
  float error = (-2 * s1 - 1 * s2 + 1 * s4 + 2 * s5) / (float)count;
  int corr = (int)(error * KP);

  // error < 0: line on left -> right motor faster, left slower
  drive(BASE_SPEED - corr, BASE_SPEED + corr);

  if (s1 || s2) lastDir = -1;
  else if (s4 || s5) lastDir = 1;
}

// ================= ARDUINO =================
void setup() {
  pinMode(m1, OUTPUT); pinMode(m2, OUTPUT);
  pinMode(m3, OUTPUT); pinMode(m4, OUTPUT);
  pinMode(e1, OUTPUT); pinMode(e2, OUTPUT);

  pinMode(ir1, INPUT); pinMode(ir2, INPUT); pinMode(ir3, INPUT);
  pinMode(ir4, INPUT); pinMode(ir5, INPUT);

  pinMode(switch1, SWITCH_ACTIVE_LOW ? INPUT_PULLUP : INPUT);
  pinMode(switch2, SWITCH_ACTIVE_LOW ? INPUT_PULLUP : INPUT);

  pinMode(red_led, OUTPUT);
  digitalWrite(red_led, LOW);

  stopMotors();
}

void loop() {
  if (running) {
    runStep();
    return;
  }

  // idle: wait for a switch to be turned ON (off -> on edge)
  bool a = sw(switch1);
  bool b = sw(switch2);
  if ((a && !prevA) || (b && !prevB)) {
    prevA = a;
    prevB = b;
    startRun();
    return;
  }
  prevA = a;
  prevB = b;
  delay(20);  // simple debounce
}