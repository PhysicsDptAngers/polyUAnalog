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

  switch (wave) {
    case 0:
      //Saw
      data = wave1;
      break;
    case 1:
      //Square
      data = wave1;
      break;
    case 2:
      //Pulse
      data = wave2;
      break;
    case 3:
      //Sine
      data = wave3;
      break;
    case 4:
      //Tri
      data = wave4;
      break;
    case 5:
      //VariStep
      data = wave5;
      break;
    case 6:
      //SkewSaw
      data = wave6;
      break;
    case 7:
      //SkewSquare
      data = wave7;
      break;
    case 8:
      data = wave8;
      break;
    case 9:
      data = wave9;
      break;
    case 10:
      data = wave10;
      break;
    case 11:
      data = wave11;
      break;
    case 12:
      data = wave12;
      break;
    case 13:
      data = wave13;
      break;
    case 14:
      data = wave14;
      break;
    case 15:
      data = wave15;
      break;
  }
}

void DSO::setFrequency(float f) {
  freq = f;
  PhaseInc = (uint32_t)(freq * srateFactor2 / dsosrate);
}

void DSO::setAmplitude(uint8_t a) {
  amplitude = a;
}

void DSO::setPw(int32_t Pw) {
  pwm = Pw * HS / 128;
}

void DSO::update() {
  const float alpha = 0.6;
  static int32_t y = 0;
  const int a0 = alpha * 256;
  const int a1 = 256 - a0;

  PhaseAcc += PhaseInc;
  uint16_t Index = PhaseAcc >> 17;
  if (waveform) wave = (data[Index] + data[((Index) + pwm) & (S - 1)]);
  else wave = (data[Index] - data[((Index) + pwm) & (S - 1)]);
  y = (a0 * wave + a1 * y ) >> 8;
  wave = y;

  vdso = (wave * amplitude >> 8);
}
