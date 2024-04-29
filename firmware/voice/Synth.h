#ifndef SYNTH_H
#define SYNTH_H

#include "as3397.h"
#include "LFO.h"
#include "ADSR.h"
#include "DSO.h"

byte CurMidiChannel = 0;

// use #define for CONTROL_RATE, not a constant
#define CONTROL_RATE 500  // Hz, powers of 2 are most reliable
// use #define for AUDIO_RATE, not a constant
#define AUDIO_RATE 31250  // Hz, powers of 2 are most reliable

#define BENDRANGE 12

As3397 as(AUDIO_RATE);
LFO lfo1(CONTROL_RATE);
ADSR eg1(CONTROL_RATE);
ADSR eg2(CONTROL_RATE);
DSO dso(AUDIO_RATE);

const float bendfactor = ((BENDRANGE * 100.0) / 8190.0);

//Variables Synthé
struct SYNTHVAR {
  uint8_t waveformA = WAVE_A;
  int8_t pwmA = 64;
  int8_t transposeA = 0;
  int8_t detuneA = 0;
  uint8_t waveshapeFactorA = 1;
  uint8_t waveformB = WAVE_B;
  int8_t pwmB = 64;
  int8_t transposeB = 0;
  int8_t detuneB = 10;
  uint8_t waveshapeFactorB = 1;
  uint8_t waveformDSO = 0;
  uint8_t AmplitudeDSO = 0;
  int8_t transposeDSO = 0;
  int8_t detuneDSO = 0;
  int8_t PwDSO = 64;
  int16_t balance = 128;
  int8_t noisemix = 0;
  int32_t Filter_freq = 4095;
  int8_t Filter_freqLow = 0;
  int32_t Filter_res = 0;
  int32_t Filter_key = 0;
  int32_t Filter_env = 0;
  int8_t mod_amount = 0;
  int32_t Pan = 128;

  uint8_t eg1Attack = 2;
  uint8_t eg1Decay = 10;
  uint8_t eg1Sustain = 30;
  uint8_t eg1Release = 100;
  uint8_t eg2Attack = 2;
  uint8_t eg2Decay = 10;
  uint8_t eg2Sustain = 30;
  uint8_t eg2Release = 25;

  uint8_t key = 69;
  int32_t freqA = 440;
  int32_t freqB = 440;
  int32_t freqDSO = 440;
  int32_t velocity = 64;
  int32_t volume = 64;
  int8_t GlbTranspose = 0;
  int8_t GlbDetune = 0;
  int16_t Glide = 0;

  int32_t bend = 0;

  uint8_t Lfo1Wave = 0;
  int32_t Lfo1Freq = 2;
  int32_t Lfo1ToPwmA = 0;
  int32_t Lfo1ToPwmB = 0;
  int32_t Lfo1ToPwmDso = 0;
  float Lfo1ToFreq = 0;
  int32_t Lfo1ToFilter = 0;
  int32_t Lfo1ToRes = 0;
  int32_t Lfo1ToPan = 32;

  int32_t VelToVca = 127;
  int32_t VelToFilter = 0;

  int32_t AFTToVca = 0;
  int32_t AFTToFilter = 0;

  int32_t keyToPan = 64;

  int32_t Eg2ToFreq = 0;

  bool RAZPid = true;
};

static struct SYNTHVAR synth[16];

void updateSynth(byte channel) {

  //synth[channel].RAZPid = true;

  as.set_Wave_Select(synth[channel].waveformA + synth[channel].waveformB);
  as.set_WaveshapeFactorDcoA(synth[channel].waveshapeFactorA);
  as.set_WaveshapeFactorDcoB(synth[channel].waveshapeFactorB);

  as.set_DcoA_pw_cv(synth[channel].pwmA);
  as.set_DcoB_pw_cv(synth[channel].pwmB);

  as.set_Balance_cv(synth[channel].balance);
  as.set_Mod_amount_cv(synth[channel].mod_amount);

  dso.setWaveform(synth[channel].waveformDSO);
  dso.setAmplitude(synth[channel].AmplitudeDSO);
  //dso.setPw(synth[channel].PwDSO);

  lfo1.setWaveform(synth[channel].Lfo1Wave);
  lfo1.setFrequency(synth[channel].Lfo1Freq);

  eg1.setAttack(synth[channel].eg1Attack);
  eg1.setDecay(synth[channel].eg1Decay);
  eg1.setSustain(synth[channel].eg1Sustain);
  eg1.setRelease(synth[channel].eg1Release);

  eg2.setAttack(synth[channel].eg2Attack);
  eg2.setDecay(synth[channel].eg2Decay);
  eg2.setSustain(synth[channel].eg2Sustain);
  eg2.setRelease(synth[channel].eg2Release);
}

