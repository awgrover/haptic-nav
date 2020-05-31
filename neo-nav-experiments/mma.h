#include <Adafruit_MMA8451.h>
Adafruit_MMA8451 mma;
sensors_event_t mma_data;
ExponentialSmooth<float> offset_x(100);

#include "fmap.h"

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

void read_mma() {
  // with adjustments
  static Every update(30);
  if ( update() ) {
    mma.getEvent(&mma_data);
    mma_data.acceleration.x -= offset_x.value();
  }
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

void show_tilt() {
  static Every update(20);

  if (update()) {
    // 2 is tilt left
    float x = read_mma_with_smooth();
    x = constrain(x, -2.0, 2.0);
    float delta = map( x, -2.0, 2.0, 0.0, 1.0);
    //Serial << x << F(" ") << delta << endl;

    byte left = 1, right = 1;
    int l_pix = 4, r_pix = 3;
    const int max_green = 0x30;
    if (delta > 0.55) {
      left = (delta - 0.5) * max_green;
    }
    else if (delta < 0.45) {
      right = ((1.0 - delta) - 0.5) * max_green;
    }
    control_neo.setPixelColor( l_pix, 00, left, 00 );
    control_neo.setPixelColor( r_pix, 00, right, 00 );
    control_neo.show();

  }
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

static int _turn_direction;
int turn_direction() {
  return _turn_direction;
}

int turn_distance() {
  // calcs both
  float x = read_mma_with_smooth();
  if (abs(x) > 0.2) {
    _turn_direction = x > 0 ? 1 : -1;
    int dist = map(
                 constrain(abs(x), 0, 2.0 ),
                 0.0, 2.0, 0.0,50.0
               );
    static Every debug(500);
    if (debug()) {
      Serial << F("Tdist ") << x << F(" ") << dist << endl;
    }
    return dist;
  }
  else {
    _turn_direction = 0;
    return 0;
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

int distance() {
  return distance_mode() * 100;
}
