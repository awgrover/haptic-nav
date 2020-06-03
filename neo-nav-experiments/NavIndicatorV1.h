#pragma once

#define NeoNumPixels 7
#define NeoI2CPin 6
#include <pwm/PWM_NeoPixel.h>
#include <every.h>

#include "Changed.h"

class NavIndicatorV1 {

    /*
      3 modes
        Far (>1 turn)
        Near 1 turn
        Almost no turns (may need far if long distance >20m....)
        Here < gps discrimination
      Indications
        Direction (center 3) cyan->purple->red
          far: 1pixel triangle?
          near: 2pixel triangle fade
          Almost: full width triangle, proportional to dist
          Here: just street side
        Distance
          far mode only
          white leds: brightest farthest
            couple of blocks, <mile, >mile
        Turn
          far: 11|10|00 each side. 00 when straight. 0 is min




      L | C | R and behind
      turn proximity: mid, near, now
      Far, there, AT
      center for direction, colors..:
        2,3,4
        L,C,R
    */

  public:
    PWM_NeoPixel PWM;
    static constexpr int center_pixel = 3;
    static constexpr int max_blue = 0x20, max_red = 0x15, max_white = 0x10; // fixme for brightness

    Navigation &navigation;

    NavIndicatorV1( Navigation &navigation) : navigation(navigation) {
    }

    boolean begin() {
      static boolean began = false;

      if (! began) {
        if ( PWM.begin(0) ) {
          Serial << F("NEO ready, pixes ") << NeoNumPixels << endl;
          PWM.neo.clear();
          PWM.neo.fill(0x001010, 0, 6);
          PWM.commit();
          began = true;
        }
      }

      static Timer leave_pwm_on(300ul);
      static boolean done = false;
      if ( leave_pwm_on() ) {
        PWM.neo.clear();
        PWM.commit();
        //Serial << F("NEO showed\n");
        done = true;
      }
      //Serial << F("stripped! ") << done << endl;
      return done;
    }

    void update() {
      static Every update_t(20);

      if (update_t()) {

        Navigation::DistanceMode dm = navigation.distance_mode();
        static Changed<Navigation::DistanceMode> dist_mode_changed;
        if ( dist_mode_changed(dm) ) {
          PWM.neo.fill(0, 0, 7);
          PWM.commit();
          Serial << F("dm ") << dm << endl;
        }

        boolean update_pwm = false;
        update_pwm =
          update_pwm
          | show_turn_and_direction(dm)
          ;

        if (update_pwm) PWM.commit();
      }
    }


    boolean turn_indicate(int turn_distance, int turn_direction) {
      // return true if PWM pixels should be updated

      if ( turn_direction == 0 ) {
        PWM.neo.fill(0x000000, 0, 2);
        PWM.neo.fill(0x000000, 5, 2);
        return true;
      }
      else {
        int a, b;

        if ( turn_distance < 5 ) { // more like immediate
          a = 0x10;
          b = 0x10; // FIXME: max_green
        }
        else if ( turn_distance < 40 ) {
          // should be "next possible turn"
          a = 0x10;
          b = 0x01; // FIXME: max_green
        }
        else {
          // far
          a = 0x01;
          b = 0x01;
        }


        if ( turn_direction < 0 ) {
          PWM.neo.fill(0x000000, 0, 2);
          PWM.neo.setPixelColor( 0, 0, a, 0 );
          PWM.neo.setPixelColor( 1, 0, b, 0 );
        }
        else {
          PWM.neo.fill(0x000000, 5, 2);
          PWM.neo.setPixelColor( 6, 0, a, 0 );
          PWM.neo.setPixelColor( 5, 0, b, 0 );
        }

        return true;
      }
    }