bool updateCtrl(struct repeating_timer *t) {
  //rampe += 64;
  int32_t tmp;
  float FMmod;
  static float yA = 0;
  static float yB = 0;
  static float yD = 0;

  lfo1.update();
  eg1.update();
  eg2.update();

  as.set_Vca_cv(synth[CurMidiChannel].velocity * synth[CurMidiChannel].VelToVca * synth[CurMidiChannel].volume * eg1.veg >> 17);

  as.set_Filter_freq_cv(synth[CurMidiChannel].Filter_freq + (eg2.veg * synth[CurMidiChannel].Filter_env / 32) + ((synth[CurMidiChannel].key - 64) * synth[CurMidiChannel].Filter_key / 2)
                        + ((lfo1.vlfo * synth[CurMidiChannel].Lfo1ToFilter) / 16) + (synth[CurMidiChannel].velocity * synth[CurMidiChannel].VelToFilter / 32));
  as.set_Filter_res_cv(synth[CurMidiChannel].Filter_res + ((lfo1.vlfo * synth[CurMidiChannel].Lfo1ToRes) / 16));

  as.set_DcoA_pw_cv(synth[CurMidiChannel].pwmA + (lfo1.vlfo * synth[CurMidiChannel].Lfo1ToPwmA / 128));
  as.set_DcoB_pw_cv(synth[CurMidiChannel].pwmB + (lfo1.vlfo * synth[CurMidiChannel].Lfo1ToPwmB / 128));
  as.set_DcoB_pw_cv(synth[CurMidiChannel].PwDSO + (lfo1.vlfo * synth[CurMidiChannel].Lfo1ToPwmDso / 128));

  tmp = synth[CurMidiChannel].bend + (lfo1.vlfo * synth[CurMidiChannel].Lfo1ToFreq / 128) + (eg2.veg * synth[CurMidiChannel].Eg2ToFreq / 128) + synth[CurMidiChannel].GlbDetune;
  FMmod = pow(2, tmp / 1200.0);

  //Glide
  if (synth[CurMidiChannel].Glide) {
    yA += (synth[CurMidiChannel].freqA - yA) / synth[CurMidiChannel].Glide;
    yB += (synth[CurMidiChannel].freqB - yB) / synth[CurMidiChannel].Glide;
    yD += (synth[CurMidiChannel].freqDSO - yD) / synth[CurMidiChannel].Glide;
  } else {
    yA = synth[CurMidiChannel].freqA;
    yB = synth[CurMidiChannel].freqB;
    yD = synth[CurMidiChannel].freqDSO;
  }

  as.set_DcoA_freq(yA * FMmod, synth[CurMidiChannel].RAZPid);
  as.set_DcoB_freq(yB * FMmod, synth[CurMidiChannel].RAZPid);
  dso.setFrequency(yD * FMmod);

  as.set_Pan_cv(synth[CurMidiChannel].Pan + ((lfo1.vlfo * synth[CurMidiChannel].Lfo1ToPan) + ((synth[CurMidiChannel].key - 64) * synth[CurMidiChannel].keyToPan)) / 8);

  synth[CurMidiChannel].RAZPid = false;
  return true;
}

bool updateAudio(struct repeating_timer *t) {
  int value;
  as.updateDcoA();
  as.updateDcoB();
  dso.update();
  value = (rand() % 32767) * synth[CurMidiChannel].noisemix / 128;  // Générer une valeur aléatoire entre 0 et 255
  value += dso.vdso;
  as.DSO(value);
  return true;
}

#endif
