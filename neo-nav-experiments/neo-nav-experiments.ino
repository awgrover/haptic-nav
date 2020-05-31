/*
  Experiments in nav indication using 7 Neo strip

  Using the MMA8451, which has tilt (and accel).
  Next: use the full GPS/Abspos setup.

  L | C | R and behind
  turn proximity: mid, near, now
  Far, there, AT

*/
#include <Streaming.h>

#define NeoNumPixels 7
#define NeoI2CPin 6
#include <array_size.h>
#include <pwm/PWM_NeoPixel.h>
#include <every.h>
#include <ExponentialSmooth.h>

#include "encoder.h"

PWM_NeoPixel PWM;

#include <Adafruit_MMA8451.h>
Adafruit_MMA8451 mma;
sensors_event_t mma_data;
ExponentialSmooth<float> offset_x(100);

class NavIndicatorV1 {

    /*
      3 modes
        Far (>1 turn)
        Near 1 turn
        Here no turns (may need far if long distance >20m....)
      Indications
        Direction (center 3) cyan->purple->red
          far: 1pixel triangle?
          near: 2pixel triangle fade
          here: full width triangle, proportional on...
        Distance
          far mode only
          white leds: brightest farthest
            couple of blocks, <mile, >mile




      L | C | R and behind
      turn proximity: mid, near, now
      Far, there, AT
      center for direction, colors..:
        2,3,4
        L,C,R
    */
};

void setup() {
  unsigned long start = millis();

  Serial.begin(115200);
  Serial << F("Start") << endl;

  pinMode(LED_BUILTIN, OUTPUT);
  randomSeed(analogRead(0));

  while (
    !(
      encoder_begin()
      & strip_begin()
      & mma_begin()
    )
  ) {}

  Serial << F("Ready in ") << (millis() - start) << endl;
}

boolean mma_begin() {
  static boolean began = false;

  // till .begin() works
  if ( ! began  ) {
    if ( (began = mma.begin()) ) {
      Serial << F("MMA ready, range ") << pow(2, mma.getRange()) << F("g") << endl;
    }
    else {
      return false;
    }
  }

  static NTimes callibrate_x(offset_x.factor);
  boolean calibrating = callibrate_x([]() {
    mma.getEvent(&mma_data);
    offset_x.average( mma_data.acceleration.x );
    //Serial << callibrate_x.count << F(" ") << F(" ") << mma_data.acceleration.x << F(" ") <<offset_x.value() << endl;
  });
  static NTimes once(1);
  if ( ! calibrating && once() ) {
    Serial << F("mma x center ") << offset_x.value() << endl;
  }
  return ! calibrating;
}

boolean strip_begin() {
  static boolean began = false;

  if (! began) {
    if ( (began = PWM.begin(0)) ) {
      Serial << F("NEO ready, pixes ") << NeoNumPixels << endl;
      PWM.neo.clear();
      PWM.neo.fill(0x001010, 0, 6);
      PWM.commit();
    }
    else {
      return false;
    }
  }

  static After leave_pwm_on(300ul);
  return leave_pwm_on([]() {
    PWM.neo.clear();
    PWM.commit();
  });
}

void loop() {
  static char command = 'e'; // default is show prompt
  static boolean first = true;

  static Every check_command(20);
  check_command([]() {
    if (Serial.available() > 0) {
      command = Serial.read();
      Serial.println(command);
    }
  });

  //***
  //***********************
  //***

  if ( first ) {
    Serial << command << endl;
    first = false;
  }

  switch (command) {
    // set command to '?' to display menu w/prompt
    // set command to -1 to prompt
    // set command to -2 to just get input
    case 'E':
      plot_encoder_raw();
      break;

    case 'e':
      show_encoder();
      break;

    case '3':
      triangle_tilt_map2();
      break;

    case '2':
      triangle_tilt_map();
      break;

    case '1':
      simple_tilt_map();
      break;

    case 'M': // monitor xyz
      xyz_monitor();
      break;

    case '?': // show menu & prompt
    // menu made by: make (menu.mk):

    // end menu

    case -1 : // show prompt, get input
      Serial.print(F("Choose (? for help): "));
      while (Serial.available() <= 0) {}
      command = Serial.read();
      Serial.println(command);
      break;

    default : // show help if not understood
      delay(10); while (Serial.available() > 0) {
        Serial.read();  // empty buffer
        delay(10);
      }
      command = '?';
      break;
  }

}

