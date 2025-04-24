#include "as3397.h"
#include "discharge.pio.h"



/**
 * Convert a frequency to a MIDI note integer.
 * @param freq The frequency to convert.
 * @return The corresponding MIDI note as an integer.
 */
uint8_t freqToMidiInt(float freq) {
  return (uint8_t)round(69.0 + 12.0 * (log(freq / 440.0) / log(2.0)));
}

/**
 * Measure the slope of the captured ADC data.
 * This function is called as an interrupt service routine (ISR) when the DMA transfer is complete.
 */
void measure_slope() {
  long maxAB = 0;

  // Acknowledge the DMA interrupt request
  dma_channel_acknowledge_irq0(dma_chan_adc);

  // Stop the ADC and drain the FIFO to ensure all conversions are complete
  adc_run(false);
  adc_fifo_drain();

  // Find the maximum value in the captured buffer
  for (int i = 0; i < CAPTURE_DEPTH; i++) {
    if (capture_buf[i] > maxAB) maxAB = capture_buf[i];
  }

  if (CAPTURE_CHANNEL == 0) {
    DcoA.slope = maxAB;
    CAPTURE_CHANNEL = 1;
    adc_set_clkdiv(DcoB.Div);

    // Calculate the error for the ramp amplitude control
    DcoA.error = DcoA.SetPoint - DcoA.slope;
    if (DcoA.note > 30) DcoA.errorSum[DcoA.note] += DcoA.error;  // Integral function calculation
    else DcoA.errorSum[DcoA.note] = DcoA.errorSum[31];

      /*
    //Limiting PI error to limit windup effect
    if (DcoA.errorSum[DcoA.note] > ERROR_SUM_MAX)
      DcoA.errorSum[DcoA.note] = ERROR_SUM_MAX;
    else if (DcoA.errorSum[DcoA.note] < -ERROR_SUM_MAX)
      DcoA.errorSum[DcoA.note] = -ERROR_SUM_MAX;
*/


#if PIDFIXE == 0
    DcoA.output = DcoA._Kp * DcoA.SetPoint + DcoA._Ki * DcoA.errorSum[DcoA.note];  // PI output calculation
#else
    DcoA.output = (DcoA._Kp * DcoA.SetPoint + DcoA._Ki * DcoA.errorSum[DcoA.note]) / SCALE_FACTOR;  // PI output calculation
#endif

    // Limit the output to the range 0-65535
    if (DcoA.output < 0) DcoA.output = 0;
    else if (DcoA.output > 65535) DcoA.output = 65535;

    // Calculate the high and low PWM output data
    uint16_t Data_H = (DcoA.output / PWMRes);
    uint16_t Data_L = (DcoA.output & (PWMRes - 1));

    // Write to the PWM output pins
    // The two PWM outputs are combined to form a 16-bit DAC
    // This controls the charging current of the capacitor, hence the ramp slope
    pwm_set_chan_level(DcoA.sliceH, DcoA.channelH, Data_H);
    pwm_set_chan_level(DcoA.sliceL, DcoA.channelL, Data_L);

  } else {
    DcoB.slope = maxAB;
    CAPTURE_CHANNEL = 0;
    adc_set_clkdiv(DcoA.Div);

    // Calculate the error for the ramp amplitude control
    DcoB.error = DcoB.SetPoint - DcoB.slope;
    if (DcoB.note > 30) DcoB.errorSum[DcoB.note] += DcoB.error;  // Integral function calculation
    else DcoB.errorSum[DcoB.note] = DcoB.errorSum[31];

      /*
    //Limiting PI error to limit windup effect
    if (DcoB.errorSum[DcoB.note] > ERROR_SUM_MAX)
      DcoB.errorSum[DcoB.note] = ERROR_SUM_MAX;
    else if (DcoB.errorSum[DcoB.note] < -ERROR_SUM_MAX)
      DcoB.errorSum[DcoB.note] = -ERROR_SUM_MAX;
*/

#if PIDFIXE == 0
    DcoB.output = DcoB._Kp * DcoB.SetPoint + DcoB._Ki * DcoB.errorSum[DcoB.note];  // PI output calculation
#else
    DcoB.output = (DcoB._Kp * DcoB.SetPoint + DcoB._Ki * DcoB.errorSum[DcoB.note]) / SCALE_FACTOR;  // PI output calculation
#endif

    // Limit the output to the range 0-65535
    if (DcoB.output < 0) DcoB.output = 0;
    else if (DcoB.output > 65535) DcoB.output = 65535;

    // Calculate the high and low PWM output data
    uint16_t Data_H = (DcoB.output / PWMRes);
    uint16_t Data_L = (DcoB.output & (PWMRes - 1));

    // Write to the PWM output pins
    // The two PWM outputs are combined to form a 16-bit DAC
    // This controls the charging current of the capacitor, hence the ramp slope
    pwm_set_chan_level(DcoB.sliceH, DcoB.channelH, Data_H);
    pwm_set_chan_level(DcoB.sliceL, DcoB.channelL, Data_L);
  }

  // Reconfigure the DMA to start a new transfer
  adc_select_input(CAPTURE_CHANNEL);
  dma_channel_configure(dma_chan_adc, &cfg,
                        capture_buf,    // Destination buffer
                        &adc_hw->fifo,  // Source (ADC FIFO)
                        CAPTURE_DEPTH,  // Number of transfers
                        true            // Start immediately
  );
  adc_run(true);
}

