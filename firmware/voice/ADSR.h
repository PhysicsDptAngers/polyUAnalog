/*
    ADSR Envelope Generator

    The ADSR (Attack, Decay, Sustain, Release) class encapsulates an envelope generator 
    used for shaping the dynamics of audio signals in synthesizers and other musical applications. 
    This header file (ADSR.h) defines the interface and implementation details for the ADSR envelope.

    Key Components:

    ADSRState Enumeration: Defines the possible states of the ADSR envelope, 
    including ATTACK, DECAY, SUSTAIN, RELEASE, and NOTEOFF.
    ADSR Class: Represents the ADSR envelope generator, providing methods to set parameters 
    (attack, decay, sustain, release) and update its state based on triggers.

    Class Members:

    state: Current state of the ADSR envelope (e.g., ATTACK, DECAY, SUSTAIN).
    attack_inc, decay_inc, release_inc: Increment values for envelope stages (attack, decay, release).
    attack_time, decay_time, release_time: Time durations for envelope stages in milliseconds.
    sustain: Sustain level represented as a 16-bit integer.
    value: Current value of the envelope generator.
    ADSR_RATE: Sample rate used for envelope calculations.
    Public Methods:

    gateOn(): Triggers the attack phase of the envelope.
    gateOff(): Triggers the release phase of the envelope.
    soundOff(): Resets the envelope to the NOTEOFF state.
    setAttack(int8_t timems): Sets the attack time in milliseconds.
    setDecay(int8_t timems): Sets the decay time in milliseconds.
    setSustain(int8_t value): Sets the sustain level (0-127).
    setRelease(int8_t timems): Sets the release time in milliseconds.
    update(): Updates the envelope state based on its current phase.
    getADSRState(): Retrieves the current state of the ADSR envelope.
    Usage:

    The ADSR class can be instantiated with a specified sample rate (ADSR_RATE) 
    and used to control the amplitude envelope of sound signals 
    in real-time synthesis and audio processing.

*/

#ifndef ADSR_H
#define ADSR_H
#include <stdint.h>  // Include standard integer types for portability
//#include "tables.h"  // Include header file for necessary tables or constants
#include "aspin.h"

// Maximum amplitude value (16-bit)
#define AMAX 0x7FFF

// Enumeration for ADSR envelope states
enum ADSRState { ATTACK,
                 DECAY,
                 SUSTAIN,
                 RELEASE,
                 NOTEOFF };

class ADSR {
private:
  ADSRState state;       // Current state of the ADSR envelope
  uint16_t attack_inc;   // Increment value during attack phase
  uint16_t decay_inc;    // Increment value during decay phase
  uint16_t release_inc;  // Increment value during release phase
  float attack_time;     // Time duration of attack phase in milliseconds
  float decay_time;      // Time duration of decay phase in milliseconds
  int16_t sustain;       // Sustain level (0-127, mapped to 16-bit)
  float release_time;    // Time duration of release phase in milliseconds
  int16_t value;         // Current value of the envelope generator
  uint32_t ADSR_RATE;    // Sample rate for the ADSR envelope

public:  
// Constructor with sample rate argument (ADSR_RATE)
  ADSR(uint32_t rate);  
  void gateOn();    // Method to trigger the attack phase
  void gateOff();   // Method to trigger the release phase
  void soundOff();  // Method to turn off the sound (reset to NOTEOFF state)
  void setAttack(int32_t timems);// Method to set the attack time (in milliseconds)
  void setDecay(int32_t timems);// Method to set the decay time (in milliseconds)
  void setSustain(int8_t value);// Method to set the sustain level (0-127, mapped to 16-bit)
  void setRelease(int32_t timems);// Method to set the release time (in milliseconds)
  void update(); // Method to update the ADSR envelope state based on the current phase
  ADSRState getADSRState();
// Variables to hold the current envelope value and its adjustments
  int32_t veg;    // Normalized envelope value (0-127)
  int32_t veg_a;  // Adjusted envelope value based on sustain level
  int32_t veg_f;  // Full envelope value
};

#endif  // ADSR_H
