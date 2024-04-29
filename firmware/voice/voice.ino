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

    v4.3
    - Fix PWM_B bug
    - Fix DSO Bug

    To do
    - Pass Filter FC and Res in 14 bits PWM
    - Pass VCA and Balance in 12 bits PWM
    - Optimize PI calculation
    - One byte control change possibility
    - Fix ADSR sustain bug
    - Adding preset transfert by SysEx
    - Adding multitimbral function

*/#include "midicontrol.h"
#include <Wire.h>
#include "as3397.h"
#include "LFO.h"
#include "ADSR.h"
#include "DSO.h"
#include "RPi_Pico_TimerInterrupt.h"

#define I2CclockFreq 400000
#define I2CAddressBase 0x10
#define I2CVoie 0
#define BUFFER_SIZE 1024  // Taille du tampon I2C, ajustez-la en fonction de vos besoins
byte buffer[BUFFER_SIZE];
int bufferWriteIndex = 0;  // Index d'écriture dans le tampon
int bufferReadIndex = 0;   // Index de lecture dans le tampon

// use #define for CONTROL_RATE, not a constant
#define CONTROL_RATE 500  // Hz, powers of 2 are most reliable
// use #define for AUDIO_RATE, not a constant
#define AUDIO_RATE 31250  // Hz, powers of 2 are most reliable

#define BENDRANGE 12

RPI_PICO_Timer ITimer3(2);
RPI_PICO_Timer ITimer4(3);

As3397 as(AUDIO_RATE);
LFO lfo1(CONTROL_RATE);
ADSR eg1(CONTROL_RATE);
ADSR eg2(CONTROL_RATE);
DSO dso(AUDIO_RATE);

uint8_t waveformA;
int8_t pwmA;
int8_t transposeA;
int8_t detuneA;
uint8_t waveformB;
int8_t pwmB;
int8_t transposeB;
int8_t detuneB;
int8_t transposeDSO;
int8_t detuneDSO;
int8_t PwDSO;
int16_t balance;
int8_t noisemix = 0;
int32_t Filter_freq = 4095;
int8_t Filter_freqHigh = 127;
int8_t Filter_freqLow = 127;
int32_t Filter_res;
int32_t Filter_key = 0;
int32_t Filter_env = 0;
int8_t mod_amount;
int32_t Pan;

uint8_t key;
int32_t freqA;
int32_t freqB;
int32_t freqDSO;
int32_t velocity;
int32_t volume = 64;
int8_t GlbTranspose = 0;
int8_t GlbDetune = 0;
int16_t Glide = 0;
const float bendfactor = ((BENDRANGE * 100.0) / 8190.0);
int32_t bend = 0;

int32_t Lfo1ToPwmA = 0;
int32_t Lfo1ToPwmB = 0;
int32_t Lfo1ToPwmDso = 0;
int32_t Lfo1ToFreq = 0;
int32_t Lfo1ToFilter = 0;
int32_t Lfo1ToRes = 0;
int32_t Lfo1ToPan = 32;

int32_t VelToVca = 127;
int32_t VelToFilter = 0;

int32_t AFTToVca = 0;
int32_t AFTToFilter = 0;

int32_t keyToPan = 64;

int32_t Eg2ToFreq;

uint16_t rampe = 0;

uint8_t I2CAddress;

bool RAZPid = true;



// DECLARATION POUR HANDLER MIDI
//------------------------------------------------
enum { MIDI_OTHER,
       MIDI_NOTE_OFF,
       MIDI_NOTE_ON,
       MIDI_CONTROL_CHANGE,
       MIDI_PROGRAM_CHANGE,
       MIDI_PITCH_BEND
};

int32_t midiToFreq(uint8_t midikey, int8_t transpose, int8_t detune) {
  int32_t freq = 8.1757989156 * pow(2.0, ((midikey + transpose + GlbTranspose) / 12.0) + ((detune) / 1200.0));
  return freq;
}

