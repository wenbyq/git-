const uint8_t PIN_RED    = 13;
const uint8_t PIN_YELLOW = 12;
const uint8_t PIN_GREEN  = 11;
const uint8_t PIN_BTN_PED  = 2;   // INPUT_PULLUP
const uint8_t PIN_BTN_EMER = 3;   // INPUT_PULLUP

// ----- Времена -----
const unsigned long T_GREEN        = 10000;  // 10 c
const unsigned long T_YELLOW       = 3000;   // 3 c
const unsigned long T_RED          = 10000;  // 10 c обычный красный
const unsigned long T_RED_PED      = 15000;  // 15 c красный после пешехода (10 + 5)
const unsigned long T_WARNING_HALF = 500;    // полупериод мигания жёлтого

const unsigned long DEBOUNCE_MS   = 80;      // окно стабильности кнопки
const unsigned long MIN_EVENT_GAP = 250;     // мёртвое время между нажатиями

enum State { S_GREEN, S_YELLOW, S_RED, S_WARNING };
State currentState = S_GREEN;

unsigned long stateStart = 0;
unsigned long duration   = 0;
bool pedRequest = false;
bool emergency  = false;

unsigned long lastBlinkMillis = 0;
bool          blinkOn         = false;

struct Button {
  uint8_t       pin;
  bool          stable;
  bool          lastRaw;
  unsigned long lastChangeMs;
  unsigned long lastEventMs;
};

Button btnPed  = { PIN_BTN_PED,  false, false, 0, 0 };
Button btnEmer = { PIN_BTN_EMER, false, false, 0, 0 };

bool buttonPressed(Button &b) {
  bool raw = (digitalRead(b.pin) == LOW); // LOW = нажата (INPUT_PULLUP)
  unsigned long now = millis();

  if (raw != b.lastRaw) {
    b.lastRaw = raw;
    b.lastChangeMs = now;
  }

  if ((now - b.lastChangeMs) >= DEBOUNCE_MS && raw != b.stable) {
    b.stable = raw;
    if (b.stable) {
      if (now - b.lastEventMs < MIN_EVENT_GAP) return false;
      b.lastEventMs = now;
      return true;
    }
  }
  return false;
}

void setOutputsForState(State s) {
  digitalWrite(PIN_RED,    LOW);
  digitalWrite(PIN_YELLOW, LOW);
  digitalWrite(PIN_GREEN,  LOW);

  switch (s) {
    case S_GREEN:   digitalWrite(PIN_GREEN,  HIGH); break;
    case S_YELLOW:  digitalWrite(PIN_YELLOW, HIGH); break;
    case S_RED:     digitalWrite(PIN_RED,    HIGH); break;
    case S_WARNING: break; // мигание управляется в loop()
  }
}

void goToState(State s, unsigned long dur) {
  currentState = s;
  stateStart   = millis();
  duration     = dur;
  setOutputsForState(s);

  if (s == S_WARNING) {
    lastBlinkMillis = millis();
    blinkOn = false;
    digitalWrite(PIN_YELLOW, LOW);
  }

  Serial.print(F("[FSM] -> "));
  switch (s) {
    case S_GREEN:   Serial.print(F("S_GREEN"));   break;
    case S_YELLOW:  Serial.print(F("S_YELLOW"));  break;
    case S_RED:     Serial.print(F("S_RED"));     break;
    case S_WARNING: Serial.print(F("S_WARNING")); break;
  }
  Serial.print(F(", dur="));
  Serial.print(dur);
  Serial.print(F(", t="));
  Serial.println(millis());
}

void readInputs() {
  if (buttonPressed(btnPed)) {
    pedRequest = true;
    Serial.println(F("[EVT] E_PED_REQ"));
  }
  if (buttonPressed(btnEmer)) {
    emergency = !emergency;
    Serial.print(F("[EVT] E_EMERGENCY "));
    Serial.println(emergency ? F("ON") : F("OFF"));
  }
}

void setup() {
  pinMode(PIN_RED,    OUTPUT);
  pinMode(PIN_YELLOW, OUTPUT);
  pinMode(PIN_GREEN,  OUTPUT);

  pinMode(PIN_BTN_PED,  INPUT_PULLUP);
  pinMode(PIN_BTN_EMER, INPUT_PULLUP);

  Serial.begin(9600);
  Serial.println(F("FSM traffic light start"));

  goToState(S_GREEN, T_GREEN);
}

void loop() {
  readInputs();

  if (emergency) {
    if (currentState != S_WARNING) {
      goToState(S_WARNING, T_WARNING_HALF);
    }
    unsigned long now = millis();
    if (now - lastBlinkMillis >= T_WARNING_HALF) {
      lastBlinkMillis = now;
      blinkOn = !blinkOn;
      digitalWrite(PIN_YELLOW, blinkOn ? HIGH : LOW);
    }
    return;
  } else if (currentState == S_WARNING) {
    digitalWrite(PIN_YELLOW, LOW);
    goToState(S_GREEN, T_GREEN);
  }

  //Основной цикл
  switch (currentState) {

    case S_GREEN:
      if (millis() - stateStart >= duration) {
        goToState(S_YELLOW, T_YELLOW);
      }
      break;

    case S_YELLOW:
      if (millis() - stateStart >= duration) {
        if (pedRequest) {
          pedRequest = false;
          goToState(S_RED, T_RED_PED);   // 15 c — с пешеходом
        } else {
          goToState(S_RED, T_RED);       // 10 c — обычный
        }
      }
      break;

    case S_RED:
      if (millis() - stateStart >= duration) {
        goToState(S_GREEN, T_GREEN);
      }
      break;

    case S_WARNING:
      break;
  }
}
