/**
 * @file voice.ino
 * @brief Monophonic voice synthesizer for the PolyUAnalog project.
 * 
 * The "main" file that integrates all auxiliary files to fetch MIDI data from the I²C bus and send the correct voltage to the AS3397 chip.
 *
 * This file manages the initialization and real-time control of  
 * a polyphonic analog and open-source synthesizer, handling  
 * voice control, modulation, I2C communication, and LED status.
 * 
 * @author M. Loumaigne, D. Guichaoua
 * @date 2023 - 2024
 * @version 4.33
 * 
 * @section Features
 * - Fixes PWM_B and DSO bugs.
 * - Implements 14-bit PWM control for filter cutoff and resonance.
 * - Implements 12-bit PWM control for VCA and balance.
 * - Limits VCA output level to prevent saturation.
 * - Optimizes PI calculations.
 * 
 * @section TODO
 * - Implement one-byte control change functionality.
 * - Fix ADSR sustain bug.
 * - Add preset transfer via SysEx.
 * - Implement multitimbral support.
 */


/*

                           ,--,                                  ,---,                                     ,--,
   ,-.----.              ,--.'|                         ,--,    '  .' \                                  ,--.'|
   \    /  \     ,---.   |  | :                       ,'_ /|   /  ;    '.           ,---,                |  | :       ,---.
   |   :    |   '   ,'\  :  : '                  .--. |  | :  :  :       \      ,-+-. /  |               :  : '      '   ,'\    ,----._,.
   |   | .\ :  /   /   | |  ' |          .--,  ,'_ /| :  . |  :  |   /\   \    ,--.'|'   |    ,--.--.    |  ' |     /   /   |  /   /  ' /
   .   : |: | .   ; ,. : '  | |        /_ ./|  |  ' | |  . .  |  :  ' ;.   :  |   |  ,"' |   /       \   '  | |    .   ; ,. : |   :     |
   |   |  \ : '   | |: : |  | :     , ' , ' :  |  | ' |  | |  |  |  ;/  \   \ |   | /  | |  .--.  .-. |  |  | :    '   | |: : |   | .\  .
   |   : .  | '   | .; : '  : |__  /___/ \: |  :  | | :  ' ;  '  :  | \  \ ,' |   | |  | |   \__\/: . .  '  : |__  '   | .; : .   ; ';  |
   :     |`-' |   :    | |  | '.'|  .  \  ' |  |  ; ' |  | '  |  |  '  '--'   |   | |  |/    ," .--.; |  |  | '.'| |   :    | '   .   . |
   :   : :     \   \  /  ;  :    ;   \  ;   :  :  | : ;  ; |  |  :  :         |   | |--'    /  /  ,.  |  ;  :    ;  \   \  /   `---`-'| |
   |   | :      `----'   |  ,   /     \  \  ;  '  :  `--'   \ |  | ,'         |   |/       ;  :   .'   \ |  ,   /    `----'    .'__/\_: |
   `---'.|                ---`-'       :  \  \ :  ,      .-./ `--''           '---'        |  ,     .-./  ---`-'               |   :    :
     `---`                              \  ' ;  `--`----'                                   `--`---'                            \   \  /
                                         `--`                                                                                    `--`-'
  
    A Polyphonic Analogic and Open Source Synthesizer

    https://github.com/PhysicsDptAngers/polyUAnalog

    M. Loumaigne 
    D. Guichaoua

    2023 - 2024 - Université d'Angers

    Monophonic voices synthesizer

    v4.33
    - Fix PWM_B bug
    - Fix DSO Bug
    - Pass Filter FC and Res in 14 bits PWM
    - Pass VCA and Balance in 12 bits PWM
    - Limits VCA output level to prevent saturation
    - Optimize PI calculation

    To do
    - One byte control change possibility
    - Fix ADSR sustain bug
    - Adding preset transfert by SysEx
    - Adding multitimbral function

*/
#include "midicontrol.h"
#include <Wire.h>
#include "Synth.h"
#include "Midimanager.h"
#include "led.h"
#include <elapsedMillis.h>  // Include library for elapsed time measurement
#include "RPi_Pico_TimerInterrupt.h"

/** @def AUTOI2CADR
 *  @brief Enables automatic I2C address detection.
 */
#define AUTOI2CADR 1

/** @def MAXVOICES
 *  @brief Maximum number of synthesizer voices.
 */
#define MAXVOICES 16
#define I2CVoice 0

/** @def I2CclockFreq
 *  @brief I2C clock frequency in Hz.
 */
#define I2CclockFreq 400000

/** @def I2CAddressBase
 *  @brief Base address for I2C voice modules.
 */
#define I2CAddressBase 0x10
#define I2CVoie 0

/** @def BUFFER_SIZE
 *  @brief Size of the I2C communication buffer.
 */
#define BUFFER_SIZE 1024  // Taille du tampon I2C, ajustez-la en fonction de vos besoins

/** @brief I2C data buffer. */
byte buffer[BUFFER_SIZE];

/** @brief Write index for the I2C buffer. */
int bufferWriteIndex = 0;  

/** @brief Read index for the I2C buffer. */
int bufferReadIndex = 0;   

/** @brief I2C address for the synthesizer module. */
uint8_t I2CAddress;

/** @brief LED instance for status indication. */
LED led(LED_BUILTIN);
/** @brief Elapsed time for LED state management. */
elapsedMillis msec = 0;  

/** @brief Hardware Timer for control signal updates. */
RPI_PICO_Timer ITimer3(2);

/** @brief Hardware Timer for audio processing updates. */
RPI_PICO_Timer ITimer4(3);

/**
 * @brief Handles incoming I2C data.
 * 
 * This function reads data from the I2C buffer and stores it  
 * in a circular buffer for later processing.
 * 
 * @param bytesReceived Number of bytes received over I2C.
 */