/**
 * Constructor for the As3397 class.
 * Initializes GPIO pins, PWM slices, and sets up the ADC and DMA.
 * @param srate The sample rate for the DCOs.
 */
As3397::As3397(uint32_t srate) {
  dcosrate = srate;

  // Initialize PWM slices for various control voltages
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

  // Initialize GPIO pins for frequency and waveform selection
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

  // Initialize the ADC for ramp input
  //adc_init();
  adc_gpio_init(INPUT_RAMPE_A + 26);
  //adc_select_input(INPUT_RAMPE_A);
  adc_gpio_init(INPUT_RAMPE_B + 26);
  //adc_select_input(INPUT_RAMPE_B);

  // Assign PWM slices and channels for DCOs and DSO
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

  // Set initial waveform selection
  set_Wave_Select(WAVE_AB);

  // Add PIO program for discharge control
  this->offset = pio_add_program(pio0, &discharge_program);

  // Init first state machine for DCOA pin frequency control
  discharge_program_init(pio0, 0, offset, DCOA_FREQ);
  pio_sm_set_enabled(pio0, 0, true);
  pio_sm_set_clkdiv(pio0, 0, _div);

  // Init second state machine for DCOB pin frequency control
  discharge_program_init(pio0, 1, offset, DCOB_FREQ);
  pio_sm_set_enabled(pio0, 1, true);
  pio_sm_set_clkdiv(pio0, 1, _div);


  // Start and set DCOA frequency
  pio_sm_exec(pio0, 0, pio_encode_jmp(offset));
  pio0->txf[0] = F_CPU / (_div * MAXFREQ) - 34;

  // Start and set DCOA frequency
  pio_sm_exec(pio0, 1, pio_encode_jmp(offset));
  pio0->txf[1] = F_CPU / (_div * MAXFREQ) - 34;

  // Set initial frequencies for DCOs
  set_DcoA_freq(440);
  set_DcoB_freq(440);
}

/**
 * Set up a GPIO pin for PWM output.
 * @param gpioCV The GPIO pin to configure.
 * @param resolution The PWM resolution.
 * @return The configured PWM slice and channel.
 */
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
 * Set the frequency for DCO A.
 * @param freq The target frequency.
 */
void As3397::set_DcoA_freq(float freq) {
  static uint8_t factor;

  DcoA.noteFreq = freq;

  //Calculate the new frequency divider for PIO state machine
  DcoA.noteDiv = _div * MAXFREQ / freq;
  //set the new frequency divider for PIO state machine
  pio_sm_set_clkdiv(pio0, 0, DcoA.noteDiv);

  //adjuste variables for ADC dma
  DcoA.note = freq / 128;
  DcoA.Div = 48E6 / (freq * (CAPTURE_DEPTH - 25));

  if (freq > (1500 / DcoA.WaveshapeFactor)) {
    gpio_put(GAMME_A, true);
    factor = 1;
  } else if (freq < (1000 / DcoA.WaveshapeFactor)) {
    gpio_put(GAMME_A, false);
    factor = 32;
  }

#if PIDFIXE == 0
  DcoA._Kp = freq * Kp * factor;
  DcoA._Ki = Ki / freq;
#else
  DcoA._Kp = freq * Kp * factor * SCALE_FACTOR;
  DcoA._Ki = (Ki / freq) * SCALE_FACTOR;
#endif
}

/**
 * Set the frequency for DCO B.
 * @param freq The target frequency.
 */
