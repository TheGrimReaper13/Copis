/*
  - ignores blades touching for now but doesn't affect other behaviour, no whip-over yet also doesn't account for contact through the blades
  - as micros loops/flows-over after 70min device needs to be restarted (maybe add signal and something to handle a reset in software when it overflows)

   W_PIN: digital output
   L_PIN: digital input
   C_PIN: didital input 
*/

# define CONTROL_USED
# undef CONTROL_USED

const unsigned long DEPRESS_TIME = 500;     // in us, 0.5ms
const unsigned long LOCKOUT_TIME = 170000;  // in us, 170ms
const long RESET_TIME = 3000; // in ms, 3s

const uint8_t SOUND_SIGNAL_PIN = 9;
const uint8_t HOLD_PIN = 2; // toggle switch, probably better to make this a momentary switch and a software toggle

# define GREEN_WEAPON_PIN 3
# define RED_WEAPON_PIN 4
# define GREEN_LAME_PIN 5
# define RED_LAME_PIN 6
# define GREEN_CONTROL_PIN 7
# define RED_CONTROL_PIN 8
# define GREEN_SIGNAL_PIN 10
# define RED_SIGNAL_PIN 11
# define GREEN_ERROR_PIN 12
# define RED_ERROR_PIN 13

struct Fencer {
  unsigned long depressed_time;
  unsigned long lockout_time;
  bool hit;
  bool error; // signals unnormal change in control circuit

  const uint8_t W_PIN;  // C line
  const uint8_t L_PIN;  // A line
  const uint8_t C_PIN;  // B line
  const uint8_t SIGNAL_PIN;
  const uint8_t ERROR_PIN;
};

Fencer green =  { 0, 0, false, false, GREEN_WEAPON_PIN, GREEN_LAME_PIN, GREEN_CONTROL_PIN, GREEN_SIGNAL_PIN, GREEN_ERROR_PIN };
Fencer red =    { 0, 0, false, false, RED_WEAPON_PIN, RED_LAME_PIN, RED_CONTROL_PIN, RED_SIGNAL_PIN, RED_ERROR_PIN };

void reset(Fencer *p) {
  // W_PIN is always LOW after exiting checkHit
  // turn off signal lights
  digitalWrite(p->SIGNAL_PIN, LOW);

  p->depressed_time = 0;
  p->lockout_time = 0;
  p->hit = false;
  p->error = false;
} // end reset

void reset() {
  delay(RESET_TIME);

  reset(&red);
  reset(&green);
} // end reset

void signalTone() {
  tone(SOUND_SIGNAL_PIN, 880, 2000);
} // end signalTone

// checks if attacker made valid hit
void checkHit(Fencer *att, Fencer *def, unsigned long now) {
  digitalWrite(att->W_PIN, HIGH);
# ifdef CONTROL_USED
  // check control circuit, if we can't read a signal on the control circuit 
  if (digitalRead(att->C_PIN) == LOW) {
    att->error = true;
  }
# else
  if (0) {}
# endif
  // so now if we can read HIGH on red L_PIN there should be contact between greens weapon and reds lame
  else if (digitalRead(def->L_PIN) == HIGH) {
    // start counting when this has been first contact
    if (att->depressed_time == 0) {
        att->depressed_time = now;
    }
    // only register hit if it makes contact for more than 0.5ms 
    else if (!att->hit && (now - att->depressed_time) > DEPRESS_TIME) {
      // now it is a valid hit if inside of lockout period which we handle before we call this functions
      att->hit = true;
      // start lockout timer (this if should be redundant as hit will always be false if lockout is 0 and if hit is true lockout is always not 0)
      if (att->lockout_time == 0) {
        att->lockout_time = now;
      }
    }
  }
  // contact lost so we reset depressed time
  else {
    att->depressed_time = 0;
  }
  digitalWrite(att->W_PIN, LOW);
} // end checkHit

void checkBladesTouching() {
}

void setup() {
  pinMode(green.SIGNAL_PIN, OUTPUT);
  pinMode(red.SIGNAL_PIN, OUTPUT);

  pinMode(green.W_PIN, OUTPUT);
  pinMode(red.W_PIN, OUTPUT);

  // digital pins are input by default
  pinMode(HOLD_PIN, INPUT_PULLUP);
  
  pinMode(green.L_PIN, INPUT);
  pinMode(red.L_PIN, INPUT);

  pinMode(green.C_PIN, INPUT);
  pinMode(red.C_PIN, INPUT);

  digitalWrite(green.W_PIN, LOW);
  digitalWrite(red.W_PIN, LOW);

  digitalWrite(green.SIGNAL_PIN, LOW);
  digitalWrite(red.SIGNAL_PIN, LOW);

  digitalWrite(green.ERROR_PIN, LOW);
  digitalWrite(red.ERROR_PIN, LOW);
}  // end setup

void loop() {

  const unsigned long now = micros();

  // check if hold pin is low as we pulled it up
  if (digitalRead(HOLD_PIN) == LOW) {
    // don't do anything as long as hold toggle is "on" but reset so we can assume operation
    reset();
  }
# ifdef CONTROL_USED
  else if (green.error || red.error) {
    // if we ecountered error we do tone 
    signalTone();
    // signal on the corresponding white lamp
    digitalWrite(green.ERROR_PIN, green.error ? HIGH : LOW);
    digitalWrite(red.ERROR_PIN, red.error ? HIGH : LOW);
    // then again reset everything
    reset();
  }
# endif
  // if at least one player made a hit we can check if lockout time has expired, if so we can signal hits and reset the routine
  else if ((green.hit && (now - green.lockout_time) > LOCKOUT_TIME) || (red.hit && (now - red.lockout_time) > LOCKOUT_TIME)) {
    // sound signal
    signalTone();
    // light signal
    digitalWrite(green.SIGNAL_PIN, green.hit ? HIGH : LOW);
    digitalWrite(red.SIGNAL_PIN, red.hit ? HIGH : LOW);
  
    reset();
  }
  else {
    // check if green made valid hit
    checkHit(&green, &red, now);
    // check if red made valid hit
    checkHit(&red, &green, now);
  }
}  // end loop