    boolean show_turn_and_direction(Navigation::DistanceMode dist_mode) {
      // return true if PWM pixels should be updated
      /*
        (center 3) cyan->purple->red

        far : 1pixel triangle ?
        near : 2pixel triangle fade
        Almost : full width triangle, proportional to dist
        Here : just street side
      */

      switch (dist_mode) {
        case Navigation::D_NONE:
          return this->blink();
          break;

        case Navigation::D_HERE : // w/in gps discrimination ~ 3m
          //constexpr int gps_resolution = 10; // can't actually tell below that
          return this->blink();
          break;
        case Navigation::D_AHEAD : // 0-turns && < 20m (1 block)
          return near_distance( navigation.direction(), navigation.distance() );
          break;
        case Navigation::D_ALMOST : // 1-turn or not D_AHEAD
          return turn_indicate(navigation.turn_distance(), navigation.turn_direction())
                 | almost_distance(navigation.direction());
          break;
        case Navigation::D_FAR :
          return turn_indicate(navigation.turn_distance(), navigation.turn_direction())
                 | single_distance(navigation.direction());
          break;
      }

      Serial << F(" unhandled dm ") << dist_mode << endl;
      return false;
    }

    boolean calc_direction(int direction, int &pix, byte &red, byte &blue) {
      // true if a change
      // pix is the "center" pixel for the direction

      // L|R|C for distance indicator
      static Changed<byte> red_changed, blue_changed;

      pix = center_pixel;
      static Changed<int> pix_changed(-1);

      if (direction >= (360 - 45) || direction < 45) {
        // center takes up 90degrees! when far away
        red  = 0;
        blue = max_blue;
      }
      else if ( direction >= 180 ) {
        pix = center_pixel + 1;
      }
      else if ( direction >= 45 ) {
        pix = center_pixel - 1;
      }

      // "behindness" colors
      if ( direction >= 45 && direction < (360 - 45) ) {
        // just work in 0..180
        int d = (direction > 180) ? 180 - (direction - 180) : direction;
        red = map( d, 45, 180, 4, max_red );
        blue = map( d, 45, 180, max_blue, 4 );
      }

      return red_changed(red)
             | blue_changed(blue)
             | pix_changed(pix)
             ;
    }

    boolean near_distance(int direction, int distance) {
      // almost full width, use outside for distance

      boolean changed = false;
      byte d_value = constrain( map(distance, 0, 100, 0, max_white), 0, max_white);

      static Changed<byte> changed_d_v(-1);
      if ( changed_d_v(d_value) ) {
        PWM.neo.setPixelColor( (int) 0, d_value, d_value, d_value );
        PWM.neo.setPixelColor( (int) NeoNumPixels - 1, d_value, d_value, d_value );
        changed = true;
      }

      // direction
      const int width = 2; // actually width = 2x + 1

      byte red, blue;
      int pix; // ignored
      static Changed<byte> red_changed, blue_changed;
      calc_direction(direction, pix, red, blue);

      float d;
      // work in -180..180
      if (direction <= 180 ) {
        d = direction;
      }
      else {
        d = direction - 360;
      }
      //Serial << F("dir") << direction << F(" d") << d << F(" ");

      int pix_start = 1;
      int pix_end = NeoNumPixels - 2;
      
      static Changed<float> led_center_changed;
      float led_center = map( d, -180.0, 180.0, (float)pix_end+1, (float)pix_start-1 );

      if (led_center_changed( led_center )) {
        float left_most = led_center - width;
        float right_most = led_center + width;
        // center,diff from int
        //Serial << (int) led_center << F(" ") << (led_center - (int) led_center) << F(" ");

        for (float p = pix_start; p <= pix_end; p++) {
          // outside our triangle (+- 1), it's zero
          if ( p < left_most || p > right_most) {
            //Serial << 0 << F(" ");
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

            int redx = red * scale;
            if (redx == 0 && scale > 0.0) redx = 1;

            int bluex = blue * scale;
            if (bluex == 0 && scale > 0.0) bluex = 1;

            PWM.neo.setPixelColor( p, redx, 0, bluex );
          }
        }
        if (direction >= (180 - 10) && direction <= (180 + 10) ) {
          // "directly" behind
          PWM.neo.setPixelColor( pix_start, red, 0, blue, 0 );
          PWM.neo.setPixelColor( pix_end, red, 0, blue, 0 );
        }
        changed |= true;
      }

      //Serial << endl;

      return changed;
    }

