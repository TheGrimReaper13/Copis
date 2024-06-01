/*

  COPIS - 0.2
  A simple DIY wired scoring box for saber fencing based on Arduino

*/
const uint8_t SYSTEM_ERROR_PIN          = 13;     // on-board LED, use it if we need to signal system errors or such

const unsigned long   DEPRESS_TIME      = 400;    // in us, 0.4ms (0.1 - 1ms)
const unsigned long   LOCKOUT_TIME      = 170000; // in us, 170ms
const unsigned long   CBREAK_TIME       = 3000;   // in us, 3ms
const unsigned long   SELF_HIT_TIME     = 500000; // in us, 0.5s
const unsigned long   MIN_WHIPOVER_TIME = 4000;   // in us, 4ms
const unsigned long   MAX_WHIPOVER_TIME = 15000;  // in us, 15ms
const unsigned int    MAX_BOUNCES       = 10;     // max amount of times contact can be lost during parry while still protecting against whipover

const unsigned long   RESET_TIME        = 3000;   // in ms, 3s
const unsigned int    SIGNAL_FREQUENCY  = 3000;   // in Hz
unsigned long         SOUND_LENGTH      = 2000;   // in ms, 2s

const uint8_t         SOUND_SIGNAL_PIN  = A1;     // produces square wave and is connected to speaker
const uint8_t         HOLD_PIN          = A2;     // toggle switch, probably better to make this a momentary switch and a software toggle, could also just be replaced with a power switch

// use macros here to make struct initialization a bit more readible as Arduino IDE doesn't allow the use of specified initializers :(
# define GREEN_WEAPON_PIN   2
# define RED_WEAPON_PIN     3
# define GREEN_LAME_PIN     4
# define RED_LAME_PIN       5
# define GREEN_CONTROL_PIN  6
# define RED_CONTROL_PIN    7
# define GREEN_SIGNAL_PIN   8
# define RED_SIGNAL_PIN     9
# define GREEN_ERROR_PIN    10
# define RED_ERROR_PIN      11  
# define GREEN_SELF_HIT_PIN 12  
# define RED_SELF_HIT_PIN   A0  // don't use 13 as it's linked to on-board LED

struct Fencer {
  unsigned long depressed_time; // time when contact was made
  unsigned long lockout_time;   // time when valid hit was made
  unsigned long cbreak_time;    // time when break in control circuit was detected
  unsigned long parry_time;     // time when a both blades made contact
  unsigned long self_hit_time;  // remember time when we hit ourself so we can keep the light on for SELF_HIT_TIME
  unsigned int  bounce_counter; // prevent whip over protection if contact between blades was lost more than 10 times

  bool          hit;            // valid hit was made
  bool          error;          // break in control circuit
  bool          whip_over;      // active whipover protection 

  const uint8_t W_PIN;          // C line, weapon
  const uint8_t L_PIN;          // A line, lame
  const uint8_t C_PIN;          // B line, weapon
  const uint8_t SIGNAL_PIN;     // red/green lamp
  const uint8_t ERROR_PIN;      // white lamp
  const uint8_t SELF_HIT_PIN;   // yellow lamp
};

void signalTone() {
  tone(SOUND_SIGNAL_PIN, SIGNAL_FREQUENCY, SOUND_LENGTH);
} // end signalTone

// use these 2 mainly for prototype when we don't have easily visible LEDs yet, use signalTone for simultanious hit
// we could use loops here but compiler would probably unroll these anyways and it's easy enough to understand them anyways
void signalToneGreen() {
  // slow beeping
  tone(SOUND_SIGNAL_PIN, SIGNAL_FREQUENCY, 250); delay(500);
  tone(SOUND_SIGNAL_PIN, SIGNAL_FREQUENCY, 250); delay(500);
  tone(SOUND_SIGNAL_PIN, SIGNAL_FREQUENCY, 250); delay(500);
  tone(SOUND_SIGNAL_PIN, SIGNAL_FREQUENCY, 250);
} // end signalToneGreen

