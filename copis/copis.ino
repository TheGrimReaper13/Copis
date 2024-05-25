/*
  - ignores blades touching for now but doesn't affect other behaviour, no whip-over yet also doesn't account for contact through the blades
  - as micros loops/flows-over after 70min device needs to be restarted (maybe add signal and something to handle a reset in software when it overflows)

  W_PIN: digital output
  L_PIN: digital input
  C_PIN: didital input 

  - if we touch ourself signal with yellow led, FIE rulebook seems to imply this lamp is directly linked to that state, so it doesn't stay on (maybe just keep it on until reset?)

*/

const unsigned long   DEPRESS_TIME      = 300;     // in us, 0.5ms
const unsigned long   LOCKOUT_TIME      = 170000;  // in us, 170ms
const unsigned long   ERROR_TIME        = 3000;    // in us, 3ms
const unsigned long   RESET_TIME        = 3000;    // in ms, 3s

const uint8_t         SOUND_SIGNAL_PIN  = 9;       // produces square wave and is connected to speaker 
const uint8_t         HOLD_PIN          = 2;       // toggle switch, probably better to make this a momentary switch and a software toggle

# define GREEN_WEAPON_PIN   3
# define RED_WEAPON_PIN     4
# define GREEN_LAME_PIN     5
# define RED_LAME_PIN       6
# define GREEN_CONTROL_PIN  7
# define RED_CONTROL_PIN    8
# define GREEN_SIGNAL_PIN   10
# define RED_SIGNAL_PIN     11
# define GREEN_ERROR_PIN    12
# define RED_ERROR_PIN      13
# define GREEN_SELF_HIT_PIN A0
# define RED_SELF_HIT_PIN   A1

struct Fencer {
  unsigned long depressed_time; // time when contact was made
  unsigned long lockout_time;   // time when valid hit was made
  unsigned long error_time;     // time when break in control circuit was detected
  bool          hit;            // we need to be able to remember that a hit was made
  bool          error;          // signals break in control circuit

  const uint8_t W_PIN;          // C line
  const uint8_t L_PIN;          // A line
  const uint8_t C_PIN;          // B line
  const uint8_t SIGNAL_PIN;     // red/green
  const uint8_t ERROR_PIN;      // white
  const uint8_t SELF_HIT_PIN;   // yellow
};

Fencer green =  { 0, 0, 0, false, false, GREEN_WEAPON_PIN,  GREEN_LAME_PIN, GREEN_CONTROL_PIN,  GREEN_SIGNAL_PIN,   GREEN_ERROR_PIN,  GREEN_SELF_HIT_PIN };
Fencer red =    { 0, 0, 0, false, false, RED_WEAPON_PIN,    RED_LAME_PIN,   RED_CONTROL_PIN,    RED_SIGNAL_PIN,     RED_ERROR_PIN,    RED_SELF_HIT_PIN };

void reset(Fencer *p) {
  // W_PIN is always LOW after exiting checkHit
  // turn off signal lights
  digitalWrite(p->SIGNAL_PIN, LOW);
  digitalWrite(p->ERROR_PIN, LOW);
  digitalWrite(p->SELF_HIT_PIN, LOW);

  p->depressed_time = 0;
  p->lockout_time = 0;
  p->error_time = 0;
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

  // check for self hit, we don't need to interrupt anything, just signal that we hit ourself (we probably want to make this so it stays on until reset is called)
  if (digitalRead(att->L_PIN) == HIGH) {
    digitalWrite(att->SELF_HIT_PIN, HIGH);
  }

  // check control circuit, if we can't read a signal on the control circuit 
  if (digitalRead(att->C_PIN) == LOW) {
    // start timer
    if (att->error_time == 0) {
      att->error_time = now;
    }
    else if (!att->error && (now - att->error_time) > ERROR_TIME) {
      att->error = true;
    }
  }
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

  if (att->error_time != 0 && digitalRead(att->C_PIN) == HIGH) {
    att->error_time = 0;
  }

  digitalWrite(att->W_PIN, LOW);
} // end checkHit

void checkBladesTouching() {
}

void setup() {
  pinMode(HOLD_PIN, INPUT_PULLUP);

  pinMode(green.L_PIN, INPUT);
  pinMode(red.L_PIN, INPUT);

  pinMode(green.C_PIN, INPUT);
  pinMode(red.C_PIN, INPUT);

  pinMode(green.W_PIN, OUTPUT);
  pinMode(red.W_PIN, OUTPUT);

  pinMode(green.SIGNAL_PIN, OUTPUT);
  pinMode(red.SIGNAL_PIN, OUTPUT);

  pinMode(green.ERROR_PIN, OUTPUT);
  pinMode(red.ERROR_PIN, OUTPUT);

  pinMode(green.SELF_HIT_PIN, OUTPUT);
  pinMode(red.SELF_HIT_PIN, OUTPUT);

  digitalWrite(green.W_PIN, LOW);
  digitalWrite(red.W_PIN, LOW);

  digitalWrite(green.SIGNAL_PIN, LOW);
  digitalWrite(red.SIGNAL_PIN, LOW);

  digitalWrite(green.ERROR_PIN, LOW);
  digitalWrite(red.ERROR_PIN, LOW);

  digitalWrite(green.SELF_HIT_PIN, LOW);
  digitalWrite(red.SELF_HIT_PIN, LOW);
}  // end setup

void loop() {

  const unsigned long now = micros();

  // check if hold pin is low as we pulled it up
  if (digitalRead(HOLD_PIN) == LOW) {
    // don't do anything as long as hold toggle is "on" but reset so we can assume operation normally
    reset();
  }
  else if (green.error || red.error) {
    // if we ecountered error we do tone 
    signalTone();
    // signal on the corresponding white lamp
    digitalWrite(green.ERROR_PIN, green.error ? HIGH : LOW);
    digitalWrite(red.ERROR_PIN, red.error ? HIGH : LOW);
    // then again reset everything
    reset();
  }
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
