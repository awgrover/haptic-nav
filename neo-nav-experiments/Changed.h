#pragma once

template<typename T>
class Changed {
    // track a value, report if changed vs a new value
  public:
    T was; // you get the default initial value (~0)
    Changed() {}
    Changed(T start) : was(start) {}

    boolean operator()(T newvalue) {
      if ( newvalue != was ) {
        was = newvalue;
        return true;
      }
      return false;
    }
};
