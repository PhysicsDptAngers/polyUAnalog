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

    v4.5
    - Adding multitimbral function
    - One byte control change possibility
    - Optimize PI calculation

    To do
    - Fixe ADSR sustain bug
    - Adding preset transfert by SysEx

*/

#include "midicontrol.h"
#include <Wire.h>
#include "Synth.h"
#include "RPi_Pico_TimerInterrupt.h"

#define I2CclockFreq 400000
#define I2CAddressBase 0x10
#define I2CVoie 0
#define BUFFER_SIZE 1024  // Taille du tampon I2C, ajustez-la en fonction de vos besoins
byte buffer[BUFFER_SIZE];
int bufferWriteIndex = 0;  // Index d'écriture dans le tampon
int bufferReadIndex = 0;   // Index de lecture dans le tampon
uint8_t I2CAddress;

RPI_PICO_Timer ITimer3(2);
RPI_PICO_Timer ITimer4(3);

// DECLARATION POUR HANDLER MIDI
//------------------------------------------------
enum { MIDI_OTHER,
       MIDI_NOTE_OFF,
       MIDI_NOTE_ON,
       MIDI_CONTROL_CHANGE,
       MIDI_PROGRAM_CHANGE,
       MIDI_PITCH_BEND
};

int32_t midiToFreq(uint8_t midikey, int8_t transpose, int8_t detune, int8_t GlbTranspose) {
  int32_t freq = 8.1757989156 * pow(2.0, ((midikey + transpose + GlbTranspose) / 12.0) + ((detune) / 1200.0));
  return freq;
}