void As3397::set_DcoB_freq(float freq) {
  static uint8_t factor;

  DcoB.noteFreq = freq;
  //Calculate the new frequency divider for PIO state machine
  DcoB.noteDiv = _div * MAXFREQ / freq;
  //set the new frequency divider for PIO state machine
  pio_sm_set_clkdiv(pio0, 1, DcoB.noteDiv);

  //adjuste variables for ADC dma
  DcoB.note = freq / 128;
  DcoB.Div = 48E6 / (freq * (CAPTURE_DEPTH - 25));



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
}

/**
 * Set the frequency modulation for the DCOs.
 * @param FMmod The modulation value.
 */
void As3397::set_DcoAFM(float FMmod) {
  // Implementation for setting frequency modulation
  pio_sm_set_clkdiv(pio0, 0, DcoA.noteDiv * FMmod);  // Modifie le diviseur dynamiquement
}

void As3397::set_DcoBFM(float FMmod) {
  // Implementation for setting frequency modulation
  pio_sm_set_clkdiv(pio0, 1, DcoB.noteDiv * FMmod);  // Modifie le diviseur dynamiquement
}

/**
 * Set the pulse width control voltage for DCO A.
 * @param cv The control voltage value.
 */
void As3397::set_DcoA_pw_cv(int32_t cv) {
  int32_t level = cv * DcoA.WaveshapeFactor;
  if (level > this->DCOA_PW_CV_sliceNum.dutyMax) level = this->DCOA_PW_CV_sliceNum.dutyMax;
  else if (level <= 0) level = 0;
  pwm_set_chan_level(this->DCOA_PW_CV_sliceNum.slice, this->DCOA_PW_CV_sliceNum.channel, level);
  DcoA.Pwm = cv;
}

/**
 * Set the pulse width control voltage for DCO B.
 * @param cv The control voltage value.
 */
void As3397::set_DcoB_pw_cv(int32_t cv) {
  int32_t level = cv * DcoB.WaveshapeFactor;
  if (level > this->DCOB_PW_CV_sliceNum.dutyMax) level = this->DCOB_PW_CV_sliceNum.dutyMax;
  else if (level <= 0) level = 0;
  pwm_set_chan_level(this->DCOB_PW_CV_sliceNum.slice, this->DCOB_PW_CV_sliceNum.channel, level);
  DcoB.Pwm = cv;
}

/**
 * Set the balance control voltage.
 * @param level The control voltage level.
 */
void As3397::set_Balance_cv(int32_t level) {
  if (level > this->BALANCE_CV_sliceNum.dutyMax) level = this->BALANCE_CV_sliceNum.dutyMax;
  else if (level < 0) level = 0;
  pwm_set_chan_level(this->BALANCE_CV_sliceNum.slice, this->BALANCE_CV_sliceNum.channel, level);
}

/**
 * Set the modulation amount control voltage.
 * @param level The control voltage level.
 */
void As3397::set_Mod_amount_cv(int32_t level) {
  if (level > this->MOD_AMOUNT_CV_sliceNum.dutyMax) level = this->MOD_AMOUNT_CV_sliceNum.dutyMax;
  else if (level < 0) level = 0;
  pwm_set_chan_level(this->MOD_AMOUNT_CV_sliceNum.slice, this->MOD_AMOUNT_CV_sliceNum.channel, level);
}

/**
 * Set the filter frequency control voltage.
 * @param level The control voltage level.
 */
void As3397::set_Filter_freq_cv(int32_t level) {
  level = this->FILTER_FREQ_CV_sliceNum.dutyMax - level;
  if (level > this->FILTER_FREQ_CV_sliceNum.dutyMax) level = this->FILTER_FREQ_CV_sliceNum.dutyMax;
  else if (level < 0) level = 0;
  pwm_set_chan_level(this->FILTER_FREQ_CV_sliceNum.slice, this->FILTER_FREQ_CV_sliceNum.channel, level);
}

/**
 * Set the filter resonance control voltage.
 * @param level The control voltage level.
 */
void As3397::set_Filter_res_cv(int32_t level) {
  if (level > this->FILTER_RES_CV_sliceNum.dutyMax) level = this->FILTER_RES_CV_sliceNum.dutyMax;
  else if (level < 0) level = 0;
  pwm_set_chan_level(this->FILTER_RES_CV_sliceNum.slice, this->FILTER_RES_CV_sliceNum.channel, level);
}

/**
 * Set the VCA control voltage.
 * @param level The control voltage level.
 */
void As3397::set_Vca_cv(int32_t level) {
  if (level > this->VCA_CV_sliceNum.dutyMax) level = this->VCA_CV_sliceNum.dutyMax;
  else if (level < 0) level = 0;
  pwm_set_chan_level(this->VCA_CV_sliceNum.slice, this->VCA_CV_sliceNum.channel, level);
}