void signalToneRed() {
  // fast beeping
  tone(SOUND_SIGNAL_PIN, SIGNAL_FREQUENCY, 124); delay(250);
  tone(SOUND_SIGNAL_PIN, SIGNAL_FREQUENCY, 124); delay(250);
  tone(SOUND_SIGNAL_PIN, SIGNAL_FREQUENCY, 124); delay(250);
  tone(SOUND_SIGNAL_PIN, SIGNAL_FREQUENCY, 124); delay(250);
  tone(SOUND_SIGNAL_PIN, SIGNAL_FREQUENCY, 124); delay(250);
  tone(SOUND_SIGNAL_PIN, SIGNAL_FREQUENCY, 124); delay(250);
  tone(SOUND_SIGNAL_PIN, SIGNAL_FREQUENCY, 124); delay(250);
  tone(SOUND_SIGNAL_PIN, SIGNAL_FREQUENCY, 124);
} // end signalToneRed

void reset(Fencer *p) {
  // W_PIN should always LOW after exiting checkHit, but set it low just to be sure
  digitalWrite(p->W_PIN, LOW);
  // turn off signal lights
  digitalWrite(p->SIGNAL_PIN, LOW);
  digitalWrite(p->ERROR_PIN, LOW);
  digitalWrite(p->SELF_HIT_PIN, LOW);

  p->depressed_time = 0;
  p->lockout_time = 0;
  p->cbreak_time = 0;
  p->parry_time = 0;
  p->self_hit_time = 0;

  p->bounce_counter = 0;

  p->hit = false;
  p->error = false;
  p->whip_over = false;
} // end reset

void resetForNextHit(Fencer *a, Fencer *b) {
  delay(RESET_TIME);

  reset(a);
  reset(b);
} // end reset