//---------------------
// GESTION MIDI IN
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
    } else if (midibytesleft < 0) {
      if (midimode == MIDI_CONTROL_CHANGE) handleControlChange(MidiChannel, midibuffer[0], Midibyte);
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

  synth[channel].key = pitch;
  synth[channel].velocity = vel;
  synth[channel].freqA = midiToFreq(synth[channel].key, synth[channel].transposeA, synth[channel].detuneA, synth[channel].GlbTranspose);
  synth[channel].freqB = midiToFreq(synth[channel].key, synth[channel].transposeB, synth[channel].detuneB, synth[channel].GlbTranspose);
  synth[channel].freqDSO = midiToFreq(synth[channel].key, synth[channel].transposeDSO, synth[channel].detuneDSO, synth[channel].GlbTranspose);
  synth[channel].RAZPid = true;

  if (channel != CurMidiChannel) {
    CurMidiChannel = channel;
    eg1.soundOff();
    eg2.soundOff();
    updateSynth(channel);
  }
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
  bool up = false;
  if (channel == CurMidiChannel) up = true;

  switch (controller) {
    case OSC1WAVE:
      switch (value) {
        case 0:
          synth[channel].waveformA = WAVE_NONE;
          synth[channel].waveshapeFactorA = 1;
          break;
        case 1:
          synth[channel].waveformA = WAVE_A;
          synth[channel].waveshapeFactorA = 1;
          break;
        case 2:
          synth[channel].waveformA = WAVE_A;
          synth[channel].waveshapeFactorA = 2;
          break;
        case 3:
          synth[channel].waveformA = WAVE_A;
          synth[channel].waveshapeFactorA = 3;
          break;
        case 4:
          synth[channel].waveformA = WAVE_A;
          synth[channel].waveshapeFactorA = 4;
          break;
      }
      break;
    case OSC2WAVE:
      switch (value) {
        case 0:
          synth[channel].waveformB = WAVE_NONE;
          synth[channel].waveshapeFactorB = 1;
          break;
        case 1:
          synth[channel].waveformB = WAVE_B;
          synth[channel].waveshapeFactorB = 1;
          break;
        case 2:
          synth[channel].waveformB = WAVE_B;
          synth[channel].waveshapeFactorB = 2;
          break;
        case 3:
          synth[channel].waveformB = WAVE_B;
          synth[channel].waveshapeFactorB = 3;
          break;
        case 4:
          synth[channel].waveformB = WAVE_B;
          synth[channel].waveshapeFactorB = 4;
          break;
      }
      break;
    case OSC1DETUNE:
      synth[channel].detuneA = value - 64;
      synth[channel].freqA = midiToFreq(synth[channel].key, synth[channel].transposeA, synth[channel].detuneA, synth[channel].GlbTranspose);
      break;
    case OSC2DETUNE:
      synth[channel].detuneB = value - 64;
      synth[channel].freqB = midiToFreq(synth[channel].key, synth[channel].transposeB, synth[channel].detuneB, synth[channel].GlbTranspose);
      break;
    case OSC1TRANSPOSE:
      synth[channel].transposeA = value - 64;
      synth[channel].freqA = midiToFreq(synth[channel].key, synth[channel].transposeA, synth[channel].detuneA, synth[channel].GlbTranspose);
      break;
    case OSC2TRANSPOSE:
      synth[channel].transposeB = value - 64;
      synth[channel].freqB = midiToFreq(synth[channel].key, synth[channel].transposeB, synth[channel].detuneB, synth[channel].GlbTranspose);
      break;
    case OSC1PWM:
      synth[channel].pwmA = value;
      break;
    case OSC2PWM:
      synth[channel].pwmB = value;
      break;
    case BALANCE:
      synth[channel].balance = value << 1;
      break;
    case MIXNOISE:
      synth[channel].noisemix = value;
      break;
    case DSOWAVE:
      synth[channel].waveformDSO = value;
      break;
    case DSOTRANSPOSE:
      synth[channel].transposeDSO = value - 64;
      synth[channel].freqDSO = midiToFreq(synth[channel].key, synth[channel].transposeDSO, synth[channel].detuneDSO, synth[channel].GlbTranspose);
      break;
    case DSODETUNE:
      synth[channel].detuneDSO = value - 64;
      synth[channel].freqDSO = midiToFreq(synth[channel].key, synth[channel].transposeDSO, synth[channel].detuneDSO, synth[channel].GlbTranspose);
      break;
    case DSOMIX:
      synth[channel].AmplitudeDSO = value;
      break;
    case DSOPW:
      synth[channel].PwDSO = value - 64;
      break;
    case FILTERFC:
      synth[channel].Filter_freq = (value << 7) + synth[channel].Filter_freqLow;
      break;
    case FILTERFCLOW:
      synth[channel].Filter_freqLow = value;
      break;
    case FILTERRES:
      synth[channel].Filter_res = value << 5;
      break;
    case FILTERKEY:
      synth[channel].Filter_key = value;
      break;
    case FILTERENV:
      synth[channel].Filter_env = value - 64;
      break;
    case MODAMOUNT:
      synth[channel].mod_amount = value << 1;
      break;
    case PAN:
      synth[channel].Pan = value << 5;
      break;
    case LFO1WAVE:
      synth[channel].Lfo1Wave = value;
      break;
    case LFO1FREQ:
      synth[channel].Lfo1Freq = value / 10.0;
      break;
    case LFO12PWA:
      synth[channel].Lfo1ToPwmA = value;
      break;
    case LFO12PWB:
      synth[channel].Lfo1ToPwmB = value;
      break;
    case LFO12FREQ:
      synth[channel].Lfo1ToFreq = value;
      break;
    case LFO12FILTER:
      synth[channel].Lfo1ToFilter = value;
      break;
    case LFO12PWDSO:
      synth[channel].Lfo1ToPwmDso = value;
      break;
    case LFO12PAN:
      synth[channel].Lfo1ToPan = value;
      break;
    case EG1ATTACK:
      synth[channel].eg1Attack = value;
      break;
    case EG1DECAY:
      synth[channel].eg1Decay = value;
      break;
    case EG1SUSTAIN:
      synth[channel].eg1Sustain = value;
      break;
    case EG1RELEASE:
      synth[channel].eg1Release = value;
      break;
    case EG2ATTACK:
      synth[channel].eg2Attack = value;
      break;
    case EG2DECAY:
      synth[channel].eg2Decay = value;
      break;
    case EG2SUSTAIN:
      synth[channel].eg2Sustain = value;
      break;
    case EG2RELEASE:
      synth[channel].eg2Release = value;
      break;
    case EG22FREQ:
      synth[channel].Eg2ToFreq = value;
      break;
    case VEL2VCA:
      synth[channel].VelToVca = value;
      break;
    case VEL2FILTER:
      synth[channel].VelToFilter = value;
      break;
    case AFT2VCA:
      synth[channel].AFTToVca = value;
      break;
    case AFT2FILTER:
      synth[channel].AFTToFilter = value;
      break;
    case KEY2PAN:
      synth[channel].keyToPan = value;
      break;
    case GLBDETUNE:
      synth[channel].GlbDetune = value - 64;
      break;
    case GLBTRANSPOSE:
      synth[channel].GlbTranspose = value - 64;
      break;
    case GLBVOLUME:
      synth[channel].volume = value;
      break;
    case GLBGLIDE:
      synth[channel].Glide = value;
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
  if (up) updateSynth(channel);
}

// Gestionnaire d'événements pour les messages pitch Bend
void handlePitchBend(byte channel, byte value1, byte value2) {
  int bender = (value1 << 7) + value2;
  synth[channel].bend = bender * bendfactor;
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




void setup() {
  // put your setup code here, to run once:
  pinMode(LED_BUILTIN, OUTPUT);

  // Select ADC input 0 (GPIO26) & 1
  adc_init();
  // Make sure GPIO is high-impedance, no pullups etc
  adc_gpio_init(I2C_ADR_READ + 26);
  adc_select_input(I2C_ADR_READ);
  uint16_t adcval = adc_read();  // Lecture de l'adresse I2C

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

  ITimer3.setInterval(1E6 / CONTROL_RATE, updateCtrl);

  Serial.begin(115200);
  //while (!Serial);
  Serial.print("I2C Adr : ");
  Serial.println(I2CAddress, HEX);
}

void setup1() {
  updateSynth(CurMidiChannel);
  ITimer4.setInterval(1E6 / AUDIO_RATE, updateAudio);
}

void loop() {
  if (eg1.getADSRState() == NOTEOFF) {
    gpio_put(LED_BUILTIN, false);  // turn the LED off by making the voltage LOW
    if (blink) {
      gpio_put(LED_BUILTIN, true);  // turn the LED on (HIGH is the voltage level)
      delay(50);                        // wait
      gpio_put(LED_BUILTIN, false);   // turn the LED off by making the voltage LOW
      blink = false;
      delay(50);
    }
  } else {
    gpio_put(LED_BUILTIN, true);  // turn the LED on (HIGH is the voltage level)
    if (blink) {
      gpio_put(LED_BUILTIN, false);   // turn the LED off by making the voltage LOW
      delay(50);                        // wait
      gpio_put(LED_BUILTIN, true);  // turn the LED on (HIGH is the voltage level)
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
