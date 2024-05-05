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

    v4.31
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

#define AUTOI2CADR 1
#define MAXVOICES 16
#define I2CVoice 0
#define I2CclockFreq 400000
#define I2CAddressBase 0x10
#define I2CVoie 0
#define BUFFER_SIZE 1024  // Taille du tampon I2C, ajustez-la en fonction de vos besoins

byte buffer[BUFFER_SIZE];
int bufferWriteIndex = 0;  // Index d'écriture dans le tampon
int bufferReadIndex = 0;   // Index de lecture dans le tampon
uint8_t I2CAddress;

LED led(LED_BUILTIN);
elapsedMillis msec = 0;  // Elapsed time since the last LED state change

RPI_PICO_Timer ITimer3(2);
RPI_PICO_Timer ITimer4(3);


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

void i2cRequest() {
  // Renvoie le status de la voie à la carte orchestre (ATTACK, DECAY, SUSTAIN, RELEASE, NOTEOFF)
  Wire.write(eg1.getADSRState());
}


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

void setup1() {
}

void loop() {
  static bool NoteState = false;
  // gestion du blink de la led (non blocant)
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

void loop1() {
  // Vérifiez si le tampon contient des données à traiter
  while (bufferReadIndex != bufferWriteIndex) {
    byte value = buffer[bufferReadIndex];
    OnMidi(value);                                          // Traitez chaque octet du tampon
    bufferReadIndex = (bufferReadIndex + 1) % BUFFER_SIZE;  // Incrémente l'index de lecture
  }
}
