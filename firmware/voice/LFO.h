#ifndef LFO_H
#define LFO_H
#include <stdint.h>

enum Waveform { wsquare,
                wtriangle,
                wsaw,
                wrandomize,
                wnoise };

class LFO {
public:
  LFO(uint32_t rate);
  void setWaveform(uint8_t wave);
  void setFrequency(float frequency);
  void setAmplitude(uint8_t a);
  int next();
  void update();

  int8_t vlfo;

private:
  float freq;
  uint8_t amplitude;
  uint16_t PhaseAcc;
  uint16_t PhaseInc;
  uint32_t srate;
  float lfosrate;
  Waveform waveform;
  uint16_t pwm;
  uint8_t value;
  uint8_t front;
};

#endif
