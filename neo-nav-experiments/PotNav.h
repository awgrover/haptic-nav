#pragma once

class PotNav {
    static constexpr int PotPin = A0;
    ExponentialSmooth<int> pot_smooth = ExponentialSmooth<int>(10);

  public:
    boolean begin() {
      static boolean began = false;
      if ( ! began ) {
        pinMode(PotPin, INPUT);
        Serial <<  F("POT began") << endl;
        began = true;
      }

      static NTimes init_pot(pot_smooth.factor);
      boolean calibrating = init_pot([this]() {
        int pot = analogRead( PotPin );
        this->pot_smooth.average( pot );
        //Serial << init_pot.count << F(" ") << F(" ") << pot << F(" ") <<pot_smooth.value() << endl;
      });
      static NTimes once(1);
      if ( ! calibrating && once() ) {
        Serial << F("pot current ") << pot_smooth.value() << endl;
      }
      return ! calibrating;
    }

    int direction() {
      // pretend degrees
      static int last = -1;
      static Every pot_update(20);
      static int degrees = 0;

      if ( pot_update() ) {
        int pot = analogRead( PotPin );
        int current = pot_smooth.average( pot );
        if (current != last) {
          last = current;
          degrees = map( current, 50, 990, 0, 360 );
          degrees = constrain( degrees, 0, 360);
          degrees = (degrees + 180 ) % 360; // dead is behind
        }
      }
      return degrees;
    }

    boolean t_calculate_direction(
      // just a test calculate, for control_neo
      //returns true if updated value
      int max_red, int max_blue, // max range
      // return
      int &pix, // 0,1 for left right,
      int &red, int &blue
    ) {
      // pretend degrees
      static int last = -1;
      static Every pot_update(20);

      if ( pot_update() ) {
        int pot = analogRead( PotPin );
        int current = pot_smooth.average( pot );
        if (current != last) {
          last = current;
          int degrees = map( current, 10, 1010, 0, 360 );
          degrees = constrain( degrees, 0, 360);

          // Serial << pot << F(" ") << degrees << endl;

          // calculate
          // Front is blue, rear is red, in-between is purple
          // 0..180 is right, 180..360 is left

          control_neo.fill(0x0, 1, 2);
          // we are using the discreet strip, conveniently oriented
          pix = degrees < 180 ? 1 : 2;
          if (degrees > 180) degrees = 180 - (degrees - 180); // so 0..180 again
          // should be a factor of distance_mode:

          const int front_range = 15;

          if ( degrees <= front_range) {
            red = 0; blue = max_blue;
          }
          else {
            red = map( degrees, 0, 180, 0, max_red );
            blue = map( degrees, 0, 180, max_blue, 0 );
          }
          return true;
        }
      }
      return false;
    }

    void show_direction() {
      int red, blue;
      int pix;
      if ( t_calculate_direction(15, 20,
                                 pix, red, blue
                                )) {
        control_neo.setPixelColor(pix, red, 00, blue);

        control_neo.show();
      }
    }
};