    boolean almost_distance(int direction) {
      // no distance indicator, just direction
      // show direction as /\ across 3 pixels
      byte red, blue;
      //static Changed<byte> red_changed, blue_changed;

      int pix; // center pix for direction +(-1,0,1)
      static Changed<int> pix_changed(-1);

      // get us the color, ignore the pix
      boolean calc_changed = calc_direction(direction, pix, red, blue);

      // figure out the distribution /\ across the 3 pixels
      float d;
      // work in -180..180
      if (direction <= 180 ) {
        d = direction;
      }
      else {
        d = direction - 360;
      }
      //Serial << F("dir") << direction << F(" d") << d << F(" ");

      float center_pix = (NeoNumPixels - 1) / 2;

      float led_center = map( d, -180.0, 180.0, center_pix + 1, center_pix - 1);

      float left_most = led_center + 1;
      float right_most = led_center - 1;
      // center,diff from int
      //Serial << F("c") << led_center << F(" deltac ") << (led_center - (int) led_center) << F(" ");

      for (float p = center_pix - 1; p <= center_pix + 1; p++) {
        //Serial << F(" p") << p;
        if ( p > left_most || p < right_most) {
          // i.e. more than 1 away from center
          //Serial << 0 << F(" ");
          PWM.neo.setPixelColor( (int) p, 0, 0, 0 );
        }
        else {
          float diff;
          if (p < led_center) {
            diff =  (p - right_most);
          }
          else if ( p == led_center) {
            diff = 1.0;
          }
          else if ( p > led_center) {
            diff = (left_most - p);
          }
          float scale = diff; // - int(diff); // 0..1

          int redx = red * scale;
          if (redx == 0 && scale > 0.0) redx = 1;

          int bluex = blue * scale;
          if (bluex == 0 && scale > 0.0) bluex = 1;

          //Serial << F("/d") << diff;
          PWM.neo.setPixelColor( p, redx, 0, bluex );
        }
      }
      if (direction >= (180 - 10) && direction <= (180 + 10) ) {
        // "directly" behind
        PWM.neo.setPixelColor( center_pixel - 1, red, 0, blue, 0 );
        PWM.neo.setPixelColor( center_pixel + 1, red, 0, blue, 0 );
      }

      //Serial << endl;

      static Changed<float> center_changed(1000);

      if (
        calc_changed
        | center_changed( led_center )
      ) {
        return true;
      }
      else {
        return false;
      }
    }
    boolean single_distance(int direction) {
      byte red, blue;
      int pix;

      boolean calc_changed = calc_direction(direction, pix, red, blue);

      static Changed<int> distance_brightness_changed;
      int distance_brightness = map(
                                  constrain(navigation.distance(), 1000, 10000),
                                  0, 10000, 0, max_white
                                );
      //Serial << distance_brightness << F(" ") << navigation.distance() << endl;

      if (
        calc_changed
        | distance_brightness_changed(distance_brightness)
      ) {
        //Serial << F("D ") << navigation.distance() << F(" ") << distance_brightness << endl;
        unsigned long db =
          ((unsigned long)distance_brightness << 16)
          + ((unsigned long)distance_brightness << 8)
          + (distance_brightness)
          ;
        PWM.neo.fill(db, center_pixel - 1, 3 );
        if (direction >= (180 - 10) && direction <= (180 + 10) ) {
          // "directly" behind
          PWM.neo.setPixelColor( center_pixel - 1, red, 0, blue, 0 );
          PWM.neo.setPixelColor( center_pixel + 1, red, 0, blue, 0 );
        }
        else {
          PWM.neo.setPixelColor( pix, red, 0, blue, 0 );
        }
        return true;
      }
      return false;
    }

    boolean blink() {
      static Every::Toggle blinker(300);
      return blinker( [this]() {
        this->PWM.neo.fill(0x101010ul * blinker.state, 0, 7);
        this->PWM.commit();
      });
    }
};
