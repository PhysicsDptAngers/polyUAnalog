#ifndef AS3397_H
#define AS3397_H

#include <hardware/gpio.h>
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include "aspin.h"

#define PIDFIXE 1

const uint64_t srateFactor = 4294967296;
#if PIDFIXE == 1
const int64_t SCALE_FACTOR = 1 << 16;
#endif

struct CallbackData {
  uint8_t WaveshapeFactor = 1;
  long SetPoint = WaveshapeFactor * 2.5 / convFactor;  // Initialise la consigne de l'asservissement à une amplitude de 2.5V
  long error = 0, errorSum = 0, output = 0;            //Variables de la commande PI
#if PIDFIXE == 0
  float _Kp = 440 * Kp;
  float _Ki = Ki / 440;
#else
  int64_t _Kp = 440 * Kp * SCALE_FACTOR;
  int64_t _Ki = (Ki / 440) * SCALE_FACTOR;
#endif
  float noteFreq = 440;
  int64_t delay_us;
  uint32_t sliceH;
  uint32_t sliceL;
  uint32_t channelH;
  uint32_t channelL;
  int8_t Pwm;
  uint32_t PhaseAcc;
  uint32_t PhaseInc;
};

struct PWMsliceChannel {
  uint32_t slice;
  uint32_t channel;
  uint32_t dutyMax;
};


static struct CallbackData DcoA;
static struct CallbackData DcoB;
static struct CallbackData Dso;


class As3397 {
private:
  struct PWMsliceChannel VWFA_MSB_CV_sliceNum;
  struct PWMsliceChannel VWFA_LSB_CV_sliceNum;
  struct PWMsliceChannel VWFB_MSB_CV_sliceNum;
  struct PWMsliceChannel VWFB_LSB_CV_sliceNum;
  struct PWMsliceChannel DSO_MSB_CV_sliceNum;
  struct PWMsliceChannel DSO_LSB_CV_sliceNum;
  struct PWMsliceChannel DCOA_PW_CV_sliceNum;
  struct PWMsliceChannel DCOB_PW_CV_sliceNum;
  struct PWMsliceChannel BALANCE_CV_sliceNum;
  struct PWMsliceChannel FILTER_FREQ_CV_sliceNum;
  struct PWMsliceChannel FILTER_RES_CV_sliceNum;
  struct PWMsliceChannel MOD_AMOUNT_CV_sliceNum;
  struct PWMsliceChannel VCA_CV_sliceNum;
  struct PWMsliceChannel PAN_CV_sliceNum;
  uint32_t dcosrate;


public:
  As3397(uint32_t srate);
  struct PWMsliceChannel set_gpio_pwm(uint gpioCV, uint32_t resolution);
  void set_DcoA_freq(float freq, bool RAZIntegrator);
  void set_DcoB_freq(float freq, bool RAZIntegrator);
  void set_DcoFM(int FMmod);
  void set_DcoA_pw_cv(int32_t level);
  void set_DcoB_pw_cv(int32_t level);
  void set_Balance_cv(int32_t level);
  void set_Mod_amount_cv(int32_t level);
  void set_Filter_freq_cv(int32_t level);
  void set_Filter_res_cv(int32_t level);
  void set_Vca_cv(int32_t level);
  void set_Pan_cv(int32_t level);

  void set_Wave_Select(uint8_t wave);
  void set_WaveshapeFactorDcoA(uint8_t waveshape);
  void set_WaveshapeFactorDcoB(uint8_t waveshape);

  void updateDcoA();
  void updateDcoB();

  void DSO(int32_t wave);

  void updatePID(void);
};

#endif