//---------------------
// GESTIONN MIDI IN
//---------------------
void OnMidi(uint8_t Midibyte) {
  static uint8_t midimode = MIDI_OTHER;
  static int8_t midibytesleft = -1;
  static uint8_t midibuffer[2];
  static byte MidiChannel;
  //------------
  // STATUS BYTE
  //------------
  if (Midibyte >= 0x80) {  // Byte de status si >127
    if (Midibyte <= 0xEF) {
      MidiChannel = (Midibyte & 0x0F);
      Midibyte = Midibyte & 0xF0;
    }
    switch (Midibyte) {
      // MIDI NOTE OFF
      case 0x80:
        midimode = MIDI_NOTE_OFF;
        midibytesleft = 2;
        handleNoteOff(MidiChannel, 0, 0);
        break;
      // MIDI NOTE ON
      case 0x90:
        midimode = MIDI_NOTE_ON;
        midibytesleft = 2;
        break;
      // MIDI CONTROL CHANGE
      case 0xB0:
        midimode = MIDI_CONTROL_CHANGE;
        midibytesleft = 2;
        break;
      // MIDI PITCH-BEND
      case 0xE0:
        midimode = MIDI_PITCH_BEND;
        midibytesleft = 2;
        break;
      default:
        midimode = MIDI_OTHER;
        midibytesleft = -1;
        break;
    }
  }
  //------------------------
  // SINON OCTETS DE DONNEES
  //------------------------
  else {
    // Stocker les octets a lire si besoin
    if (midibytesleft > 0) {          // Reste il des octets a lire ?
      midibuffer[0] = midibuffer[1];  // Oui, swap des octets
      midibuffer[1] = Midibyte;       // stockage octet courant
      midibytesleft--;                // un octet de moins a lire
    }
    // Traitement du message
    if (midibytesleft == 0) {
      //uint8_t note = midibuffer[0];
      midibytesleft = -1;
      switch (midimode) {
        case MIDI_NOTE_OFF:
          //handleNoteOff(MidiChannel, midibuffer[0], midibuffer[1]);
          break;
        case MIDI_NOTE_ON:
          // turn note off if velocity is zero
          if (midibuffer[1] == 0) {
            handleNoteOff(MidiChannel, midibuffer[0], midibuffer[1]);
          } else if (midibuffer[1] != 0) {
            handleNoteOn(MidiChannel, midibuffer[0], midibuffer[1]);
          }
          break;
        case MIDI_CONTROL_CHANGE:
          handleControlChange(MidiChannel, midibuffer[0], midibuffer[1]);
          break;
        case MIDI_PITCH_BEND:
          handlePitchBend(MidiChannel, midibuffer[0], midibuffer[1]);
          break;
        default:
          break;
      }
    }
  }
}

// Gestionnaire d'événements pour les messages MIDI Note On
void handleNoteOn(byte channel, byte pitch, byte vel) {
  key = pitch;
  velocity = vel;
  freqA = midiToFreq(key, transposeA, detuneA);
  freqB = midiToFreq(key, transposeB, detuneB);
  freqDSO = midiToFreq(key, transposeDSO, detuneDSO);
  RAZPid = true;
  eg1.gateOn();
  eg2.gateOn();
}

// Gestionnaire d'événements pour les messages MIDI Note Off
void handleNoteOff(byte channel, byte pitch, byte velocity) {
  eg1.gateOff();
  eg2.gateOff();
}

