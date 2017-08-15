/* 
    Adapted from: http://makezine.com/projects/make-35/advanced-arduino-sound-synthesis/
    No apparent license.

    Uses pin 9 as a PWM output. Expects a poor-man's DAC on there:
        pin9 -> 10K -> audio
                    -> .1microFarad -> GND

    * repaired "prog_mem"
    * ignore narrowing warnings
    * added more comments

    Needs
    * hook up compass
    * package as a class or something, for 1..n pwm's
    * pin 10 & 9 are using the same wavetable. Figure out stereo
    * volume scaled by frequency
    * Consider "direct digital synthesis arduino" alt method
*/

#include <avr/interrupt.h> // Load AVR timer interrupt macros
#include <avr/pgmspace.h> // for PROGMEM

const int MYCLOCK_M=16; // set this. Uno 16. nano 16.
const int WANT_TIMER_M = 2; // want 2mhz
const int VOLUME = 4; // actually, the volume divider. 1/nth as loud. needs to scale with the frequency though

/******** Lookup table ********/
#define LENGTH  256  // The length of the waveform lookup table
#pragma GCC diagnostic ignored "-Wnarrowing"
    // otherwise: tons of warnings about int->const char
    // FIXME: probably make correct as uc
// sinewave apparently
const char ref[256] PROGMEM = {     128,131,134,137,140,143,146,149,152,155,158,161,164,167,170,173, 176,179,182,185,187,190,193,195,198,201,203,206,208,210,213,215,
217,219,222,224,226,228,230,231,233,235,236,238,240,241,242,244,
245,246,247,248,249,250,251,251,252,253,253,254,254,254,254,254,
255,254,254,254,254,254,253,253,252,251,251,250,249,248,247,246,
245,244,242,241,240,238,236,235,233,231,230,228,226,224,222,219,
217,215,213,210,208,206,203,201,198,195,193,190,187,185,182,179,
176,173,170,167,164,161,158,155,152,149,146,143,140,137,134,131,
128,124,121,118,115,112,109,106,103,100, 97, 94, 91, 88, 85, 82,
79,  76, 73, 70, 68, 65, 62, 60, 57, 54, 52, 49, 47, 45, 42, 40,
38,  36, 33, 31, 29, 27, 25, 24, 22, 20, 19, 17, 15, 14, 13, 11,
10,   9,  8,  7,  6,  5,  4,  4,  3,  2,  2,  1,  1,  1,  1,  1,
1,    1,  1,  1,  1,  1,  2,  2,  3,  4,  4,  5,  6,  7,  8,  9,
10,  11, 13, 14, 15, 17, 19, 20, 22, 24, 25, 27, 29, 31, 33, 36,
38,  40, 42, 45, 47, 49, 52, 54, 57, 60, 62, 65, 68, 70, 73, 76,
79,  82, 85, 88, 91, 94, 97,100,103,106,109,112,115,118,121,124};
#pragma GCC diagnostic warning "-Wnarrowing"

byte wave[LENGTH];

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  for (int i=0; i<LENGTH; i++) {
    wave[i] = pgm_read_byte(&ref[i]); // Don't need to copy. but, PROGMEM is ~5(?)/read vs ~2/read for ram
  }
  
  /******** Set timer1 for 8-bit fast PWM output ********/
  pinMode(9, OUTPUT);
  pinMode(10, OUTPUT);

  OCR1AL = 0; // initial frequency

  TCCR1B  = (1 << CS10);    // Set prescaler to full 16MHz
  TCCR1A |= (1 << COM1A1);  // PWM 9 pin to go low when TCNT1=OCR1A
  TCCR1A |= (1 << COM1B1);  // PWM 10 pin to go low when TCNT1=OCR1A
  TCCR1A |= (1 << WGM10);   // Put timer into 8-bit fast PWM mode
  TCCR1B |= (1 << WGM12);   // Maybe this is fast? or the 8 bit?

  // Set up timer 2 to call ISR 
  TCCR2A = 0; // We need no options in control register A
  TCCR2B = (1 << CS21); // want 2mhz, so prescalar to divide by 8. how fast TCNT2 is updated
  TIMSK2 = (1 << OCIE2A); // Set timer to call ISR when TCNT2 = OCRA2
  OCR2A = 64;  // Sample every 32 tics. 2.000.000 / (freq * 256) = OCR2A. so, 256 samples -> frequency
  sei(); // // Enable interrupts to generate waveform!


}

void loop() {  // Nothing to do!
  static uint8_t x=10;
  static int HILO = HIGH;

  OCR2A = x--;
  if (x > 150U) x=80;

  HILO = millis()/200 % 2;
  digitalWrite(LED_BUILTIN, HILO);

  // delay( 8 + (80 - x) / 40 * 10 );
  delay(8);
}

ISR(TIMER2_COMPA_vect) { // Call when TCNT2 = OCRA2
  // tuned for approx 16mhz
  static byte index=0; // auto wrap around? seems crude. not unsigned?
  OCR1AL = pgm_read_byte(&ref[index]); // wave[index] / VOLUME; // just added several ticks to this routine
  OCR1BL = wave[index]; // apparently pin 10 pwm
  index += 1;
  asm("NOP;NOP"); // estimate, FIXME: should self-calibrate
  TCNT2=6; //spent in ISR
}

