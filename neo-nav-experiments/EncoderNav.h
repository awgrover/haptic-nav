// Paul Stoffregan:
#define ENCODER_OPTIMIZE_INTERRUPTS
#include <Encoder.h>

#include "Navigation.h"

class EncoderNav {
    /*
      Use the encoder to make distance_mode().
    */
  public:
    Encoder encoder = Encoder(2, 3); // pins

    boolean begin() {
      static boolean done = false;
      if (! done) {
        encoder.write( Navigation::D_FAR * 4 );
        Serial << "Encoder ready " << encoder.read() << endl;
        done = true;
      }
      return done;
    }

    Navigation::DistanceMode distance_mode() {

      // we get +4 per detent, so /4
      long value = (encoder.read() / 4);
      if ( value > 3)  {
        value = 3;
        encoder.write(value * 4); // correct back to encoder count = *4
      }
      else if ( value < 0 ) {
        value = 0;
        encoder.write(value);
      }
      //Serial << value << endl;
      return (Navigation::DistanceMode) value;
    }

    void plot_encoder_raw() {
      static long last = -32000;
      static Every heartbeat(1000);

      long value = distance_mode(); // encoder.read();

      if ( value != last ) {
        Serial << value << endl;
        last = value;
      }
      else if ( heartbeat() ) {
        Serial << value << endl;
      }

    }

    void show_encoder() {
      static const unsigned long distance_colors[] = {
        0x001500ul, // green="here"
        0x000020ul, // blue="this block"
        0x150020ul, // purple="1 turn"
        0x150000ul, // red="lots of turns"
      };

      static long last = -32000;

      long value = distance_mode();
      if ( value != last ) {
        /* for (int i = 0; i <= value; i++) {
          control_neo.clear();
          control_neo.setPixelColor(i, 0x20, 0x20, 0x20);
          } */
        control_neo.setPixelColor(0, distance_colors[ value ] );
        control_neo.show();
        //Serial << value << F(" 0x") << _HEX(distance_colors[ value ]) << endl;
        last = value;
      }
    }
};
