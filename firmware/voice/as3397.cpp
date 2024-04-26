#include "as3397.h"

As3397::As3397(uint32_t srate) {
  dcosrate = srate;

  this->VWFA_MSB_CV_sliceNum = set_gpio_pwm(VWFA_MSB_CV, PWMRes);
  this->VWFA_LSB_CV_sliceNum = set_gpio_pwm(VWFA_LSB_CV, PWMRes);
  this->VWFB_MSB_CV_sliceNum = set_gpio_pwm(VWFB_MSB_CV, PWMRes);
  this->VWFB_LSB_CV_sliceNum = set_gpio_pwm(VWFB_LSB_CV, PWMRes);
  this->DSO_MSB_CV_sliceNum = set_gpio_pwm(DSO_MSB, PWMRes);
  this->DSO_LSB_CV_sliceNum = set_gpio_pwm(DSO_LSB, PWMRes);
  this->DCOA_PW_CV_sliceNum = set_gpio_pwm(DCOA_PW_CV, PWMRes);
  this->DCOB_PW_CV_sliceNum = set_gpio_pwm(DCOB_PW_CV, PWMRes);
  this->BALANCE_CV_sliceNum = set_gpio_pwm(BALANCE_CV, PWMRes);
  this->MOD_AMOUNT_CV_sliceNum = set_gpio_pwm(MOD_AMOUNT_CV, PWMRes);
  this->FILTER_FREQ_CV_sliceNum = set_gpio_pwm(FILTER_FREQ_CV, PWMResFilter);
  this->FILTER_RES_CV_sliceNum = set_gpio_pwm(FILTER_RES_CV, PWMResFilter);
  this->VCA_CV_sliceNum = set_gpio_pwm(VCA_CV, PWMResFilter);
  this->PAN_CV_sliceNum = set_gpio_pwm(PAN_CV, PWMResFilter);

  gpio_init(DCOA_FREQ);
  gpio_set_dir(DCOA_FREQ, GPIO_OUT);
  gpio_init(DCOB_FREQ);
  gpio_set_dir(DCOB_FREQ, GPIO_OUT);
  gpio_init(GAMME_A);
  gpio_set_dir(GAMME_A, GPIO_OUT);
  gpio_init(GAMME_B);
  gpio_set_dir(GAMME_B, GPIO_OUT);
  gpio_init(WS_BIT0);
  gpio_set_dir(WS_BIT0, GPIO_OUT);
  gpio_init(WS_BIT1);
  gpio_set_dir(WS_BIT1, GPIO_OUT);

  // Select ADC input 0 (GPIO26) & 1
  adc_init();
  // Make sure GPIO is high-impedance, no pullups etc
  adc_gpio_init(INPUT_RAMPE_A + 26);
  adc_select_input(INPUT_RAMPE_A);
  adc_gpio_init(INPUT_RAMPE_B + 26);
  adc_select_input(INPUT_RAMPE_B);

  DcoA.sliceH = this->VWFA_MSB_CV_sliceNum.slice;
  DcoA.sliceL = this->VWFA_LSB_CV_sliceNum.slice;
  DcoB.sliceH = this->VWFB_MSB_CV_sliceNum.slice;
  DcoB.sliceL = this->VWFB_LSB_CV_sliceNum.slice;
  Dso.sliceH = this->DSO_MSB_CV_sliceNum.slice;
  Dso.sliceL = this->DSO_LSB_CV_sliceNum.slice;
  DcoA.channelH = this->VWFA_MSB_CV_sliceNum.channel;
  DcoA.channelL = this->VWFA_LSB_CV_sliceNum.channel;
  DcoB.channelH = this->VWFB_MSB_CV_sliceNum.channel;
  DcoB.channelL = this->VWFB_LSB_CV_sliceNum.channel;
  Dso.channelH = this->DSO_MSB_CV_sliceNum.channel;
  Dso.channelL = this->DSO_LSB_CV_sliceNum.channel;

  set_Wave_Select(WAVE_AB);
  set_DcoA_freq(440, true);
  set_DcoB_freq(440, true);
}

