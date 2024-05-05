#ifndef SYNTH_H
#define SYNTH_H

#include "as3397.h"
#include "LFO.h"
#include "ADSR.h"
#include "DSO.h"

// use #define for CONTROL_RATE, not a constant
#define CONTROL_RATE 500  // Hz, powers of 2 are most reliable
// use #define for AUDIO_RATE, not a constant
#define AUDIO_RATE 62500  // Hz, powers of 2 are most reliable

#define BENDRANGE 12
const float bendfactor = ((BENDRANGE * 100.0) / 8190.0);

As3397 as(AUDIO_RATE);
LFO lfo1(CONTROL_RATE);
ADSR eg1(CONTROL_RATE);
ADSR eg2(CONTROL_RATE);
DSO dso(AUDIO_RATE);

uint8_t waveformA = 1;
int8_t pwmA = 0;
int8_t transposeA = 64;
int8_t detuneA = 64;
uint8_t waveformB = 1;
int8_t pwmB = 0;
int8_t transposeB = 64;
int8_t detuneB = 64;
int8_t transposeDSO = 64;
int8_t detuneDSO = 64;
int8_t PwDSO = 0;
int16_t balance = 64;
int8_t noisemix = 0;
int32_t Filter_freq = 4095;
int8_t Filter_freqHigh = 127;
int8_t Filter_freqLow = 127;
int32_t Filter_res = 32;
int32_t Filter_key = 0;
int32_t Filter_env = 0;
int8_t mod_amount = 0;
int32_t Pan = 64;

uint8_t key;
float freqA;
float freqB;
float freqDSO;
int64_t velocity;
int64_t volume = 64;
int8_t GlbTranspose = 0;
int8_t GlbDetune = 0;
int16_t Glide = 0;

int32_t bend = 0;

int32_t Lfo1ToPwmA = 0;
int32_t Lfo1ToPwmB = 0;
int32_t Lfo1ToPwmDso = 0;
int32_t Lfo1ToFreq = 0;
int32_t Lfo1ToFilter = 0;
int32_t Lfo1ToRes = 0;
int32_t Lfo1ToPan = 32;

int64_t VelToVca = 127;
int32_t VelToFilter = 0;

int32_t AFTToVca = 0;
int32_t AFTToFilter = 0;

int32_t keyToPan = 64;

int32_t Eg2ToFreq = 0;

bool RAZPid = true;

bool updateCtrl(struct repeating_timer *t) {
  int32_t tmp;
  float FMmod;
  static float yA = 0;
  static float yB = 0;
  static float yD = 0;

  lfo1.update();
  eg1.update();
  eg2.update();

  //Limits VCA output level to prevent saturation
  as.set_Vca_cv(velocity * VelToVca * volume * eg1.veg_f / (128 * 128 * 128));

  as.set_Filter_freq_cv(Filter_freq + (eg2.veg_a * Filter_env / 8) + ((key - 64) * Filter_key * 2)
                        + ((lfo1.vlfo * Lfo1ToFilter) / 4) + (velocity * VelToFilter / 8));
  as.set_Filter_res_cv(Filter_res + ((lfo1.vlfo * Lfo1ToRes) / 4));

  as.set_DcoA_pw_cv(pwmA + (lfo1.vlfo * Lfo1ToPwmA / 128));
  as.set_DcoB_pw_cv(pwmB + (lfo1.vlfo * Lfo1ToPwmB / 128));

  dso.setPw(PwDSO + (lfo1.vlfo * Lfo1ToPwmDso / 128));
  
  tmp = bend + (lfo1.vlfo * Lfo1ToFreq / 128) + (eg2.veg_a * Eg2ToFreq / 128) + GlbDetune;
  FMmod = pow(2, tmp / 1200.0);

  //Glide
  if (Glide) {
    yA += (freqA - yA) / Glide;
    yB += (freqB - yB) / Glide;
    yD += (freqDSO - yD) / Glide;
  }
  else {
    yA = freqA;
    yB = freqB;
    yD = freqDSO;
  }

  as.set_DcoA_freq(yA * FMmod, RAZPid);
  as.set_DcoB_freq(yB * FMmod, RAZPid);
  dso.setFrequency(yD * FMmod);

  as.set_Pan_cv(Pan + (lfo1.vlfo * Lfo1ToPan / 128) + ((key - 64) * keyToPan / 128));

  RAZPid = false;
  return true;
}

bool updateAudio(struct repeating_timer *t) {
  int value;
  as.updateDcoA();
  as.updateDcoB();
  dso.update();
  value = (rand() % 32767) * noisemix / 128;  // Générer une valeur aléatoire entre 0 et 255
  value += dso.vdso;
  as.DSO(value);
  return true;
}



#endif