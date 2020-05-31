/*
  Experiments in nav indication using 7 Neo strip

  Using the MMA8451, which has tilt (and accel).
  Next: use the full GPS/Abspos setup.

  L | C | R and behind
  turn proximity: mid, near, now
  Far, there, AT

*/
   
#include <Streaming.h>
#include <array_size.h>
#include "fmap.h"

#define NeoNumPixels 7
#define NeoI2CPin 6
#include <pwm/PWM_NeoPixel.h>
#include <every.h>
#include <ExponentialSmooth.h>

PWM_NeoPixel PWM;

const int ControlPixCount = 5;
Adafruit_NeoPixel control_neo(ControlPixCount, 5, NEO_RGB + NEO_KHZ800);

#include "encoder.h"
#include "mma.h"
#include "pot.h"

#include "NavIndicatorV1.h"

void setup() {
  
  unsigned long start = millis();
  static Timer timeout(1000);

  Serial.begin(115200);
  Serial << F("\nStart") << endl;

  pinMode(LED_BUILTIN, OUTPUT);
  randomSeed(analogRead(0));

  static Every setting_up_heartbeat(60);
  while (
    !(
      control_neo_begin()
      & strip_begin()
      & encoder_begin()
      & mma_begin()
      & pot_begin()
    )
  ) {
    // flashes rapidly while waiting for all .begin
    setting_up_heartbeat( []() {
      digitalWrite(LED_BUILTIN, ! digitalRead(LED_BUILTIN) );
    });

    if (timeout()) {
      Serial << F("setup() timed out in ") << timeout.interval << F("msecs\n");
      digitalWrite(LED_BUILTIN, HIGH);
      while (1) {}
    }

    //if ( encoder_begin() ) { Serial << F("E ok\n"); }
    //if ( strip_begin() ) { Serial << F("S ok\n");}
    //if ( mma_begin() ) { Serial << F("M ok\n"); }
  }

  digitalWrite(LED_BUILTIN, HIGH);

  Serial << F("Ready in ") << (millis() - start) << endl << endl;
}

//void func_with_callback(std::function<void()> callback) {}

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

  static Timer leave_pwm_on(300ul);
  static boolean done = false;
  leave_pwm_on([]() {
    PWM.neo.clear();
    PWM.commit();
    //Serial << F("NEO showed\n");
    done = true;
  });
  //Serial << F("stripped! ") << done << endl;
  return done;
}


boolean control_neo_begin() {
  static Timer done_with_show(500ul);
  static boolean began = false;

  if (! began ) {
    began = true;
    control_neo.begin();
    control_neo.clear();
    control_neo.fill(0x010101ul, 0, ControlPixCount );
    control_neo.setPixelColor(0, 0x20, 0, 0);
    control_neo.setPixelColor(1, 0, 0x20, 0);
    control_neo.setPixelColor(2, 0, 0, 0x20);
    control_neo.show();
  }

  static boolean done = false;
  done_with_show( []() {
    control_neo.clear();
    control_neo.show();
    Serial << F("control neo ready, discrete pixes ") << ControlPixCount << endl;
    done = true;
  });
  return done;
}

void loop() {
  static char command = 'a'; // default is show prompt
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
    Serial << F("Command: ") << command << endl;
    first = false;
  }

  switch (command) {
    // set command to '?' to display menu w/prompt
    // set command to -1 to prompt
    // set command to -2 to just get input
    case 'a' : {
        static NavIndicatorV1 ni(
          distance_mode,
          distance,
          direction,
          turn_distance,
          turn_direction
          );
        show_tilt();
        show_direction();
        show_encoder();
        ni.update();
      }
      break;
      
    case 't' :
      show_tilt();
      break;

    case 'p' :
      show_direction();
      break;

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

  // so you know it's running
  static Every::Pattern heartbeat(500);
  heartbeat( []() {
    digitalWrite(LED_BUILTIN, ! digitalRead(LED_BUILTIN) );
  });

}
