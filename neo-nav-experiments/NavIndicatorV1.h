#pragma once

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
    static constexpr int center_pixel = 3;
    static constexpr int max_blue = 0x20, max_red = 0x15, max_white = 0x10; // fixme for brightness

    DistanceMode (&distance_mode)();
    int (&distance)();
    int (&direction)();
    int (&turn_distance)();
    int (&turn_direction)();

    NavIndicatorV1(
      DistanceMode (&distance_mode)(),
      int (&distance)(),
      int (&direction)(), // combine?
      int (&turn_distance)(),
      int (&turn_direction)()
    ) : distance_mode(distance_mode),
      distance(distance), direction(direction),
      turn_distance(turn_distance), turn_direction(turn_direction)
    {
    }

    void update() {
      static Every update_t(20);

      if (update_t()) {
        DistanceMode dm = distance_mode();
        static Changed<DistanceMode> dist_mode_changed;
        if ( dist_mode_changed(dm) ) {
          PWM.neo.fill(0, 0, 7);
          PWM.commit();
          Serial << F("dm ") << dm << endl;
        }

        boolean update_pwm = false;
        update_pwm =
          update_pwm
          | show_direction(dm)
          | show_turn(dm)
          ;

        if (update_pwm) PWM.commit();
      }
    }

    boolean show_turn(DistanceMode dist_mode) {
      // return true if PWM pixels should be updated

      switch (dist_mode) {
        case D_None:
          return false;
          break;

        case D_Here : // w/in gps discrimination ~ 3m
        case D_AHEAD : // 0-turns && < 20m (1 block)
          return false;
          break;
        case D_ALMOST : // 1-turn or not D_AHEAD
        case D_FAR :
          return turn_indicate(turn_distance(), turn_direction());
          break;
      }

      Serial << F(" unhandled dm ") << dist_mode << endl;
      return false;
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

    boolean show_direction(DistanceMode dist_mode) {
      // return true if PWM pixels should be updated
      /*
        (center 3) cyan->purple->red

        far : 1pixel triangle ?
        near : 2pixel triangle fade
        Almost : full width triangle, proportional to dist
        Here : just street side
      */

      switch (dist_mode) { // FIXME: fold the caseblocks together
        case D_None:
          return false;
          break;

        case D_Here : // w/in gps discrimination ~ 3m
          return this->blink();
          break;
        case D_AHEAD : // 0-turns && < 20m (1 block)
          return this->blink();
          break;
        case D_ALMOST : // 1-turn or not D_AHEAD
          return this->blink();
          break;
        case D_FAR :
          return single_dist(dist_mode, direction());
          break;
      }

      Serial << F(" unhandled dm ") << dist_mode << endl;
      return false;
    }

    boolean single_dist(DistanceMode dist_mode, int direction) {
      // L|R|C for distance indicator
      byte red, blue;
      static Changed<byte> red_changed, blue_changed;

      int pix = center_pixel;
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
        int d = (direction > 180) ? 180 - (direction - 180) : direction;
        red = map( d, 45, 180, 4, max_red );
        blue = map( d, 45, 180, max_blue, 4 );
      }

      static Changed<int> distance_brightness_changed;
      int distance_brightness = map(
                                  constrain(distance(), 0, 10000),
                                  0, 400, 0, max_white
                                );

      if (
        red_changed(red)
        | blue_changed(blue)
        | pix_changed(pix)
        | distance_brightness_changed(distance_brightness)
      ) {
        //Serial << F("D ") << distance() << F(" ") << distance_brightness << endl;
        unsigned long db =
          ((unsigned long)distance_brightness << 16)
          + ((unsigned long)distance_brightness << 8)
          + (distance_brightness)
          ;
        PWM.neo.fill(db, center_pixel - 1, 3 );
        if (direction >= (180 - 10) && direction <= (180 + 10) ) {
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
      return blinker( []() {
        PWM.neo.fill(0x101010ul * blinker.state, 0, 7);
        PWM.commit();
      });
    }
};