void i2cReceive(int bytesReceived) {
  while (Wire.available()) {  // loop through all but the last
    int nextWriteIndex = (bufferWriteIndex + 1) % BUFFER_SIZE;
    if (nextWriteIndex != bufferReadIndex) {
      byte value = Wire.read();
      buffer[bufferWriteIndex] = value;
      bufferWriteIndex = nextWriteIndex;
    }
  }
  //led.Blink();
}


/**
 * @brief Handles I2C requests for the synthesizer's state.
 * 
 * This function sends the current ADSR envelope state  
 * over I2C to the master device.
 */
void i2cRequest() {
  // Renvoie le status de la voie à la carte orchestre (ATTACK, DECAY, SUSTAIN, RELEASE, NOTEOFF)
  Wire.write(eg1.getADSRState());
}

/**
 * @brief Initializes the synthesizer module.
 * 
 * This function configures GPIOs, ADC, I2C communication,  
 * waveform parameters, LFOs, envelopes, and real-time timers.
 */
void setup() {
  // put your setup code here, to run once:
  pinMode(LED_BUILTIN, OUTPUT);

  // Initialize ADC and read I2C address
  adc_init();
  adc_gpio_init(I2C_ADR_READ + 26);  // Set GPIO for ADC input
  adc_select_input(I2C_ADR_READ);
  uint16_t waiting = 4096 - adc_read();  // Read the I2C address from ADC input

  // Configure Wire library for I2C communication
  Wire.setSDA(20);              // Set SDA pin
  Wire.setSCL(21);              // Set SCL pin
  Wire.setClock(I2CclockFreq);  // Set I2C clock frequency

#if AUTOI2CADR == 0
  // Calculate and set I2C address based on configuration
  I2CAddress = I2CAddressBase + I2CVoice;
#else
  // Iterate over possible I2C addresses to scan for connected  over voices/modules
  // Find for free I2C addresses.
  // Delay depend of the card position
  delay(waiting / 10);
  delayMicroseconds((waiting % 10) * 100);
  // Initialize the Wire library for I2C communication
  Wire.begin();
  // Loop through each possible I2C address to scan
  for (int i = 0; i < MAXVOICES; i++) {
    // Calculate the I2C address based on the base address and current iteration
    I2CAddress = I2CAddressBase + i;
    // Begin an I2C transmission to the current address
    Wire.beginTransmission(I2CAddress);
    // End the I2C transmission and check for any errors
    byte error = Wire.endTransmission();
    // If the error code is 2 (NACK received), it means no device is present at this address, so stop scanning
    if (error == 2) {
      break;
    }
  }
  // End the I2C communication after scanning all addresses
  Wire.end();

#endif

  Wire.begin(I2CAddress);      // Begin I2C communication with given address
  Wire.onReceive(i2cReceive);  // Set function to handle I2C data reception
  Wire.onRequest(i2cRequest);  // Set function to handle I2C data request

  // Blink the built-in LED to indicate initialization
  for (int i = 0; i < (I2CAddress - I2CAddressBase + 1); i++) {
    led.Toggle();  // Blink the number of I2C ADR*
    delay(100);
    led.Toggle();
    delay(100);
  }

  waveformA = 0;
  pwmA = 64;
  transposeA = 0;
  detuneA = 0;
  waveformB = 0;
  pwmB = 64;
  transposeB = 0;
  detuneB = 10;
  transposeDSO = 0;
  detuneDSO = 0;
  balance = 128;
  Filter_freq = 4095;
  Filter_res = 0;
  mod_amount = 0;
  Pan = 128;

  as.set_DcoA_freq(220, false);
  as.set_DcoB_freq(222, false);
  as.set_DcoA_pw_cv(pwmA);
  as.set_DcoB_pw_cv(pwmB);
  as.set_Balance_cv(balance);
  as.set_Mod_amount_cv(mod_amount);
  as.set_Filter_freq_cv(Filter_freq);
  as.set_Filter_res_cv(Filter_res);
  as.set_Vca_cv(0);
  as.set_Pan_cv(128);
  as.set_Wave_Select(WAVE_AB);

  lfo1.setWaveform(0);
  lfo1.setFrequency(5);
  lfo1.setAmplitude(64);

  ITimer3.setInterval(1E6 / CONTROL_RATE, updateCtrl);
  ITimer4.setInterval(1E6 / AUDIO_RATE, updateAudio);

  Serial.begin(115200);
  //while (!Serial);
  Serial.print("I2C Adr : ");
  Serial.println(I2CAddress, HEX);
}

/**
 * @brief Initializes the secondary core setup (currently unused).
 */
void setup1() {
}

/**
 * @brief Main loop for handling LED blinking based on note state. verything else is managed by hardware timers (audio and control updates) or by core1 for processing I²C messages. 
 */
void loop() {
  static bool NoteState = false;
  Management of LED blinking (non-blocking)
  if (eg1.getADSRState() == NOTEOFF) {
    if (NoteState) {
      NoteState = false;
      led.Clr();
    }
  } else {
    if (!NoteState) {
      NoteState = true;
      led.Set();
    }
  }
  if (msec >= 50) {  // Check if 50 milliseconds have passed
    msec = 0;        // Reset the milliseconds counter
    led.Update();
  }
}

/**
 * @brief Secondary core loop for processing MIDI input via I²C bus from the dedicated buffer.
 */
void loop1() {
  // Check if the buffer contains data to process
  while (bufferReadIndex != bufferWriteIndex) {
    byte value = buffer[bufferReadIndex];
    OnMidi(value);                                          // Process each byte in the buffer  
    bufferReadIndex = (bufferReadIndex + 1) % BUFFER_SIZE;  // Increment the read index
  }
}
