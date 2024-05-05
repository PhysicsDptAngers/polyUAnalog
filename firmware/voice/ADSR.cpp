#include "ADSR.h"

// Constructeur de la classe ADSR
ADSR::ADSR(uint32_t rate)
  : ADSR_RATE(rate) {
  // Initialisation de l'état, de la valeur et du niveau de sustain
  state = NOTEOFF;
  value = 0;
  sustain = 0;

  // Réglage des paramètres d'attaque, de décroissance, de relâchement et de sustain par défaut
  setAttack(2);
  setDecay(10);
  setRelease(30);
  setSustain(100);
}

// Méthode pour commencer l'attaque
void ADSR::gateOn() {
  state = ATTACK;
}

// Méthode pour commencer le relâchement
void ADSR::gateOff() {
  state = RELEASE;
}

// Méthode pour couper le son
void ADSR::soundOff() {
  state = NOTEOFF;
  veg = 0;
  veg_a = (int16_t)(0 - sustain) >> 7;
}

// Méthode pour régler le temps d'attaque
void ADSR::setAttack(int8_t timems) {
  attack_time = ADSR_RATE * tablogMidi[timems] / 1000.0;
  if (timems < 2) attack_inc = AMAX + 1;
  else attack_inc = AMAX / attack_time;
}

// Méthode pour régler le temps de décroissance
void ADSR::setDecay(int8_t timems) {
  decay_time = ADSR_RATE * tablogMidi[timems] / 1000.0 - 1.0;
  decay_inc = (AMAX - sustain) / decay_time;
}

// Méthode pour régler le niveau de sustain
void ADSR::setSustain(int8_t value) {
  sustain = value << 7;
  decay_inc = (AMAX - sustain) / decay_time;
  release_inc = sustain / release_time;
}

// Méthode pour régler le temps de relâchement
void ADSR::setRelease(int8_t timems) {
  release_time = ADSR_RATE * tablogMidi[timems] / 1000.0;
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