struct PWMsliceChannel As3397::set_gpio_pwm(uint gpioCV, uint32_t resolution) {
  struct PWMsliceChannel sliceChannel;
  sliceChannel.slice = pwm_gpio_to_slice_num(gpioCV);
  sliceChannel.channel = pwm_gpio_to_channel(gpioCV);
  gpio_set_function(gpioCV, GPIO_FUNC_PWM);
  sliceChannel.dutyMax = resolution - 1;
  pwm_set_wrap(sliceChannel.slice, sliceChannel.dutyMax);
  pwm_set_chan_level(sliceChannel.slice, sliceChannel.channel, 0);
  pwm_set_enabled(sliceChannel.slice, true);
  return sliceChannel;
}

// Fonction pour régler la fréquence de la note DCOA
void As3397::set_DcoA_freq(int32_t freq, bool RAZIntegrator) {

  static uint8_t factor;

  DcoA.noteFreq = freq;
  //DcoA.delay_us = 1E6 / freq;
  DcoA.PhaseInc = (uint32_t)(freq * srateFactor / dcosrate);

  if (freq > (1500 / DcoA.WaveshapeFactor)) {
    gpio_put(GAMME_A, true);
    factor = 1;
  } else if (freq < (1000 / DcoA.WaveshapeFactor)) {
    gpio_put(GAMME_A, false);
    factor = 32;
  }

  DcoA._Kp = freq * Kp * factor * SCALE_FACTOR;
  DcoA._Ki = (Ki / freq) * SCALE_FACTOR;

  if (RAZIntegrator) {
    // Réinitialise l'accumulateur d'erreur
    DcoA.errorSum = 0;
  }
}

// Fonction pour régler la fréquence de la note DCOB
void As3397::set_DcoB_freq(int32_t freq, bool RAZIntegrator) {

  static uint8_t factor;

  DcoB.noteFreq = freq;
  //DcoB.delay_us = 1E6 / freq;
  DcoB.PhaseInc = (uint32_t)(freq * srateFactor / dcosrate);

  if (freq > (1500 / DcoB.WaveshapeFactor)) {
    gpio_put(GAMME_B, true);
    factor = 1;
  } else if (freq < (1000 / DcoB.WaveshapeFactor)) {
    gpio_put(GAMME_B, false);
    factor = 32;
  }

  DcoB._Kp = freq * Kp * factor * SCALE_FACTOR;
  DcoB._Ki = (Ki / freq) * SCALE_FACTOR;

  if (RAZIntegrator) {
    // Réinitialise l'accumulateur d'erreur
    DcoB.errorSum = 0;
  }
}

void As3397::set_DcoFM(int FMmod) {
  DcoA.PhaseInc = (uint32_t)((DcoA.noteFreq + FMmod) * srateFactor / dcosrate);
  DcoB.PhaseInc = (uint32_t)((DcoB.noteFreq + FMmod) * srateFactor / dcosrate);
}


void As3397::set_DcoA_pw_cv(int8_t cv) {
  int32_t level = cv * DcoA.WaveshapeFactor / 2;
  if (level > this->DCOA_PW_CV_sliceNum.dutyMax) level = this->DCOA_PW_CV_sliceNum.dutyMax;
  else if (level <= 0) level = this->DCOA_PW_CV_sliceNum.dutyMax;
  pwm_set_chan_level(this->DCOA_PW_CV_sliceNum.slice, this->DCOA_PW_CV_sliceNum.channel, level);
  DcoA.Pwm = cv;
}

void As3397::set_DcoB_pw_cv(int8_t cv) {
  int32_t level = cv * DcoB.WaveshapeFactor / 2;
  if (level > this->DCOB_PW_CV_sliceNum.dutyMax) level = this->DCOB_PW_CV_sliceNum.dutyMax;
  else if (level <= 0) level = this->DCOB_PW_CV_sliceNum.dutyMax;
  pwm_set_chan_level(this->DCOB_PW_CV_sliceNum.slice, this->DCOB_PW_CV_sliceNum.channel, level);
  DcoB.Pwm = cv;
}

