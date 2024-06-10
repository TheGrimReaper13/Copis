/*

  COPIS - 0.5 - soldered prototype #1
  A simple DIY wired scoring box for saber fencing based on Arduino

  Function not marked with "time-critical" are called after a hit was made and lockout time has passed or are called before en-garde call by ref.

  TODO: We'll need to do a lot more testing how much frequency/program speed will affect behaviour. So higher speed will probably lead to more crosstalk which already seems to be a small issue.
        
*/

typedef unsigned long time_t; // micros() returns an unsigned long value

struct Fencer {
  time_t        depressed_time; // time when contact was made
  time_t        lockout_time;   // time when valid hit was made
  time_t        cbreak_time;    // time when break in control circuit was detected
  time_t        parry_time;     // time when a both blades made contact
  time_t        self_hit_time;  // remember time when we hit ourself so we can keep the light on for SELF_HIT_TIME
  time_t        sh_signal_time; // time when self hit signal light was turned on

  unsigned int  bounce_counter; // prevent whip over protection if contact between blades was lost more than 10 times

  bool          hit;            // valid hit was made
  bool          cbreak;         // break in control circuit detected
  bool          whip_over;      // whipover protection active
  bool          self_hit;       // contact with lame and weapon of the same fencer

  const uint8_t W_PIN;          // C line, weapon
  const uint8_t L_PIN;          // A line, lame
  const uint8_t C_PIN;          // B line, weapon
  const uint8_t SIGNAL_PIN;     // red/green lamp
  const uint8_t CBREAK_PIN;     // white lamp
  const uint8_t SELF_HIT_PIN;   // yellow lamp
};

const time_t          DEPRESS_TIME      = 600;    // in us, 0.6ms (0.1 - 1ms), FIE, we could make this controllable with a trimmer or poti to find a reliable value
const time_t          LOCKOUT_TIME      = 170000; // in us, 170ms, FIE
const time_t          CBREAK_TIME       = 3000;   // in us, 3ms
const time_t          SELF_HIT_TIME     = 500000; // in us, 0.5s, FIE
const time_t          MIN_PARRY_TIME    = 4000;   // in us, 4ms, FIE
const time_t          MAX_PARRY_TIME    = 15000;  // in us, 15ms, FIE

const unsigned int    MAX_BOUNCES       = 10;     // FIE, max amount of times contact can be lost during parry while still protecting against whipover

const time_t          RESET_TIME        = 3000;   // in ms, 3s
const time_t          SOUND_LENGTH      = 2000;   // in ms, 2s, FIE
const unsigned int    SOUND_FREQUENCY   = 3000;   // in Hz

const uint8_t         HOLD_PIN          = A1;     // toggle switch, probably better to make this a momentary switch and a software toggle, could also just be replaced with a power switch
const uint8_t         SOUND_SIGNAL_PIN  = A2;     // produces square wave and is connected to speaker
//const uint8_t         SYSTEM_ERROR_PIN  = 13;     // on-board LED, use it if we need to signal system errors or such

// use macros here to make struct initialization a bit more readible as Arduino IDE doesn't allow the use of specified initializers for members :(
# define GREEN_WEAPON_PIN   2
# define RED_WEAPON_PIN     3
# define GREEN_LAME_PIN     4
# define RED_LAME_PIN       5
# define GREEN_CONTROL_PIN  6
# define RED_CONTROL_PIN    7
# define GREEN_SIGNAL_PIN   8
# define RED_SIGNAL_PIN     9
# define GREEN_CBREAK_PIN   10
# define RED_CBREAK_PIN     11  
# define GREEN_SELF_HIT_PIN 12  
# define RED_SELF_HIT_PIN   A0  // don't use 13 here as it's linked to on board LED, see SYSTEM_ERROR_PIN

void signalTone() {
  tone(SOUND_SIGNAL_PIN, SOUND_FREQUENCY, SOUND_LENGTH);
} // end signalTone

// use these 2 mainly for prototype when we don't have easily visible LEDs yet, use signalTone for simultanious hit, not used anymore at this stage (we should remove these after next iteration or move them to different branch)
void signalToneGreen() {
  // slow beeping
  for (int i = 0 ; i < 3 ; i++) {
    tone(SOUND_SIGNAL_PIN, SOUND_FREQUENCY, 250);
    delay(500);
  }
  tone(SOUND_SIGNAL_PIN, SOUND_FREQUENCY, 250);
} // end signalToneGreen

void signalToneRed() {
  // fast beeping
  for (int i = 0 ; i < 7 ; i++) {
    tone(SOUND_SIGNAL_PIN, SOUND_FREQUENCY, 124);
    delay(250);
  }
  tone(SOUND_SIGNAL_PIN, SOUND_FREQUENCY, 124);
} // end signalToneRed