void read_mma() {
  // with adjustments
  mma.getEvent(&mma_data);
  mma_data.acceleration.x -= offset_x.value();
}

float read_mma_with_smooth() {
  static ExponentialSmooth<float> mma_smooth(5);
  read_mma();
  return mma_smooth.average( mma_data.acceleration.x );
}

void xyz_monitor() {
  static Every mma_read(10);
  if ( mma_read() ) {
    read_mma();
    Serial << mma_data.acceleration.x << F(" ");
    Serial << mma_data.acceleration.y  << F(" ");
    Serial << mma_data.acceleration.z  << endl;
  }
}

float map(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void simple_tilt_map() {
  static Every update(20);
  static int last_pix = 3;

  if (update()) {
    float x = read_mma_with_smooth();
    float led_center = map( x, -2.0, 2.0, 0.0, 7.0);
    led_center = constrain(led_center, -1, 7);

    if ( (int) x != last_pix ) PWM.neo.setPixelColor( last_pix, 0x0 );
    PWM.neo.setPixelColor( led_center, 0x101000 );
    PWM.commit();
    last_pix = led_center;
  }
}

void triangle_tilt_map() {
  // a "Triangle" of brightness over +-1
  static Every update(40);
  const unsigned long Bright = 0x30;

  if (update()) {
    float x = read_mma_with_smooth();
    float led_center = map( x, -2.0, 2.0, 0.0, 7.0);
    led_center = constrain(led_center, -1, 8);

    float left_most = led_center - 1;
    float right_most = led_center + 1;
    // center,diff from int
    Serial << (int) led_center << F(" ") << (led_center - (int) led_center) << F(" ");

    for (float p = 0; p < 7; p++) {
      // outside our triangle (+- 1), it's zero
      if ( p < left_most || p > right_most) {
        Serial << 0 << F(" ");
        PWM.neo.setPixelColor( (int) p, 0, 0, 0 );
      }
      else {
        float diff;
        if (p <= led_center) {
          diff = p - left_most;
        }
        else if ( p > led_center) {
          diff = right_most - p;
        }
        float scale = diff - int(diff); // 0..1
        float scaled = Bright * scale;
        Serial << scaled << F(" ");
        PWM.neo.setPixelColor( p, scaled, scaled, 0 );
      }
    }

    Serial << endl;
    PWM.commit();
  }
}

void triangle_tilt_map2() {
  // a "Triangle" of brightness over +-2
  // pretty nice, but not nav useful?

  static Every update(40);
  const unsigned long Bright = 0x30;
  const int width = 2;

  if (update()) {
    float x = read_mma_with_smooth();
    float led_center = map( x, -2.0, 2.0, 0.0, 7.0);
    led_center = constrain(led_center, -1, 8);

    float left_most = led_center - width;
    float right_most = led_center + width;
    // center,diff from int
    Serial << (int) led_center << F(" ") << (led_center - (int) led_center) << F(" ");

    for (float p = 0; p < 7; p++) {
      // outside our triangle (+- 1), it's zero
      if ( p < left_most || p > right_most) {
        Serial << 0 << F(" ");
        PWM.neo.setPixelColor( (int) p, 0, 0, 0 );
      }
      else {
        float diff;
        if (p <= led_center) {
          diff = p - left_most;
        }
        else if ( p > led_center) {
          diff = right_most - p;
        }
        float scale = diff / width; // 0..1
        float scaled = Bright * scale;
        Serial << scaled << F(" ");
        PWM.neo.setPixelColor( p, scaled, scaled, 0 );
      }
    }

    Serial << endl;
    PWM.commit();
  }
}