// Gestionnaire d'événements pour les messages MIDI CC
void handleControlChange(byte channel, byte controller, byte value) {
  switch (controller) {
    case OSC1WAVE:
      switch (value) {
        case 0:
          waveformA = WAVE_NONE;
          as.set_WaveshapeFactorDcoA(1);
          break;
        case 1:
          waveformA = WAVE_A;
          as.set_WaveshapeFactorDcoA(1);
          break;
        case 2:
          waveformA = WAVE_A;
          as.set_WaveshapeFactorDcoA(2);
          break;
        case 3:
          waveformA = WAVE_A;
          as.set_WaveshapeFactorDcoA(3);
          break;
        case 4:
          waveformA = WAVE_A;
          as.set_WaveshapeFactorDcoA(4);
          break;
      }
      as.set_Wave_Select(waveformA + waveformB);
      break;
    case OSC2WAVE:
      switch (value) {
        case 0:
          waveformB = WAVE_NONE;
          as.set_WaveshapeFactorDcoB(1);
          break;
        case 1:
          waveformB = WAVE_B;
          as.set_WaveshapeFactorDcoB(1);
          break;
        case 2:
          waveformB = WAVE_B;
          as.set_WaveshapeFactorDcoB(2);
          break;
        case 3:
          waveformB = WAVE_B;
          as.set_WaveshapeFactorDcoB(3);
          break;
        case 4:
          waveformB = WAVE_B;
          as.set_WaveshapeFactorDcoB(4);
          break;
      }
      as.set_Wave_Select(waveformA + waveformB);
      break;
    case OSC1DETUNE:
      detuneA = value - 64;
      freqA = midiToFreq(key, transposeA, detuneA);
      break;
    case OSC2DETUNE:
      detuneB = value - 64;
      freqB = midiToFreq(key, transposeB, detuneB);
      break;
    case OSC1TRANSPOSE:
      transposeA = value - 64;
      freqA = midiToFreq(key, transposeA, detuneA);
      break;
    case OSC2TRANSPOSE:
      transposeB = value - 64;
      freqB = midiToFreq(key, transposeB, detuneB);
      break;
    case OSC1PWM:
      pwmA = value;
      break;
    case OSC2PWM:
      pwmB = value;
      break;
    case BALANCE:
      as.set_Balance_cv(value << 1);
      break;
    case MIXNOISE:
      noisemix = value;
      break;
    case DSOWAVE:
      dso.setWaveform(value);
      break;
      uint16_t srate;
    case DSOTRANSPOSE:
      transposeDSO = value - 64;
      freqDSO = midiToFreq(key, transposeDSO, detuneDSO);
      dso.setFrequency(freqDSO);
      break;
    case DSODETUNE:
      detuneDSO = value - 64;
      freqDSO = midiToFreq(key, transposeDSO, detuneDSO);
      dso.setFrequency(freqDSO);
      break;
    case DSOMIX:
      dso.setAmplitude(value);
      break;
    case DSOPW:
      PwDSO = value;
      break;
    case FILTERFC:
      Filter_freq = (value << 7) + Filter_freqLow;
      break;
    case FILTERFCLOW:
      Filter_freqLow = value;
      break;
    case FILTERRES:
      Filter_res = value << 5;
      break;
    case FILTERKEY:
      Filter_key = value;
      break;
    case FILTERENV:
      Filter_env = value - 64;
      break;
    case MODAMOUNT:
      as.set_Mod_amount_cv(value << 1);
      break;
    case PAN:
      Pan = value << 1;
      //as.set_Pan_cv(Pan);
      break;
    case LFO1WAVE:
      lfo1.setWaveform(value);
      break;
    case LFO1FREQ:
      lfo1.setFrequency(value / 10.0);
      break;
    case LFO12PWA:
      Lfo1ToPwmA = value;
      break;
    case LFO12PWB:
      Lfo1ToPwmB = value;
      break;
    case LFO12FREQ:
      Lfo1ToFreq = value;
      break;
    case LFO12FILTER:
      Lfo1ToFilter = value;
      break;
    case LFO12PWDSO:
      Lfo1ToPwmDso = value;
      break;
    case LFO12PAN:
      Lfo1ToPan = value;
      break;
    case EG1ATTACK:
      eg1.setAttack(value);
      break;
    case EG1DECAY:
      eg1.setDecay(value);
      break;
    case EG1SUSTAIN:
      eg1.setSustain(value);
      break;
    case EG1RELEASE:
      eg1.setRelease(value);
      break;
    case EG2ATTACK:
      eg2.setAttack(value);
      break;
    case EG2DECAY:
      eg2.setDecay(value);
      break;
    case EG2SUSTAIN:
      eg2.setSustain(value);
      break;
    case EG2RELEASE:
      eg2.setRelease(value);
      break;
    case EG22FREQ:
      Eg2ToFreq = value;
      break;
    case VEL2VCA:
      VelToVca = value;
      break;
    case VEL2FILTER:
      VelToFilter = value;
      break;
    case AFT2VCA:
      AFTToVca = value;
      break;
    case AFT2FILTER:
      AFTToFilter = value;
      break;
    case KEY2PAN:
      keyToPan = value;
      break;
    case GLBDETUNE:
      GlbDetune = value - 64;
      break;
    case GLBTRANSPOSE:
      GlbTranspose = value - 64;
      break;
    case GLBVOLUME:
      volume = value;
      break;
    case GLBGLIDE:
      Glide = value;
      break;
    case ALLSOUNDOFF:
      eg1.soundOff();
      eg2.soundOff();
      break;
    case ALLNOTESOFF:
      eg1.gateOff();
      eg2.gateOff();
      break;
  }
}

