/*
  Use the encoder to make distance_mode().
*/

#include <Adafruit_NeoPixel.h>
// Paul Stoffregan:
#define ENCODER_OPTIMIZE_INTERRUPTS
#include <Encoder.h>

#include <every.h>

Encoder encoder(2, 3); // pins
const int EncoderPixCount = 5;
Adafruit_NeoPixel encoder_neo(EncoderPixCount, 5, NEO_RGB + NEO_KHZ800);
enum DistanceMode {
  D_Here, // w/in gps discrimination ~ 3m
  D_AHEAD, // 0-turns && < 20m (1 block)
  D_ALMOST, // 1-turn or not D_AHEAD
  D_FAR     // > 1-turn
};

boolean encoder_begin() {
  static Timer done_with_show(500ul);
  static boolean began = false;

  if (! began ) {
    began = true;
    encoder_neo.begin();
    encoder_neo.clear();
    encoder_neo.fill(0x010101ul, 0, EncoderPixCount );
    encoder_neo.setPixelColor(0, 0x20, 0, 0);
    encoder_neo.setPixelColor(1, 0, 0x20, 0);
    encoder_neo.setPixelColor(2, 0, 0, 0x20);
    encoder_neo.show();
  }

  static boolean done=false;
  done_with_show( []() {
    encoder_neo.clear();
    encoder_neo.show();
    Serial << F("encoder ready, discrete pixes ") << EncoderPixCount << endl;
    done = true;
  });
  return done;
}


long distance_mode() {

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
  return value;
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
      encoder_neo.clear();
      encoder_neo.setPixelColor(i, 0x20, 0x20, 0x20);
      } */
    encoder_neo.setPixelColor(0, distance_colors[ value ] );
    encoder_neo.show();
    Serial << value << F(" 0x") << _HEX(distance_colors[ value ]) << endl;
    last = value;
  }
}