void As3397::set_Balance_cv(int32_t level) {
  if (level > this->BALANCE_CV_sliceNum.dutyMax) level = this->BALANCE_CV_sliceNum.dutyMax;
  else if (level < 0) level = 0;
  pwm_set_chan_level(this->BALANCE_CV_sliceNum.slice, this->BALANCE_CV_sliceNum.channel, level);
}

void As3397::set_Mod_amount_cv(int32_t level) {
  if (level > this->MOD_AMOUNT_CV_sliceNum.dutyMax) level = this->MOD_AMOUNT_CV_sliceNum.dutyMax;
  else if (level < 0) level = 0;
  pwm_set_chan_level(this->MOD_AMOUNT_CV_sliceNum.slice, this->MOD_AMOUNT_CV_sliceNum.channel, level);
}

void As3397::set_Filter_freq_cv(int32_t level) {
  level = this->FILTER_FREQ_CV_sliceNum.dutyMax - level;
  if (level > this->FILTER_FREQ_CV_sliceNum.dutyMax) level = this->FILTER_FREQ_CV_sliceNum.dutyMax;
  else if (level < 0) level = 0;
  pwm_set_chan_level(this->FILTER_FREQ_CV_sliceNum.slice, this->FILTER_FREQ_CV_sliceNum.channel, level);
}

void As3397::set_Filter_res_cv(int32_t level) {
  if (level > this->FILTER_RES_CV_sliceNum.dutyMax) level = this->FILTER_RES_CV_sliceNum.dutyMax;
  else if (level < 0) level = 0;
  pwm_set_chan_level(this->FILTER_RES_CV_sliceNum.slice, this->FILTER_RES_CV_sliceNum.channel, level);
}

void As3397::set_Vca_cv(int32_t level) {
  if (level > this->VCA_CV_sliceNum.dutyMax) level = this->VCA_CV_sliceNum.dutyMax;
  else if (level < 0) level = 0;
  pwm_set_chan_level(this->VCA_CV_sliceNum.slice, this->VCA_CV_sliceNum.channel, level);
}

void As3397::set_Pan_cv(int32_t level) {
  if (level > this->PAN_CV_sliceNum.dutyMax) level = this->PAN_CV_sliceNum.dutyMax;
  else if (level < 0) level = 0;
  pwm_set_chan_level(this->PAN_CV_sliceNum.slice, this->PAN_CV_sliceNum.channel, level);
}

void As3397::set_Wave_Select(uint8_t wave) {
  wave = wave & 3;
  switch (wave) {
    case WAVE_AB:
      gpio_put(WS_BIT0, false);
      gpio_put(WS_BIT1, false);
      break;
    case WAVE_B:
      gpio_put(WS_BIT0, true);
      gpio_put(WS_BIT1, false);
      break;
    case WAVE_NONE:
      gpio_put(WS_BIT0, false);
      gpio_put(WS_BIT1, true);
      break;
    case WAVE_A:
      gpio_put(WS_BIT0, true);
      gpio_put(WS_BIT1, true);
      break;
  }
}

void As3397::set_WaveshapeFactorDcoA(uint8_t waveshape) {
  if (waveshape > 4) waveshape = 4;
  else if (waveshape < 1) waveshape = 1;
  DcoA.WaveshapeFactor = waveshape;
  DcoA.SetPoint = DcoA.WaveshapeFactor * 2.5 / convFactor;
  set_DcoA_pw_cv(DcoA.Pwm);
}

void As3397::set_WaveshapeFactorDcoB(uint8_t waveshape) {
  if (waveshape > 4) waveshape = 4;
  else if (waveshape < 1) waveshape = 1;
  DcoB.WaveshapeFactor = waveshape;
  DcoB.SetPoint = DcoB.WaveshapeFactor * 2.5 / convFactor;
  set_DcoB_pw_cv(DcoB.Pwm);
}

