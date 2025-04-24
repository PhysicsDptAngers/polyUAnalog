#include "ADSR.h"

// Constructor for the ADSR class
ADSR::ADSR(uint32_t rate)
  : ADSR_RATE(rate) {
  // Initialize the state, value, and sustain level
  state = NOTEOFF;  // Set the initial state to NOTEOFF
  value = 0;        // Initialize the ADSR envelope value to 0
  sustain = 0;      // Set the sustain level to 0 initially

  // Set default attack, decay, release, and sustain parameters
  setAttack(2);     // Set attack time (2 milliseconds) for the ADSR envelope
  setDecay(10);     // Set decay time (10 milliseconds) for the ADSR envelope
  setRelease(30);   // Set release time (30 milliseconds) for the ADSR envelope
  setSustain(100);  // Set sustain level (100) for the ADSR envelope
}

// Method to initiate attack phase
void ADSR::gateOn() {
  state = ATTACK;// Set the ADSR state to ATTACK, triggering the attack phase
}

// Method to initiate release phase
void ADSR::gateOff() {
  state = RELEASE;// Set the ADSR state to RELEASE if the current state is not NOTEOFF
}

// Method to silence the ADSR envelope
void ADSR::soundOff() {
  state = NOTEOFF;  // Set the ADSR state to NOTEOFF, stopping the envelope generation
  veg = 0;  // Reset the envelope generator value to zero
  veg_a = (int16_t)(0 - sustain) >> 7;  // Calculate the release envelope rate for a smooth release transition
  veg_f = 0;
}

// Method to set the attack time of the ADSR envelope
void ADSR::setAttack(int32_t timems) {
  attack_time = ADSR_RATE * timems / 1000.0;  // Calculate attack time in samples
  if (timems < 2)
    attack_inc = AMAX + 1;  // Set attack increment for very short times (less than 2ms)
  else
    attack_inc = AMAX / attack_time;  // Calculate attack increment based on the attack time
}

// Method to set the decay time of the ADSR envelope
void ADSR::setDecay(int32_t timems) {
  decay_time = ADSR_RATE * timems / 1000.0 - 1.0;
  decay_inc = (AMAX - sustain) / decay_time;
}

// Méthode pour régler le niveau de sustain
void ADSR::setSustain(int8_t value) {
  sustain = value << 7;
  decay_inc = (AMAX - sustain) / decay_time;
  release_inc = sustain / release_time;
}

// Méthode pour régler le temps de relâchement
void ADSR::setRelease(int32_t timems) {
  release_time = ADSR_RATE * timems / 1000.0;
  release_inc = sustain / release_time;
}

// Méthode pour mettre à jour l'enveloppe en fonction de l'état actuel
void ADSR::update() {
  if (state == NOTEOFF) value = 0;
  else switch (state) {
      case ATTACK:
        // Si l'état est ATTACK, la valeur est augmentée
        value += attack_inc;
        if (value <= 0) {
          // Si la valeur dépasse AMAX, passer à l'état DECAY
          state = DECAY;
          value = AMAX;
        }
        break;
      case DECAY:
        // Si l'état est DECAY, la valeur est réduite
        value -= decay_inc;
        if (value <= sustain) {
          // Si la valeur tombe en dessous du niveau de sustain, passer à l'état SUSTAIN
          state = SUSTAIN;
          value = sustain;
        }
        break;
      case SUSTAIN:
        // Si l'état est SUSTAIN, la valeur reste constante
        if (value <= 0) {
          // Si le sustain est à zéro, passer à l'état NOTEOFF
          state = NOTEOFF;
          value = 0;
        }
        break;
      case RELEASE:
        // Si l'état est RELEASE, la valeur est réduite
        value -= release_inc;
        if (value <= 0) {
          // Si la valeur tombe à zéro, passer à l'état NOTEOFF
          state = NOTEOFF;
          value = 0;
        }
        break;
      default:
        state = NOTEOFF;
        value = 0;
        break;
    }
  veg = value >> 7;
  veg_f = PWMResVCA * value / AMAX;
  veg_a = (int16_t)(value - sustain) >> 7;
}

ADSRState ADSR::getADSRState() {
  return state;
}
