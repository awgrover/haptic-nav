#pragma once

#include <Adafruit_MMA8451.h>
#include <Adafruit_NeoPixel.h>

#include <ExponentialSmooth.h>

#include "fmap.h"
#include "Navigation.h"

constexpr int ControlPixCount = 5;
Adafruit_NeoPixel control_neo(ControlPixCount, 5, NEO_RGB + NEO_KHZ800);

#include "EncoderNav.h"
#include "MMANav.h"
#include "PotNav.h"

class NavigationEmulator1 : public Navigation {

    int _turn_direction; // cached by turn_distance()

  public:
    MMANav mma_nav;
    PotNav pot_nav;
    EncoderNav encoder_nav;

    NavigationEmulator1() {
      //this->mma_nav = new MMA;
    }

    boolean begin() {
      // inits all it's dependants
      // call until true
      static boolean began = false;

      if (! began) {
        if (
          control_neo_begin()
          & encoder_nav.begin()
          & mma_nav.begin()
          & pot_nav.begin()
        ) {
          Serial << F("Nav Emulator ready\n");
          turn_direction(); // initial
          began = true;
        }
      }
      return began;
    }

    void update() {
      // call periodically
      mma_nav.read_mma_with_smooth();
      mma_nav.show_tilt();
      pot_nav.show_direction();
      encoder_nav.show_encoder();
    }

    void debug_mode(char command) {
      // from the "menu"
      switch (command) {
        case 't' :
          mma_nav.show_tilt();
          break;

        case 'M' :
          mma_nav.xyz_monitor();
          break;

        /*
          case '3':
          mma_nav.triangle_tilt_map2();
          break;

          case '2':
          mma_nav.triangle_tilt_map();
          break;
        */

        case '1':
          mma_nav.simple_tilt_map();
          break;

        case 'p' :
          pot_nav.show_direction();
          break;

        case 'E':
          encoder_nav.plot_encoder_raw();
          break;

        case 'e':
          encoder_nav.show_encoder();
          break;

        default :
          Serial << F("bad command ") << command << F(" in ") << F(__FILE__) << endl;
      }
    }

    int direction() {
      return pot_nav.direction();  // degrees
    }

    DistanceMode distance_mode() {
      return encoder_nav.distance_mode();  // see enum
    }

    int turn_direction() {
      return _turn_direction;
    }

    int turn_distance() {
      // calcs both
      float x = mma_nav.smooth_x();
      if (abs(x) > 0.2) {
        _turn_direction = x > 0 ? 1 : -1;
        int dist = map(
                     constrain(abs(x), 0, 2.0 ),
                     0.0, 2.0, 0.0, 50.0
                   );
        /*static Every debug(500);
          if (debug()) {
          Serial << F("Tdist ") << x << F(" ") << dist << endl;
          }*/
        return dist;
      }
      else {
        _turn_direction = 0;
        return 0;
      }
    }

    int distance() {
      return abs( 4000 - abs(mma_nav.smooth_y()) * 5000 ); // 0..30000
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

};