// as this expression is a bit more complicated we move it into it's own function to keep loop() clean and easy to understand
void signalSelfHit(Fencer *p, const time_t now) { // time-critical
  if (p->self_hit) {
      // if we hit ourselves and we haven't turned the LED on yet turn it on and start sh_signal timer
    if (p->sh_signal_time == 0) {
      p->sh_signal_time = now;
      digitalWrite(p->SELF_HIT_PIN, HIGH);
    }
  }
  // if we have turned the LED on but the contact between own lame and weapon was lost we leave the LED on for SELF_HIT_TIME
  else if (p->sh_signal_time != 0 && (now - p->sh_signal_time) > SELF_HIT_TIME) {
    p->sh_signal_time = 0;
    digitalWrite(p->SELF_HIT_PIN, LOW); 
  }
} // end signalSelfHit

void reset(Fencer *p) {
  // W_PIN should always be LOW after exiting checkHit, but set it low just to be sure
  digitalWrite(p->W_PIN,        LOW);
  // turn off signal lights
  digitalWrite(p->SIGNAL_PIN,   LOW);
  digitalWrite(p->CBREAK_PIN,   LOW);
  digitalWrite(p->SELF_HIT_PIN, LOW);

  p->depressed_time = 0;
  p->lockout_time   = 0;
  p->cbreak_time    = 0;
  p->parry_time     = 0;
  p->self_hit_time  = 0;
  p->sh_signal_time = 0;

  p->bounce_counter = 0;

  p->hit            = false;
  p->cbreak         = false;
  p->whip_over      = false;
  p->self_hit       = false;
} // end reset

void resetAndWait(Fencer *p, Fencer *q) {
  // first delay so we don't turn off signal lights
  delay(RESET_TIME);

  reset(p);
  reset(q);
} // end reset

// checks if attacker made valid hit
void checkHit(Fencer *att, Fencer *def, const time_t now) {  // time-critical
  // make "attackers" weapon hot so we can read a current at both fencers lames and the "defenders" weapon to detect hits and blade contact
  digitalWrite(att->W_PIN, HIGH);

  // check for self hit, we don't need to interrupt anything, just signal briefly with light
  if (digitalRead(att->L_PIN) == HIGH) {
    if (att->self_hit_time == 0) {
      att->self_hit_time = now;
    }
    else if (!att->self_hit && (now - att->self_hit_time) > DEPRESS_TIME) {
      att->self_hit = true;
    }
  }
  // we turn on the LED outside this function so we don't need to worry about keeping it lit here
  else {
    att->self_hit_time = 0;
    att->self_hit = false;
  }
  
  // handle whip-over protection
  const time_t parry_diff = now - att->parry_time;  // save diff as constant as we need it several times

  // check if blades made contact
  if (digitalRead(def->C_PIN) == HIGH) {
    // start timer first time we made contact
    if (att->parry_time == 0) {
      att->parry_time = now; 
    }
  }
  // if there is no contact between the blades and parry_time is "running" we advance the bounce counter
  else if (att->parry_time != 0) {
    att->bounce_counter++;
  }

  // reset if no hit was made after whip_over time has passed or contact between blades was interrupted more than 10 times
  if (parry_diff > MAX_PARRY_TIME || att->bounce_counter > MAX_BOUNCES) {
    att->parry_time = 0;
    att->bounce_counter = 0;
    att->whip_over = false;
  }
  // prevent hits from registering while in the parameters for whipover protection
  else if (parry_diff >= MIN_PARRY_TIME && parry_diff <= MAX_PARRY_TIME && att->bounce_counter <= MAX_BOUNCES) {
    att->whip_over = true;
  }

  // reset cbreak time if we can detect a signal again
  if (digitalRead(att->C_PIN) == HIGH) {
    att->cbreak_time = 0;
  }

  // handle break in control circuit, if we can't read a signal on the control circuit for CBREAK_TIME we set cbreak flag
  if (digitalRead(att->C_PIN) == LOW) {
    // start timer
    if (att->cbreak_time == 0) {
      att->cbreak_time = now;
    }
    else if ((now - att->cbreak_time) > CBREAK_TIME) {
      att->cbreak = true;
    }
  }
  // handle actual hit detection
  // only make reading if whip_over protection is not active
  // if we can read HIGH on def L_PIN the att seemed to have done something right because they actually hit
  else if (!att->whip_over && digitalRead(def->L_PIN) == HIGH) {
    // start counting when this has been first contact
    if (att->depressed_time == 0) {
        att->depressed_time = now;
    }
    // only register hit if it makes contact for more than 0.5ms and whip_over protection is inactive
    else if (!att->hit && (now - att->depressed_time) > DEPRESS_TIME) {
      // now it is a valid hit if inside of lockout period which we handle before we call this functions
      att->hit = true;
      // start lockout timer
      att->lockout_time = now;
    }
  }
  // contact lost so we reset depressed time, this will also be called if whip_over is active
  else {
    att->depressed_time = 0;
  }
  // make "attackers" weapon cold to not trigger valid self hits while doing the same function with the current "defender" as the "attacker"
  digitalWrite(att->W_PIN, LOW);
} // end checkHit

