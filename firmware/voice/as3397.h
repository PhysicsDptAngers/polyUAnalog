/**
 * @file as3397.h
 * @brief AS3397-based digitally controlled oscillator (DCO) and control voltage manager.
 * 
 * This class represents the interface with the AS3397 chip. 
 * For instance, the chip requires ten control voltages for configuration, 
 * which are generated using PWM signals from the RP2040 microcontroller. 
 * As a result, this class contains 10 (+4) PWM members. 
 * This is the most complex file to understand, as it is closely tied to certain hardware-specific aspects of the AS3397 chip.
 * 
 * @author M. Loumaigne, D. Guichaoua
 * @date 2023 - 2024
 * @version 4.33
 
 */

#ifndef AS3397_H
#define AS3397_H

#include <hardware/gpio.h>
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include "aspin.h"

#define PIDFIXE 1

const uint64_t srateFactor = 4294967296; /**< ? */
#if PIDFIXE == 1
const int64_t SCALE_FACTOR = 1 << 16;
#endif

/**
 * @struct CallbackData
 * @brief Structure for handling control data for the DCO (Digitally Controlled Oscillator).
 */
struct CallbackData {
  uint8_t WaveshapeFactor = 1; /**< Factor defining the waveshape. */
  long SetPoint = WaveshapeFactor * 2.5 / convFactor; /**< Setpoint for the waveshapper control voltage, initialized to 2.5V amplitude. convFactor is defined in aspin.h */
  long error = 0, errorSum = 0, output = 0; /**< PI control variables. */
#if PIDFIXE == 0
  float _Kp = 440 * Kp; /**< Proportional gain. */
  float _Ki = Ki / 440; /**< Integral gain. */
#else
  int64_t _Kp = 440 * Kp * SCALE_FACTOR; /**< Proportional gain with scaling factor. */
  int64_t _Ki = (Ki / 440) * SCALE_FACTOR; /**< Integral gain with scaling factor. */
#endif
  float noteFreq = 440; /**< Frequency of the note in Hz. */
  int64_t delay_us; /**< Delay in microseconds. */
  uint32_t sliceH; /**< High slice number for 2x8bits PWM DAC. */
  uint32_t sliceL; /**< Low slice number for 2x8bits PWM DAC. */
  uint32_t channelH; /**< High channel number 2x8bits for PWM DAC. */
  uint32_t channelL; /**< Low channel number 2x8bits for PWM DAC. */
  int8_t Pwm; /**< PWM value. */
  uint32_t PhaseAcc; /**< Phase accumulator for each callback. Once it reaches PhaseInc, the condensator is decharged */
  uint32_t PhaseInc; /**< Phase increment. Updated according to the frequency of the note and the callback frequence (see AUDIO_RATE in synth.h)*/
};

/**
 * @struct PWMsliceChannel
 * @brief Structure to define a PWM slice and its properties. This PWM signal is low-pass filtered in order to be used as a DAC.
 */
struct PWMsliceChannel {
  uint32_t slice; /**< PWM slice number. */
  uint32_t channel; /**< PWM channel number. */
  uint32_t dutyMax; /**< Maximum duty cycle. */
};

static struct CallbackData DcoA; /**< Global variable : ? */
static struct CallbackData DcoB;
static struct CallbackData Dso;

/**
 * @class As3397
 * @brief Class to manage the AS3397 module with "PWM-DAC" for the CV-control.
 */