// Gestionnaire d'événements pour les messages pitch Bend
void handlePitchBend(byte channel, byte value1, byte value2) {
  int bender = (value1 << 7) + value2;
  bend = bender * bendfactor;
}


bool blink = false;

void i2cReceive(int bytesReceived) {
  while (Wire.available()) {  // loop through all but the last
    int nextWriteIndex = (bufferWriteIndex + 1) % BUFFER_SIZE;
    if (nextWriteIndex != bufferReadIndex) {
      byte value = Wire.read();
      buffer[bufferWriteIndex] = value;
      bufferWriteIndex = nextWriteIndex;
    }
    blink = true;
  }
}

void i2cRequest() {
  // Renvoie le status de la voie à la carte orchestre (ATTACK, DECAY, SUSTAIN, RELEASE, NOTEOFF)
  Wire.write(eg1.getADSRState());
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

  as.set_Vca_cv(velocity * VelToVca * volume * eg1.veg / (128 * 128 * 128));

  as.set_Filter_freq_cv(Filter_freq + (eg2.veg * Filter_env / 32) + ((key - 64) * Filter_key / 2)
                        + ((lfo1.vlfo * Lfo1ToFilter) / 16) + (velocity * VelToFilter / 32));
  as.set_Filter_res_cv(Filter_res + ((lfo1.vlfo * Lfo1ToRes) / 16));

  as.set_DcoA_pw_cv(pwmA + (lfo1.vlfo * Lfo1ToPwmA / 128));
  as.set_DcoB_pw_cv(pwmB + (lfo1.vlfo * Lfo1ToPwmB / 128));

  dso.setPw(PwDSO + (lfo1.vlfo * Lfo1ToPwmDso / 128));
  
  tmp = bend + (lfo1.vlfo * Lfo1ToFreq / 128) + (eg2.veg * Eg2ToFreq / 128) + GlbDetune;
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


void setup() {
  // put your setup code here, to run once:
  pinMode(LED_BUILTIN, OUTPUT);

  // Select ADC input 0 (GPIO26) & 1
  adc_init();
  // Make sure GPIO is high-impedance, no pullups etc
  adc_gpio_init(I2C_ADR_READ + 26);
  adc_select_input(I2C_ADR_READ);
  uint16_t adcval = adc_read();            // Lecture de l'adresse I2C
  
  I2CAddress = I2CAddressBase + I2CVoie;

  Wire.setSDA(20);
  Wire.setSCL(21);
  Wire.setClock(I2CclockFreq);
  Wire.begin(I2CAddress);
  Wire.onReceive(i2cReceive);
  Wire.onRequest(i2cRequest);

  for (int i = 0; i < (I2CAddress - I2CAddressBase + 1); i++) {
    digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
    delay(250);                       // wait
    digitalWrite(LED_BUILTIN, LOW);   // turn the LED off by making the voltage LOW
    delay(250);                       // wait
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
  lfo1.setAmplitude(1);

  ITimer3.setInterval(1E6 / CONTROL_RATE, updateCtrl);

  Serial.begin(115200);
  //while (!Serial);
  Serial.print("I2C Adr : ");
  Serial.println(I2CAddress, HEX);
}

void setup1() {
  lfo1.setWaveform(0);
  lfo1.setFrequency(1);
  lfo1.setAmplitude(1);
  dso.setWaveform(0);

  ITimer4.setInterval(1E6 / AUDIO_RATE, updateAudio);
}

void loop() {
  if (eg1.getADSRState() == NOTEOFF) {
    digitalWrite(LED_BUILTIN, LOW);  // turn the LED off by making the voltage LOW
    if (blink) {
      digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
      delay(50);                        // wait
      digitalWrite(LED_BUILTIN, LOW);   // turn the LED off by making the voltage LOW
      blink = false;
      delay(50);
    }
  } else {
    digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
    if (blink) {
      digitalWrite(LED_BUILTIN, LOW);   // turn the LED off by making the voltage LOW
      delay(50);                        // wait
      digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
      blink = false;
      delay(50);
    }
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
