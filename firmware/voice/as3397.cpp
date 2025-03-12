#include "as3397.h"

As3397::As3397(uint32_t srate) {
  dcosrate = srate;

  this->PAN_CV_sliceNum = set_gpio_pwm(PAN_CV, PWMRes);
  this->MOD_AMOUNT_CV_sliceNum = set_gpio_pwm(MOD_AMOUNT_CV, PWMRes);

  this->DCOA_PW_CV_sliceNum = set_gpio_pwm(DCOA_PW_CV, PWMRes);
  this->DCOB_PW_CV_sliceNum = set_gpio_pwm(DCOB_PW_CV, PWMRes);

  this->BALANCE_CV_sliceNum = set_gpio_pwm(BALANCE_CV, PWMResVCA);
  this->VCA_CV_sliceNum = set_gpio_pwm(VCA_CV, PWMResVCA);

  this->FILTER_RES_CV_sliceNum = set_gpio_pwm(FILTER_RES_CV, PWMResFilter);
  this->FILTER_FREQ_CV_sliceNum = set_gpio_pwm(FILTER_FREQ_CV, PWMResFilter);

  this->VWFA_MSB_CV_sliceNum = set_gpio_pwm(VWFA_MSB_CV, PWMRes);
  this->VWFA_LSB_CV_sliceNum = set_gpio_pwm(VWFA_LSB_CV, PWMRes);

  this->VWFB_MSB_CV_sliceNum = set_gpio_pwm(VWFB_MSB_CV, PWMRes);
  this->VWFB_LSB_CV_sliceNum = set_gpio_pwm(VWFB_LSB_CV, PWMRes);
  
  this->DSO_MSB_CV_sliceNum = set_gpio_pwm(DSO_MSB, PWMRes);
  this->DSO_LSB_CV_sliceNum = set_gpio_pwm(DSO_LSB, PWMRes);


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

/**
 * @brief Sets the frequency of DCO A.
 * 
 * This function adjusts the frequency of the digitally controlled oscillator (DCO A).
 * It performs the following actions:
 * - Updates the phase increment based on the desired frequency.
 * - Manages frequency-dependent scaling using a MOSFET switch that modifies the resistance value,
 *   thereby adjusting the charging current of the DCO capacitor.
 * - Adjusts the proportional and integral control parameters for the PID feedback loop, which
 *   controls the maximum charging voltage of the DCO. This, in turn, influences the DCO waveform
 *   through the waveshaper.
 * - If requested, resets the integrator error accumulator of the PID controller.
 * 
 * @param freq The desired frequency in Hz.
 * @param RAZIntegrator If true, resets the integrator error accumulator.
 */
 void As3397::set_DcoA_freq(float freq, bool RAZIntegrator) {

  static uint8_t factor;

  DcoA.noteFreq = freq;

  // Compute phase increment based on desired frequency, dcosrate is fixed via a hardware timer of the rp2040 and
  // corresponds to a call to "updateAudio" (see synth.h) at AUDIO_RATE (typically 62.5 kHz)
  // srateFactor : ? software FM ?
  DcoA.PhaseInc = (uint32_t)(freq * srateFactor / dcosrate);

// Adjust the frequency scaling factor based on the note's frequency.
// A lower frequency results in a longer voltage ramp. Very low or very high frequencies may lose precision,
// so the voice board has two different charging current settings.
// The appropriate setting, based on the note frequency, is selected by opening or closing a MOSFET 
// transistor using gpio_put(GAMME_A, true/false);  
if (freq > (1500 / DcoA.WaveshapeFactor)) {
    gpio_put(GAMME_A, true);
    factor = 1;
  } else if (freq < (1000 / DcoA.WaveshapeFactor)) {
    gpio_put(GAMME_A, false);
    factor = 32;
  }

  // Set proportional and integral control gains
  #if PIDFIXE == 0
    DcoA._Kp = freq * Kp * factor;
    DcoA._Ki = Ki / freq;
  #else
    DcoA._Kp = freq * Kp * factor * SCALE_FACTOR;
    DcoA._Ki = (Ki / freq) * SCALE_FACTOR;
  #endif

  // Reset error accumulator if requested
  if (RAZIntegrator) {
    DcoA.errorSum = 0;
  }
}


/**
 * @brief Sets the frequency of DCO B.
 * 
 * See set_DcoA_freq documentation.
 * @param freq The desired frequency in Hz.
 * @param RAZIntegrator If true, resets the integrator error accumulator.
 */
void As3397::set_DcoB_freq(float freq, bool RAZIntegrator) {

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

#if PIDFIXE == 0
  DcoB._Kp = freq * Kp * factor;
  DcoB._Ki = Ki / freq;
#else
  DcoB._Kp = freq * Kp * factor * SCALE_FACTOR;
  DcoB._Ki = (Ki / freq) * SCALE_FACTOR;
#endif

  if (RAZIntegrator) {
    DcoB.errorSum = 0;
  }
}


void As3397::set_DcoFM(int FMmod) {
  DcoA.PhaseInc = (uint32_t)((DcoA.noteFreq + FMmod) * srateFactor / dcosrate);
  DcoB.PhaseInc = (uint32_t)((DcoB.noteFreq + FMmod) * srateFactor / dcosrate);
}

/**
 * @brief Sets the pulse width control voltage (PW CV) for DCO A.
 * 
 * This function adjusts the pulse width modulation (PWM) of the square part of DCO A  
 * based on the provided control voltage (CV). 
 * 
 * @param cv The control voltage value for pulse width adjustment.
 */
void As3397::set_DcoA_pw_cv(int32_t cv) {
  int32_t level = cv * DcoA.WaveshapeFactor;
  if (level > this->DCOA_PW_CV_sliceNum.dutyMax) level = this->DCOA_PW_CV_sliceNum.dutyMax;
  else if (level <= 0) level = this->DCOA_PW_CV_sliceNum.dutyMax;
  pwm_set_chan_level(this->DCOA_PW_CV_sliceNum.slice, this->DCOA_PW_CV_sliceNum.channel, level);
  DcoA.Pwm = cv;
}


void As3397::set_DcoB_pw_cv(int32_t cv) {
  int32_t level = cv * DcoB.WaveshapeFactor;
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

  // The quad mixer has two binary input that leads to four cases :
  switch (wave) {
    case WAVE_AB:
      gpio_put(WS_BIT0, false);
      gpio_put(WS_BIT1, false);
      break;
    case WAVE_B:
      gpio_put(WS_BIT0, true);
      gpio_put(WS_BIT1, false);
      break;
    case WAVE_A: //Normaly NONE
      gpio_put(WS_BIT0, false);
      gpio_put(WS_BIT1, true);
      break;
    case WAVE_NONE: //Normaly WAVE_A
      gpio_put(WS_BIT0, true);
      gpio_put(WS_BIT1, true);
      break;
  }
}

void As3397::set_WaveshapeFactorDcoA(uint8_t waveshape) {

  // Currently, four waveshapes have been implemented: saw, triangle, and clipped triangle.  
  // However, intermediate waveshapes or even waveshape modulation are entirely possible.
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

// Send the value outputed by the DSO to the dual two 8bits PWM-DAC 
void As3397::DSO(int32_t wave) {
  uint16_t wavea = wave + 32768;
  // Electronically speaking, the two signals—LSB and MSB—are mixed using a passive resistor mixer with a resistance ratio of 256 between the two resistors.
  uint16_t Data_H = (wavea / PWMRes);
  uint16_t Data_L = (wavea & (PWMRes - 1));
  pwm_set_chan_level(Dso.sliceH, Dso.channelH, Data_H);
  pwm_set_chan_level(Dso.sliceL, Dso.channelL, Data_L);
}



/**
 * @brief Updates the phase accumulator, discharges the DCO capacitor if needed,  
 * and applies PI control to the DCO A waveshape.
 * 
 * This function updates the phase accumulator for DCO A and applies  
 * a Proportional-Integral (PI) control loop to regulate the amplitude  
 * of the charging capacitor ramp. The PI controller adjusts the current  
 * flowing into the capacitor, ensuring proper waveform generation.  
 * 
 * The process includes:
 * - Updating the phase accumulator.
 * - If the phase accumulator exceeds `PhaseInc` (whose value depends on the desired frequency), then:
 *   - Reading the peak value of the analog ramp signal. Since the capacitor is about to discharge,  
 *     this corresponds to the maximum ramp voltage. This value, processed by the waveshaper,  
 *     defines the DCO waveform.
 *   - Discharging the capacitor by briefly activating the MOSFET transistor, which shorts the  
 *     capacitor to ground. The duration of this pulse is determined by the execution time  
 *     of the following instructions (i.e. in beetween "gpio_put(DCOA_FREQ, true);" and "gpio_put(DCOA_FREQ, false);"). It should be sufficient to fully discharge the capacitor,  
 *     which takes a few microseconds due to the MOSFET's residual resistance.
 *   - Computing the error for amplitude control. To maintain a consistent waveform across all frequencies,  
 *     the maximum ramp voltage must be dynamically adjusted by modifying the charging voltage.  
 *     This is controlled via the "Waveshape A" control voltage (`VWFA_MSB_CV_sliceNum` and  
 *     `VWFA_LSB_CV_sliceNum` variables).
 *   - Applying the PI controller to adjust the charging current.
 *   - Setting PWM-DAC values to regulate the capacitor charge rate.
 */
 void As3397::updateDcoA() {
  // Update the phase accumulator
  DcoA.PhaseAcc += DcoA.PhaseInc;
  if (DcoA.PhaseAcc >= DcoA.PhaseInc) return; // 

  // Select the ADC input channel corresponding to the ramp voltage
  adc_select_input(INPUT_RAMPE_A);
  uint16_t adcval = adc_read();  // Read the peak analog value of the ramp. It corresponds to the maximum voltage of the ramp since we are about to discharge the condensator

  // Discharge the capacitor (reset the ramp voltage). Electronically, this process takes a few µs.
  gpio_put(DCOA_FREQ, true);

  // Compute the amplitude control error (difference between setpoint and measured value)
  DcoA.error = DcoA.SetPoint - adcval;

  // Accumulate the error for integral control
  DcoA.errorSum += DcoA.error;

  // Compute the PI controller output, which determines the charging current
  #if PIDFIXE == 0
      DcoA.output = DcoA._Kp * DcoA.SetPoint + DcoA._Ki * DcoA.errorSum;
  #else
      DcoA.output = (DcoA._Kp * DcoA.SetPoint + DcoA._Ki * DcoA.errorSum) / SCALE_FACTOR;
  #endif

  // Limit the output value to the valid range (0 - 65535)
  if (DcoA.output < 0) DcoA.output = 0;
  else if (DcoA.output > 65535) DcoA.output = 65535;

  // Compute the high and low PWM values
  uint16_t Data_H = (DcoA.output / PWMRes);
  uint16_t Data_L = (DcoA.output & (PWMRes - 1));

  // Write to PWM-DAC output channels
  // The two PWM outputs are combined to form a 16-bit DAC,
  // which controls the capacitor charging current and, consequently, the ramp slope.
  pwm_set_chan_level(DcoA.sliceH, DcoA.channelH, Data_H);
  pwm_set_chan_level(DcoA.sliceL, DcoA.channelL, Data_L);

  // Start charging the capacitor (begin ramp cycle)
  gpio_put(DCOA_FREQ, false);
}

void As3397::updateDcoB() {
  // Update the phase accumulator
  DcoB.PhaseAcc += DcoB.PhaseInc;
  if (DcoB.PhaseAcc >= DcoB.PhaseInc) return;

  // Select the ADC input channel corresponding to the ramp voltage
  adc_select_input(INPUT_RAMPE_B);
  uint16_t adcval = adc_read();  // Read the peak analog value of the ramp

  // Discharge the capacitor (reset the ramp voltage)
  gpio_put(DCOB_FREQ, true);

  // Compute the amplitude control error (difference between setpoint and measured value)
  DcoB.error = DcoB.SetPoint - adcval;

  // Accumulate the error for integral control
  DcoB.errorSum += DcoB.error;

  // Compute the PI controller output, which determines the charging current
  #if PIDFIXE == 0
      DcoB.output = DcoB._Kp * DcoB.SetPoint + DcoB._Ki * DcoB.errorSum;
  #else
      DcoB.output = (DcoB._Kp * DcoB.SetPoint + DcoB._Ki * DcoB.errorSum) / SCALE_FACTOR;
  #endif

  // Limit the output value to the valid range (0 - 65535)
  if (DcoB.output < 0) DcoB.output = 0;
  else if (DcoB.output > 65535) DcoB.output = 65535;

  // Compute the high and low PWM values
  uint16_t Data_H = (DcoB.output / PWMRes);
  uint16_t Data_L = (DcoB.output & (PWMRes - 1));

  // Write to PWM output channels
  // The two PWM outputs are combined to form a 16-bit DAC,
  // which controls the capacitor charging current and, consequently, the ramp slope.
  pwm_set_chan_level(DcoB.sliceH, DcoB.channelH, Data_H);
  pwm_set_chan_level(DcoB.sliceL, DcoB.channelL, Data_L);

  // Start charging the capacitor (begin ramp cycle)
  gpio_put(DCOB_FREQ, false);
}