void setup() {
  //pinMode(SYSTEM_ERROR_PIN,   OUTPUT);
  pinMode(HOLD_PIN,           INPUT_PULLUP);

  // BODY WIRE LINES
  pinMode(GREEN_LAME_PIN,     INPUT);      pinMode(RED_LAME_PIN,      INPUT);
  pinMode(GREEN_CONTROL_PIN,  INPUT);      pinMode(RED_CONTROL_PIN,   INPUT);
  pinMode(GREEN_WEAPON_PIN,   OUTPUT);     pinMode(RED_WEAPON_PIN,    OUTPUT);
  
  // SIGNAL SPEAKER AND LEDS
  pinMode(SOUND_SIGNAL_PIN,   OUTPUT);
  pinMode(GREEN_SIGNAL_PIN,   OUTPUT);     pinMode(RED_SIGNAL_PIN,    OUTPUT);
  pinMode(GREEN_CBREAK_PIN,   OUTPUT);     pinMode(RED_CBREAK_PIN,    OUTPUT);
  pinMode(GREEN_SELF_HIT_PIN, OUTPUT);     pinMode(RED_SELF_HIT_PIN,  OUTPUT);

  //digitalWrite(SYSTEM_ERROR_PIN,    LOW);
  digitalWrite(SOUND_SIGNAL_PIN,    LOW);
  digitalWrite(GREEN_WEAPON_PIN,    LOW);  digitalWrite(RED_WEAPON_PIN,   LOW);
  digitalWrite(GREEN_SIGNAL_PIN,    LOW);  digitalWrite(RED_SIGNAL_PIN,   LOW);
  digitalWrite(GREEN_CBREAK_PIN,    LOW);  digitalWrite(RED_CBREAK_PIN,   LOW);
  digitalWrite(GREEN_SELF_HIT_PIN,  LOW);  digitalWrite(RED_SELF_HIT_PIN, LOW);
}  // end setup

void loop() {  // time-critical

  static Fencer green = { 
    0, 0, 0, 0, 0, 0,
    false, false, false, false,
    GREEN_WEAPON_PIN, GREEN_LAME_PIN,   GREEN_CONTROL_PIN,
    GREEN_SIGNAL_PIN, GREEN_CBREAK_PIN,  GREEN_SELF_HIT_PIN
  };

  static Fencer red = { 
    0, 0, 0, 0, 0, 0,
    false, false, false, false,
    RED_WEAPON_PIN,   RED_LAME_PIN,     RED_CONTROL_PIN,
    RED_SIGNAL_PIN,   RED_CBREAK_PIN,    RED_SELF_HIT_PIN
  };

  const time_t now = micros();

  // need to check for LOW as we are pulling pin up
  if (digitalRead(HOLD_PIN) == LOW) { 
    // don't do anything as long as hold toggle is "on" but reset so we can assume operation normally
    resetAndWait(&green, &red);
    return;
  }

  // check if green made valid hit
  checkHit(&green, &red, now);
  // check if red made valid hit
  checkHit(&red, &green, now);

  // signal break in control circuit
  if (green.cbreak || red.cbreak) {
    signalTone();
    // signal on the corresponding white lamp
    digitalWrite(green.CBREAK_PIN, green.cbreak ? HIGH : LOW);
    digitalWrite(red.CBREAK_PIN, red.cbreak ? HIGH : LOW);
    // then reset everything
    resetAndWait(&green, &red);
    return;
  }

  // signal self hits
  signalSelfHit(&green, now);
  signalSelfHit(&red, now);

  // signal any hits made with light
  digitalWrite(green.SIGNAL_PIN, green.hit ? HIGH : LOW);
  digitalWrite(red.SIGNAL_PIN, red.hit ? HIGH : LOW);
  // and sound
  if (green.hit || red.hit) {
    signalTone();
  }

  // if any hits were made and lockout time has passed we can reset
  if ((green.hit && (now - green.lockout_time) > LOCKOUT_TIME) || (red.hit && (now - red.lockout_time) > LOCKOUT_TIME)) {
    resetAndWait(&green, &red);
  }
}  // end loop