void As3397::DSO(int32_t wave) {
  uint16_t wavea = wave + 32768;
  uint16_t Data_H = (wavea / PWMRes);
  uint16_t Data_L = (wavea & (PWMRes - 1));
  pwm_set_chan_level(Dso.sliceH, Dso.channelH, Data_H);
  pwm_set_chan_level(Dso.sliceL, Dso.channelL, Data_L);
}

void As3397::updateDcoA() {
  DcoA.PhaseAcc += DcoA.PhaseInc;
  if (DcoA.PhaseAcc >= DcoA.PhaseInc) return;

  adc_select_input(INPUT_RAMPE_A);
  uint16_t adcval = adc_read();  // Lecture de la valeur analogique max de la rampe

  gpio_put(DCOA_FREQ, true);  //décharge le condensateur de rampe

  DcoA.error = DcoA.SetPoint - adcval;  // calcul l'erreur pour l'asservissement de l'amplitude de la rampe
  DcoA.errorSum += DcoA.error; // Calcul de la fonction integral

  DcoA.output = (DcoA._Kp * DcoA.SetPoint + DcoA._Ki * DcoA.errorSum) / SCALE_FACTOR;  // Calcul de la sortie du PI

  if (DcoA.output < 0) DcoA.output = 0;  // Limite la sortie à 0-65535
  else if (DcoA.output > 65535) DcoA.output = 65535;

  // Calcule les données pour la sortie haute et basse du PWM
  uint16_t Data_H = (DcoA.output / PWMRes);
  uint16_t Data_L = (DcoA.output & (PWMRes - 1));

  // Écriture sur les broches de sortie du PWM
  // Les deux sorties PWM sont combinées pour former un DAC 16bits
  // Ceci commande le courant de charge du condensateur donc la pente de la rampe
  pwm_set_chan_level(DcoA.sliceH, DcoA.channelH, Data_H);
  pwm_set_chan_level(DcoA.sliceL, DcoA.channelL, Data_L);

  gpio_put(DCOA_FREQ, false);  // Début de charge du condensateur de la rampe
}

void As3397::updateDcoB() {
  DcoB.PhaseAcc += DcoB.PhaseInc;
  if (DcoB.PhaseAcc >= DcoB.PhaseInc) return;

  adc_select_input(INPUT_RAMPE_B);
  uint16_t adcval = adc_read();  // Lecture de la valeur analogique max de la rampe

  gpio_put(DCOB_FREQ, true);  //décharge le condensateur de rampe

  DcoB.error = DcoB.SetPoint - adcval;  // calcul l'erreur pour l'asservissement de l'amplitude de la rampe
  DcoB.errorSum += DcoB.error; // Calcul de la fonction integral

  DcoB.output = (DcoB._Kp * DcoB.SetPoint + DcoB._Ki * DcoB.errorSum) / SCALE_FACTOR;  // Calcul de la sortie du PI

  if (DcoB.output < 0) DcoB.output = 0;  // Limite la sortie à 0-65535
  else if (DcoB.output > 65535) DcoB.output = 65535;
  // Calcule les données pour la sortie haute et basse du PWM
  uint16_t Data_H = (DcoB.output / PWMRes);
  uint16_t Data_L = (DcoB.output & (PWMRes - 1));

  // Écriture sur les broches de sortie du PWM
  // Les deux sorties PWM sont combinées pour former un DAC 16bits
  // Ceci commande le courant de charge du condensateur donc la pente de la rampe
  pwm_set_chan_level(DcoB.sliceH, DcoB.channelH, Data_H);
  pwm_set_chan_level(DcoB.sliceL, DcoB.channelL, Data_L);

  gpio_put(DCOB_FREQ, false);  // Début de charge du condensateur de la rampe
}