/**
 * Set the pan control voltage.
 * @param level The control voltage level.
 */
void As3397::set_Pan_cv(int32_t level) {
  if (level > this->PAN_CV_sliceNum.dutyMax) level = this->PAN_CV_sliceNum.dutyMax;
  else if (level < 0) level = 0;
  pwm_set_chan_level(this->PAN_CV_sliceNum.slice, this->PAN_CV_sliceNum.channel, level);
}

/**
 * Set the waveform selection.
 * @param wave The waveform selection value.
 */
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
    case WAVE_NONE:  // Normally NONE
      gpio_put(WS_BIT0, false);
      gpio_put(WS_BIT1, true);
      break;
    case WAVE_A:  // Normally WAVE_A
      gpio_put(WS_BIT0, true);
      gpio_put(WS_BIT1, true);
      break;
  }
}

/**
 * Set the waveshape factor for DCO A.
 * @param waveshape The waveshape factor.
 */
void As3397::set_WaveshapeFactorDcoA(uint8_t waveshape) {
  if (waveshape > 4) waveshape = 4;
  else if (waveshape < 1) waveshape = 1;
  DcoA.WaveshapeFactor = waveshape;
  DcoA.SetPoint = DcoA.WaveshapeFactor * 2.5 / convFactor;
  set_DcoA_pw_cv(DcoA.Pwm);
}

/**
 * Set the waveshape factor for DCO B.
 * @param waveshape The waveshape factor.
 */
void As3397::set_WaveshapeFactorDcoB(uint8_t waveshape) {
  if (waveshape > 4) waveshape = 4;
  else if (waveshape < 1) waveshape = 1;
  DcoB.WaveshapeFactor = waveshape;
  DcoB.SetPoint = DcoB.WaveshapeFactor * 2.5 / convFactor;
  set_DcoB_pw_cv(DcoB.Pwm);
}

/**
 * Set the digital sympathetic oscillator (DSO) waveform.
 * @param wave The waveform value.
 */
void As3397::DSO(int32_t wave) {
  uint16_t wavea = wave + 32768;
  uint16_t Data_H = (wavea / PWMRes);
  uint16_t Data_L = (wavea & (PWMRes - 1));
  pwm_set_chan_level(Dso.sliceH, Dso.channelH, Data_H);
  pwm_set_chan_level(Dso.sliceL, Dso.channelL, Data_L);
}

/**
 * Initialize the ADC and DMA for capturing data.
 */
void As3397::AdcDma_init() {
  // Initialize GPIO pins for analog input
  adc_gpio_init(26 + INPUT_RAMPE_A);
  adc_gpio_init(26 + INPUT_RAMPE_B);

  adc_init();

  // Configure the ADC FIFO to use DMA
  adc_fifo_setup(
    true,   // Write each completed conversion to the sample FIFO
    true,   // Enable DMA data request (DREQ)
    1,      // DREQ asserted when at least 1 sample is present
    false,  // Disable error bit
    false   // Keep samples as 12-bit values
  );

  // Set the ADC clock divider
  adc_set_clkdiv(48E6 / (440.0 * (CAPTURE_DEPTH - 10)));

  // Claim a DMA channel and configure it
  dma_chan_adc = dma_claim_unused_channel(true);
  cfg = dma_channel_get_default_config(dma_chan_adc);

  // Configure the DMA channel for reading from the ADC FIFO
  channel_config_set_transfer_data_size(&cfg, DMA_SIZE_16);
  channel_config_set_read_increment(&cfg, false);
  channel_config_set_write_increment(&cfg, true);
  channel_config_set_dreq(&cfg, DREQ_ADC);

  // Set the starting ADC channel and configure the DMA transfer
  adc_select_input(CAPTURE_CHANNEL);
  dma_channel_configure(dma_chan_adc, &cfg,
                        capture_buf,    // Destination buffer
                        &adc_hw->fifo,  // Source (ADC FIFO)
                        CAPTURE_DEPTH,  // Number of transfersm
                        true            // Start immediately
  );

  // Enable DMA interrupts and set the interrupt handler
  dma_channel_set_irq0_enabled(dma_chan_adc, true);
  irq_set_exclusive_handler(DMA_IRQ_0, measure_slope);
  irq_set_enabled(DMA_IRQ_0, true);

  // Ensure the DMA interrupt is not quiet (disabled)
  channel_config_set_irq_quiet(&cfg, false);

  // Start the ADC
  adc_run(true);
}

