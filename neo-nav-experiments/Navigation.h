#pragma once
#include <ExponentialSmooth.h>

class Navigation {
    // api for navigating info
    // emulator for navigating info
    // mma, 2 bit encoder, pot

  public:
    enum DistanceMode {
      D_NONE = -1, // good for initing
      D_HERE, // w/in gps discrimination ~ 3m
      D_AHEAD, // 0-turns && < 20m (1 block)
      D_ALMOST, // 1-turn or not D_AHEAD
      D_FAR     // > 1-turn
    };

    virtual boolean begin() = 0; // inits all it's dependants
    virtual void update() = 0; // stuff that is periodic
    virtual int distance() = 0; // abs meters
    virtual int direction() = 0; // degrees
    virtual int turn_distance() = 0;
    virtual int turn_direction() = 0; // -1 | 0 | 1 // refine for angle of turn.
    virtual DistanceMode distance_mode() = 0; // see enum
    virtual void debug_mode(char command) = 0; // handle some menu stuff
};
