#include "LFO.h"
#include <cstdlib>  // Pour utiliser la fonction rand()

LFO::LFO(uint32_t rate)
  : srate(rate) {
  waveform = wtriangle;
  freq = 1;
  amplitude = 1;
  lfosrate = (65535.0 / srate);
  PhaseAcc = rand() % 65536;
  pwm = 64 << 9;
  PhaseInc = (uint16_t)(lfosrate * freq);
}

void LFO::setWaveform(uint8_t wave) {
  switch (wave) {
    case 0:
      waveform = wtriangle;
      break;
    case 1:
      waveform = wsaw;
      break;
    case 2:
      waveform = wsquare;
      break;
    case 3:
      waveform = wrandomize;
      break;
    case 4:
      waveform = wnoise;
      break;
  }
}

void LFO::setFrequency(float f) {
  freq = f;
  PhaseInc = (uint16_t)(lfosrate * freq);
}

void LFO::setAmplitude(float a) {
  amplitude = a;
}

int LFO::next() {
  return vlfo;
}

void LFO::update() {
  PhaseAcc += PhaseInc;
  switch (waveform) {
    case wtriangle:  // Convert phase into a triangle wave
      value = (PhaseAcc >> 7) & 0xFF;
      if (PhaseAcc & 0x8000) value = ~value;
      break;
    case wsaw:
      value = (PhaseAcc >> 8) & 0xFF;
      break;
    case wsquare:
      if (PhaseAcc > (pwm)) value = 0;
      else value = 0xFF;
      break;
    case wrandomize:
      if (PhaseAcc & 0x8000) {
        if (front) {
          value = rand() % 256;  // Générer une valeur aléatoire entre 0 et 255
          front = 0;
        }
      } else front = 1;
      break;
    case wnoise:
      value = rand() % 256;  // Générer une valeur aléatoire entre 0 et 255
    break;
  }
  vlfo = (int8_t)(value - 0x80) /* amplitude*/;
}
