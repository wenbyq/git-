enum State { S_GREEN, S_YELLOW, S_RED, S_WARNING, NUM_S };
enum Event { E_NONE, E_TIMER, E_PED, E_NIGHT, NUM_E };

State st = S_GREEN;
unsigned long tStart = 0, dur = 0;
bool pedReq = false, night = false;
bool blinkOn = false;
unsigned long lastBlink = 0;

typedef void (*H)();
H table[NUM_S][NUM_E];

void setOut(State s) {
  digitalWrite(13, s == S_RED);
  digitalWrite(12, s == S_YELLOW);
  digitalWrite(11, s == S_GREEN);
}

void goTo(State s, unsigned long d) {
  st = s;
  tStart = millis();
  dur = d;
  setOut(s);

  if (s == S_WARNING) {
    blinkOn = false;
    lastBlink = millis();
  }

  Serial.print(millis());
  Serial.print(" -> ");
  Serial.println(s);
}

void hGreenT() {
  goTo(S_YELLOW, 3000);
}

void hYellowT() {
  if (pedReq) {
    pedReq = false;
    goTo(S_RED, 15000);
  } else {
    goTo(S_RED, 10000);
  }
}

void hRedT() {
  goTo(S_GREEN, 10000);
}

void hPed() {
  pedReq = true;
  Serial.println("PED REQ");
}

void hNight() {
  night = !night;
  Serial.print("NIGHT = ");
  Serial.println(night);

  if (night) {
    goTo(S_WARNING, 500);
  } else {
    goTo(S_GREEN, 10000);
  }
}

void setupTable() {
  for (int s = 0; s < NUM_S; s++) {
    for (int e = 0; e < NUM_E; e++) {
      table[s][e] = 0;
    }
  }

  table[S_GREEN][E_TIMER]  = hGreenT;
  table[S_GREEN][E_PED]    = hPed;
  table[S_YELLOW][E_TIMER] = hYellowT;
  table[S_RED][E_TIMER]    = hRedT;

  for (int s = 0; s < NUM_S; s++) {
    table[s][E_NIGHT] = hNight;
  }
}

Event pollEvent() {
  static bool btnStable = false, btnRaw = false;
  static unsigned long btnRawChange = 0;

  bool raw = !digitalRead(2);

  if (raw != btnRaw) {
    btnRaw = raw;
    btnRawChange = millis();
  }

  if (millis() - btnRawChange > 50 && btnRaw != btnStable) {
    btnStable = btnRaw;

    if (btnStable) {
      return E_PED;
    }
  }

  static bool nightStable = false, nightRaw = false;
  static unsigned long nightRawChange = 0;

  bool rawN = !digitalRead(3);

  if (rawN != nightRaw) {
    nightRaw = rawN;
    nightRawChange = millis();
  }

  if (millis() - nightRawChange > 50 && nightRaw != nightStable) {
    nightStable = nightRaw;

    if (nightStable) {
      return E_NIGHT;
    }
  }

  if (st != S_WARNING && millis() - tStart >= dur) {
    return E_TIMER;
  }

  return E_NONE;
}

void setup() {
  Serial.begin(9600);

  pinMode(13, OUTPUT);
  pinMode(12, OUTPUT);
  pinMode(11, OUTPUT);
  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);

  setupTable();
  goTo(S_GREEN, 10000);
}

void loop() {
  if (st == S_WARNING && millis() - lastBlink >= 500) {
    lastBlink = millis();
    blinkOn = !blinkOn;

    digitalWrite(13, 0);
    digitalWrite(12, blinkOn);
    digitalWrite(11, 0);
  }

  Event e = pollEvent();

  if (e != E_NONE && table[st][e]) {
    table[st][e]();
  }
}