// checks if attacker made valid hit
void checkHit(Fencer *att, Fencer *def, unsigned long now) {
  
  digitalWrite(att->W_PIN, HIGH);
  // there's a change we need to wait for a very short amount of time to allow reliable signal reading (seems not to ne needed)

  // check for self hit, we don't need to interrupt anything, just signal briefly with light
  if (digitalRead(att->L_PIN) == HIGH) {
    att->self_hit_time = now;
  }
  else if (att->self_hit_time != 0 && (now - att->self_hit_time) > SELF_HIT_TIME) {
    att->self_hit_time = 0;
  }

  // Whip-over
  const unsigned long parry_diff = now - att->parry_time;  // save diff as constant as we need it several times

  // check if blades made contact
  if (digitalRead(def->C_PIN) == HIGH) {
    // start timer first time we made contact
    if (att->parry_time == 0) {
      att->parry_time = now; 
    }
    // only disable hits with an interval of 4-15ms between the contact of the blades and contact between blade and lame
    else if (parry_diff >= MIN_WHIPOVER_TIME && parry_diff < MAX_WHIPOVER_TIME) {
# ifdef WHIP_OVER
      att->whip_over = true;
# endif
    }
  }
  // if bounce_couner > 0, parry_diff will always be in intented range
  else if ((att->bounce_counter > 0 && att->bounce_counter < MAX_BOUNCES) && (parry_diff >= MIN_WHIPOVER_TIME && parry_diff < MAX_WHIPOVER_TIME)) {
# ifdef WHIP_OVER
    att->whip_over = true;
# endif
  }
  // no contact between blades so we reset parry timer and if parry_time was already set we advance bounce_counter as contact between blades has been lost
  else if (att->parry_time != 0) {
    att->bounce_counter++;
    //att->parry_time = 0;
  }

  // reset if no hit was made after whip_over time has passed or contact between blades was interrupted more than 10 times
  if (parry_diff > MAX_WHIPOVER_TIME || att->bounce_counter > MAX_BOUNCES) {
    att->parry_time = 0;
    att->bounce_counter = 0;
    att->whip_over = false;
  }

  // reset cbreak time if we can detect a signal again
  if (digitalRead(att->C_PIN) == HIGH) {
    att->cbreak_time = 0;
  }

  // check control circuit, if we can't read a signal on the control circuit for CBREAK_TIME we set error flag
  if (digitalRead(att->C_PIN) == LOW) {
    // start timer
    if (att->cbreak_time == 0) {
      att->cbreak_time = now;
    }
    else if ((now - att->cbreak_time) > CBREAK_TIME) {
      att->error = true;
    }
  }
  // only make reading if whip_over protection is not active
  // if we can read HIGH on red L_PIN there should be contact between greens weapon and reds lame
  // this can't be true if C_PIN read low
  else if (!att->whip_over && digitalRead(def->L_PIN) == HIGH) {
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
  // contact lost so we reset depressed time, this will also be called if whip_over is active
  else {
    att->depressed_time = 0;
  }

  digitalWrite(att->W_PIN, LOW);
} // end checkHit

void setup() {
  pinMode(HOLD_PIN, INPUT_PULLUP);
  pinMode(SOUND_SIGNAL_PIN, OUTPUT);

  pinMode(GREEN_LAME_PIN, INPUT);         pinMode(RED_LAME_PIN, INPUT);
  pinMode(GREEN_CONTROL_PIN, INPUT);      pinMode(RED_CONTROL_PIN, INPUT);

  pinMode(GREEN_WEAPON_PIN, OUTPUT);      pinMode(RED_WEAPON_PIN, OUTPUT);
  pinMode(GREEN_SIGNAL_PIN, OUTPUT);      pinMode(RED_SIGNAL_PIN, OUTPUT);
  pinMode(GREEN_ERROR_PIN, OUTPUT);       pinMode(RED_ERROR_PIN, OUTPUT);
  pinMode(GREEN_SELF_HIT_PIN, OUTPUT);    pinMode(RED_SELF_HIT_PIN, OUTPUT);


  digitalWrite(GREEN_WEAPON_PIN, LOW);    digitalWrite(RED_WEAPON_PIN, LOW);
  digitalWrite(GREEN_SIGNAL_PIN, LOW);    digitalWrite(RED_SIGNAL_PIN, LOW);
  digitalWrite(GREEN_ERROR_PIN, LOW);     digitalWrite(RED_ERROR_PIN, LOW);
  digitalWrite(GREEN_SELF_HIT_PIN, LOW);  digitalWrite(RED_SELF_HIT_PIN, LOW);
}  // end setup

void loop() {

    static Fencer green = { 
    0, 0, 0, 0, 0, 0,
    false, false, false,
    GREEN_WEAPON_PIN, GREEN_LAME_PIN,   GREEN_CONTROL_PIN,
    GREEN_SIGNAL_PIN, GREEN_ERROR_PIN,  GREEN_SELF_HIT_PIN
  };

  static Fencer red = { 
    0, 0, 0, 0, 0, 0,
    false, false, false,
    RED_WEAPON_PIN,   RED_LAME_PIN,     RED_CONTROL_PIN,
    RED_SIGNAL_PIN,   RED_ERROR_PIN,    RED_SELF_HIT_PIN
  };

  const unsigned long now = micros();

  // check if hold pin is low as we pulled it up
  if (digitalRead(HOLD_PIN) == LOW) {
    // don't do anything as long as hold toggle is "on" but reset so we can assume operation normally
    resetForNextHit(&green, &red);
    return;
  }

  // check if green made valid hit
  checkHit(&green, &red, now);
  // check if red made valid hit
  checkHit(&red, &green, now);

  // signal self hits
  digitalWrite(green.SELF_HIT_PIN, green.self_hit_time != 0 ? HIGH : LOW);
  digitalWrite(red.SELF_HIT_PIN, red.self_hit_time != 0 ? HIGH : LOW);

  if (green.error || red.error) {
    // if we ecountered error signal with tone
    signalTone();
    // signal on the corresponding white lamp
    digitalWrite(green.ERROR_PIN, green.error ? HIGH : LOW);
    digitalWrite(red.ERROR_PIN, red.error ? HIGH : LOW);
    // then again reset everything
    resetForNextHit(&green, &red);
  }
  // if at least one player made a hit we can check if lockout time has expired, if so we can signal hits and reset the routine
  else if ((green.hit && (now - green.lockout_time) > LOCKOUT_TIME) || (red.hit && (now - red.lockout_time) > LOCKOUT_TIME)) {
      // light signal
    digitalWrite(green.SIGNAL_PIN, green.hit ? HIGH : LOW);
    digitalWrite(red.SIGNAL_PIN, red.hit ? HIGH : LOW);
    
    // sound signal
    if (green.hit && red.hit) {
      signalTone();
    }
    else if (green.hit) {
      signalToneGreen();
    }
    else if (red.hit) {
      signalToneRed();
    }

    resetForNextHit(&green, &red);
  }
}  // end loop
