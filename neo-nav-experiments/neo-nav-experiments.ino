

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

#include <every.h>

#include "Navigation.h"
#include "NavigationEmulator1.h"
#include "NavIndicatorV1.h"

NavigationEmulator1 navigation;
NavIndicatorV1 nav_indicator(navigation);

void setup() {

  unsigned long start = millis();
  static Timer timeout(1000);

  Serial.begin(115200);
  Serial << F("\nStart") << endl;

  pinMode(LED_BUILTIN, OUTPUT);
  randomSeed(analogRead(0));

  static Every setting_up_heartbeat(60);
  while ( ! (
            navigation.begin()
            | nav_indicator.begin()
          )
        )
  {
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
        navigation.update();
        nav_indicator.update();
      }
      break;

    case 'E':
    case 'e':
    case 'p' :
    case 't' :
    case '3':
    case '2':
    case '1':
    case 'M': // monitor xyz
      navigation.debug_mode(command);
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
