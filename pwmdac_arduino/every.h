#define get_now() unsigned long now = millis(); /* fetch millis once, share for scope */

// Could factor. HILO etc for on N, during N, after N
// micros isn't quite the same

#define on_millis(n, codeblock) \
    /* Requires get_now() at top */ \
    /* On every N millis, run codeblock, but only exactly on N, must be called every milli() */ \
    /* If called during every milli, If want exactly every N millis() */ \
    /* Does not fail at 3 day rollover, but gets the interval wrong by (n-1) - (2^64 % n) */ \
    /* codeblock can be a {...}, several statements, etc. but any "," must be inside a () */ \
    /* takes about .8 microseconds excluding codeblock, +.1 on a hit */ \
    { \
    static bool flipflop = 0; /* will call first time we hit multiple of n */ \
    if ( ! (now % n) ) { /* we need get_now() */ \
      if ( !flipflop ) { \
          codeblock;   \
          flipflop = 1; \
          } \
    } \
    else { \
        flipflop = 0; \
        } \
    }

#define every_millis(n, codeblock) \
    /* Requires get_now() at top */ \
    /* fail at 3 day rollover */ \
    /* After N millis pass, run codeblock, even if a lot after, window slips */ \
    /* codeblock can be a {...}, several statements, etc. but any "," must be inside a () */ \
    /* takes about 1.55 microseconds excluding codeblock, +0 on a hit */ \
    { \
    static unsigned long trigger_at = millis() + n; \
    if (now >= trigger_at) { /* We need get_now() at top */ \
        trigger_at = millis() + n; \
        codeblock; \
        } \
    }

#define every_micros(n, codeblock) \
    /* Will fail in 70 minutes or so ! */ \
    /* Requires get_micros_now() at top */ \
    /* After N micros pass, run codeblock, even if a lot after. Resolution is 4 or 8, see micros() in ref */ \
    /* codeblock can be a {...}, several statements, etc. but any "," must be inside a () */ \
    /* takes about 1.55 microseconds excluding codeblock, +0 on a hit */ \
    { \
    static unsigned long trigger_at = micros() + n; \
    /* the >= is based on where trigger_at starts! if TA is >, then >=, else <= */ \
    if (now_micros >= trigger_at) { /* We need get_now_micros() at top */ \
        trigger_at = micros() + n; \
        codeblock; \
        } \
    }

#define timen(ntimes, codeblock ) \
    /* run the codeblock in a foreach loop n times, print @__LINE__ micros */ \
    { \
    unsigned long start_time = micros(); \
    for (int i=0; i<ntimes; i++) { codeblock; } \
    float took = (float)(micros() - start_time)/ntimes; \
    Serial.print("Elapsed @");Serial.print(__LINE__);Serial.print(" ");Serial.print(took); Serial.println(" micros");\
    }

#define hilo_every_millis(millis_duration, codeblock) \
    /* Requires get_now() at top */ \
    /* Duration will be wrong at 3 day rollover by 2^64 ... something */ \
    /* once during each millis_duration, window does not slip, may skip */ \
    /* calling code_block with HILO set, e.g. ab_every_millis(10, digitalWrite(LED_BUILTIN, HILO);), starts HIGH */ \
    /* Not ON, but during the duration */ \
    /* takes about .8 micros, + .1 on change */ \
    { \
    static bool HILO = LOW; /* assume we will see clock early, so will be HIGH */ \
    short int new_state = now / millis_duration % 2; /* we need get_now() at top */ /* assume 1 HIGH 0 LOW */ \
    if ( HILO != new_state ) { /* i.e. changed */ \
        HILO = new_state; \
        /* for ab: if (new_state) { codeblock_a; } else { codeblock_b; } */ \
        codeblock; \
        } \
    }
