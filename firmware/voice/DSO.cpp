#include "DSO.h"


DSO::DSO(uint32_t srate) {
  dsosrate = srate;
  waveform = 0;
  freq = 110;
  amplitude = 0;
  PhaseInc = (uint32_t)(freq * srateFactor2 / dsosrate);
}

void DSO::setWaveform(uint8_t wave) {
  float iQS = 1.0 / QS;
  uint8_t index;

  waveform = wave;

  const int16_t *wave_ptr = wave1;

  switch (wave) {
    case 0:
      //Square
      wave_ptr = wave1;
      break;
    case 1:
      //Saw
      wave_ptr = wave1;
      break;
    case 2:
      //Pulse
      wave_ptr = wave2;
      break;
    case 3:
      //Sin
      wave_ptr = wave3;
      break;
    case 4:
      //Tri
      wave_ptr = wave4;
      break;
    case 5:
      //VariStep
      wave_ptr = wave5;
      break;
    case 6:
      //SkewSaw
      wave_ptr = wave6;
      break;
    case 7:
      //SkewSquare
      wave_ptr = wave7;
      break;
    case 8:
      //Smooth Brass
      wave_ptr = wave8;
      break;
    case 9:
      //Bass
      wave_ptr = wave9;
      break;
    case 10:
      //Dark FM
      wave_ptr = wave10;
      break;
    case 11:
      //MultiWave
      wave_ptr = wave11;
      break;
    case 12:
      //Bell FM
      wave_ptr = wave12;
      break;
    case 13:
      //Dark Pad
      wave_ptr = wave13;
      break;
    case 14:
      //Organ Mixture
      wave_ptr = wave14;
      break;
    case 15:
      //DCO Maze
      wave_ptr = wave15;
      break;
  }
  // Copier les données en RAM
  memcpy(data, wave_ptr, S*2);
}

void DSO::setFrequency(float f) {
  freq = f;
  PhaseInc = (uint32_t)(freq * srateFactor2 / dsosrate);
}

void DSO::setAmplitude(uint8_t a) {
  amplitude = a;
}

void DSO::setPw(int32_t Pw) {
  pwm = Pw * S / 256;
}

void DSO::update() {
    PhaseAcc += PhaseInc;

    uint16_t Index = PhaseAcc >> 17;             // Index actuel
    uint16_t nextIndex = (Index + 1) & (S - 1);  // Prochain échantillon

    uint16_t frac = (PhaseAcc & 0x1FFFF) >> 10;        // Fraction de phase (17 bits)

    // --- Première interpolation ---
    int16_t sampleA = data[Index];
    int16_t sampleB = data[nextIndex];
    int16_t interpolatedSample = sampleA + ((sampleB - sampleA) * frac >> 7);

    // --- Seconde interpolation pour PWM ---
    uint16_t pwmIndex = (Index + pwm) & (S - 1);
    uint16_t nextPwmIndex = (pwmIndex + 1) & (S - 1);

    int16_t samplePwmA = data[pwmIndex];
    int16_t samplePwmB = data[nextPwmIndex];
    int16_t interpolatedPwm = samplePwmA + ((samplePwmB - samplePwmA) * frac >> 7);

    // Génération du signal final
    if (waveform) {
        wave = interpolatedSample + interpolatedPwm;
    } else {
        wave = interpolatedSample - interpolatedPwm;
    }

    // Appliquer l’amplitude
    vdso = (wave * amplitude) >> 8;
  }