class As3397 {
private:
  struct PWMsliceChannel VWFA_MSB_CV_sliceNum; /**< Control voltage for charging the capacitor ramp. This determines the waveform of the DCO via the waveshaper. DCO A, MSB 8-bit */
  struct PWMsliceChannel VWFA_LSB_CV_sliceNum; /**< Control voltage for charging the capacitor ramp. This determines the waveform of the DCO via the waveshaper. DCO A, LSB 8-bit */
  struct PWMsliceChannel VWFB_MSB_CV_sliceNum; /**< Control voltage for charging the capacitor ramp. This determines the waveform of the DCO via the waveshaper. DCO B, MSB 8-bit */
  struct PWMsliceChannel VWFB_LSB_CV_sliceNum; /**< Control voltage for charging the capacitor ramp. This determines the waveform of the DCO via the waveshaper. DCO B, LSB 8-bit */
  struct PWMsliceChannel DSO_MSB_CV_sliceNum; /**< Digital Signal Oscillator. MSB 8bit part of the audio waveform */
  struct PWMsliceChannel DSO_LSB_CV_sliceNum; /**< Digital Signal Oscillator. LSB 8bit part of the audio waveform */
  struct PWMsliceChannel DCOA_PW_CV_sliceNum; /**< Control voltage for Pulse Width of the square part of the DCO A oscillator*/
  struct PWMsliceChannel DCOB_PW_CV_sliceNum; /**< Control voltage for Pulse Width of the square part of the DCO B oscillator*/
  struct PWMsliceChannel BALANCE_CV_sliceNum; /**< Control voltage for the balance (DCO A vs DCO B) at the ouput of the DCO mixer*/
  struct PWMsliceChannel FILTER_FREQ_CV_sliceNum; /**< Control voltage for the cut-off frequency of the low pass filter (VCF) 12 bits value */
  struct PWMsliceChannel FILTER_RES_CV_sliceNum; /**< Control voltage for resonance of the low pass filter (VCF) 12 bits value */
  struct PWMsliceChannel MOD_AMOUNT_CV_sliceNum; /**< Control voltage for ? */
  struct PWMsliceChannel VCA_CV_sliceNum; /**< Control voltage for the output volume of the synth via the Voltage Controled Amplifier*/
  struct PWMsliceChannel PAN_CV_sliceNum; /**< Control voltage for the balance (left-right) at the ouput of the VCA*/
  uint32_t dcosrate; /**< ample rate at which the DCO phase is updated. This is concurrent with the "updateAudio" callback (see synth.h) at AUDIO_RATE.*/

public:
  /**
   * @brief Constructor for As3397. Filtered "PWM-DAC" are initialized and default waveform and frequency are set for DCOA and DCOB
   * @param srate Sample rate.
   */
  As3397(uint32_t srate);

  /**
   * @brief Configures a GPIO pin for "PWM-DAC" output.
   * @param gpioCV GPIO pin number.
   * @param resolution PWM resolution that also sets the resolution for the corresponding DAC
   * @return PWMsliceChannel structure containing the slice and channel.
   */
  struct PWMsliceChannel set_gpio_pwm(uint gpioCV, uint32_t resolution);

  /**
  * @brief Sets the frequency of DCO A.
  * 
  * @param freq The desired frequency in Hz.
  * @param RAZIntegrator If true, resets the phase accumulator.
  */
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

 /**
 * @brief Sets the wave selection via the quad mixer.
 * 
 * The quad mixer has two binary inputs, resulting in four possible states:
 * - **WAVE_AB**: Both DCO A and DCO B are active (+ square waves A and B).
 * - **WAVE_B**: Only DCO B is active, while DCO A is muted (+ square waves A and B).
 * - **WAVE_A**: Only DCO A is active, while DCO B is muted (+ square waves A and B).
 * - **WAVE_NONE**: Both DCOs are muted, **BUT** the square comparators still emit a square wave.  
 *   If the duty cycle is set to 0, the analog oscillators are muted, but the Digital Signal Oscillator (DSO) may still be active.
 * 
 * @param wave The wave selection index.
 */
  void set_Wave_Select(uint8_t wave);

/**
 * @brief Sets the waveshape factor for DCO A.
 * 
 * The voltage ramp at the charging capacitor is sent to the waveshaper.  
 * Depending on the maximum value of the ramp, different waveforms are generated,  
 * ranging from a sawtooth to a clipped triangle, passing through a pure triangle.  
 * 
 * Currently, four waveshapes have been implemented: saw, triangle, and clipped triangle.  
 * However, intermediate waveshapes or even waveshape modulation are entirely possible.
 * 
 * @param waveshape Waveshape factor.
 */
  void set_WaveshapeFactorDcoA(uint8_t waveshape);

  /**
   * @brief Sets the waveshape factor for DcoB. See set_WaveshapeFactorDcoA documentation
   * @param waveshape Waveshape factor.
   */
  void set_WaveshapeFactorDcoB(uint8_t waveshape);

  /**
   * @brief Updates the frequency of DcoA.
   */
  void updateDcoA();

  /**
   * @brief Updates the frequency of DcoB.
   */
  void updateDcoB();

  /**
   * @brief Sets the output waveform for DSO (Digital Signal Oscillator)
   * @param wave Is the 16 bit value of the sound generated by the DSO. It is sent to the two 8 bit "PWM-DAC" (low pas filtered PWM)
   */
  void DSO(int32_t wave);

  /**
   * @brief Updates the PID control loop for the maximum value of the DCO ramp (and consequently the waveshape at the output of the waveshapper)
   */
  void updatePID(void);
};

#endif