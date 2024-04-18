// USB MIDI receive example, Note on/off -> LED on/off
// contributed by Alessandro Fasan
#include <Wire.h>
#include <MIDI.h>

#define I2CclockFreq 400000
#define I2CAddress 0x10

MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);

#define MULTICAST 1

#define NoteOff 0x80
#define NoteOn 0x90
#define Aftertouch 0XA0
#define CtrChange 0xB0
#define ProgramChange 0xC0
#define ChannelPressure 0xD0
#define PitchBend 0xE0

#define GLBDETUNE 104
//Whell Mod
#define WHEEL 1
//LOW PASS FILTER
#define FILTERFC 74
#define FILTERFCLOW 75
#define DEFAULTCONF 121

#define ADR 0x10

#define MAXVOICES 16

enum ADSRState { ATTACK,
                 DECAY,
                 SUSTAIN,
                 RELEASE,
                 NOTEOFF };

const uint8_t Preset[] = { 9, 1, 10, 64, 11, 64, 18, 64,
                           12, 1, 13, 64, 14, 64, 15, 64,
                           16, 0,
                           38, 0, 39, 64, 40, 64, 41, 64, 42, 0, 44, 0,
                           19, 1, 20, 127, 21, 127, 22, 1,
                           23, 1, 24, 127, 25, 127, 26, 1, 51, 0,
                           75, 63, 74, 31, 27, 63, 28, 0, 29, 0, 37, 0,
                           30, 0, 31, 16, 35, 0, 36, 0, 85, 0, 86, 0, 43, 0, 45, 0,
                           46, 127, 47, 0,
                           48, 0, 49, 0, 50, 0, 104, 64, 105, 64, 106, 127, 7, 32, 5, 0, 32, 64,
                           120, 1, 123, 0 };

struct listvoices {
  byte adr = ADR;
  ADSRState state = NOTEOFF;
  int note = -1;
  uint32_t count = 0;
};


// Create an IntervalTimer object
IntervalTimer myTimer;

byte nVoices = 0;
struct listvoices voices[MAXVOICES];
uint32_t counter = 0;

bool blink = false;

bool MonoMode = false;

void OnNoteOn(byte channel, byte note, byte velocity) {
  uint32_t idx;
  byte voicefree = 255;


  if (MonoMode) {
#if MULTICAST == 0
    for (int i = 0; i < nVoices; i++) {
      sendI2c(voices[i].adr, NoteOn, note, velocity);
      blink = true;
    }
#else
    sendI2c(0, NoteOn, note, velocity);
    blink = true;
#endif
  } else {
    //Vérifie si une voie joue déjà la note
    for (int i = 0; i < nVoices; i++) {
      if (voices[i].note == note) {
        return;
      }
    }
    //Cherche une voie libre
    for (int i = 0; i < nVoices; i++) {
      Wire.requestFrom((int)voices[i].adr, 1);
      while (Wire.available()) {
        voices[i].state = (ADSRState)Wire.read();
      }
      if (voices[i].state == NOTEOFF) {
        voices[i].state = ATTACK;
        voices[i].note = note;
        counter++;
        voices[i].count = counter;
        sendI2c(voices[i].adr, NoteOn, note, velocity);
        blink = true;
        return;
      }
    }
    //Cherche la voie la plus ancienne à l'état RELEASE
    idx = counter + 1;
    for (int i = 0; i < nVoices; i++) {
      if ((voices[i].state != RELEASE) && (idx > voices[i].count)) {
        idx = voices[i].count;
        voicefree = i;
      }
    }
    if (voicefree == 255) {
      //Cherche la voie la plus ancienne
      for (int i = 0; i < nVoices; i++) {
        if (idx > voices[i].count) {
          idx = voices[i].count;
          voicefree = i;
        }
      }
    }
    if (voicefree < nVoices) {
      voices[voicefree].state = ATTACK;
      voices[voicefree].note = note;
      counter++;
      voices[voicefree].count = counter;
      sendI2c(voices[voicefree].adr, NoteOn, note, velocity);
      blink = true;
    }
  }
}

void OnNoteOff(byte channel, byte note, byte velocity) {
  if (MonoMode) {
#if MULTICAST == 0
    for (int i = 0; i < nVoices; i++) {
      sendI2c(voices[i].adr, NoteOff, note, velocity);
      blink = true;
    }
#else
    sendI2c(0, NoteOff, note, velocity);
    blink = true;
#endif
  } else {
    for (int i = 0; i < nVoices; i++) {
      Wire.requestFrom((int)voices[i].adr, 1);
      while (Wire.available()) {
        voices[i].state = (ADSRState)Wire.read();
      }
      if (voices[i].note == note) {
        voices[i].note = -1;
        if ((voices[i].state != RELEASE) && (voices[i].state != NOTEOFF)) {
          voices[i].state = RELEASE;
          counter++;
          voices[i].count = counter;
          sendI2c(voices[i].adr, NoteOff, note, velocity);
          blink = true;
        }
      }
    }
  }
}

void OnControlChange(byte channel, byte controler, byte value) {
  if (controler == GLBDETUNE) {
    for (int i = 0; i < nVoices; i++) {
      if (value) sendI2c(voices[i].adr, CtrChange, GLBDETUNE, (rand() % value) / 2);
      else sendI2c(voices[i].adr, CtrChange, GLBDETUNE, 0);
      blink = true;
    }
  } else if (controler == WHEEL) {
    int Cutoff = value * 32;
#if MULTICAST == 0
    for (int i = 0; i < nVoices; i++) {
      sendI2c(voices[i].adr, CtrChange, FILTERFCLOW, Cutoff & 0x7F);
      sendI2c(voices[i].adr, CtrChange, FILTERFC, Cutoff >> 7);
      blink = true;
    }
#else
    sendI2c(0, CtrChange, FILTERFCLOW, Cutoff & 0x7F);
    sendI2c(0, CtrChange, FILTERFC, Cutoff >> 7);
    blink = true;

#endif
  } else if (controler == DEFAULTCONF) {
#if MULTICAST == 0
    for (int i = 0; i < nVoices; i++) {
      for (int j = 0; j < sizeof(Preset); j += 2)
        sendI2c(voices[i].adr, CtrChange, Preset[j], Preset[j + 1]);
      blink = true;
    }
#else
    for (uint j = 0; j < sizeof(Preset); j += 2) {
      sendI2c(0, CtrChange, Preset[j], Preset[j + 1]);
      blink = true;
      delay(20);
    }
#endif
  } else if (controler <= 124) {
#if MULTICAST == 0
    for (int i = 0; i < nVoices; i++) {
      sendI2c(voices[i].adr, CtrChange, controler, value);
      blink = true;
    }
#else
    sendI2c(0, CtrChange, controler, value);
    blink = true;

#endif
  } else if (controler == 126) {
    //Monophonic Operation
    MonoMode = true;
    //All sounds Off
    OnControlChange(0, 120, 0);
  } else if (controler == 127) {
    //Polyphonic Operation
    MonoMode = false;
    //All sounds Off
    OnControlChange(0, 120, 0);
  }
}

void OnPitchChange(byte channel, int pitch) {
  for (int i = 0; i < nVoices; i++) {
    //pitch = pitch + 8192;
    sendI2c(voices[i].adr, PitchBend, pitch >> 8, pitch & 0xFF);
    blink = true;
  }
}


void sendI2c(byte address, byte midicode, byte value1, int value2) {
  Wire.beginTransmission(address);
  Wire.write(midicode);
  Wire.write(value1);
  Wire.write(value2);
  Wire.endTransmission();
}

void scanVoices() {
  byte address = 0;

  for (int i = 0; i < MAXVOICES; i++) {
    address = ADR + i;
    Wire.beginTransmission(address);
    byte error = Wire.endTransmission();
    if (error == 0) {
      Serial.print("Voices ");
      Serial.print(nVoices, DEC);
      Serial.print(" found at address 0x");
      Serial.print(address, HEX);
      voices[i].adr = address;

      Wire.requestFrom((int)voices[i].adr, 1);
      while (Wire.available()) {
        voices[i].state = (ADSRState)Wire.read();
      }
      Serial.print(" State : ");
      switch (voices[i].state) {
        case ATTACK:
          Serial.print("ATTACK");
          break;
        case DECAY:
          Serial.print("DECAY");
          break;
        case SUSTAIN:
          Serial.print("SUSTAIN");
          break;
        case RELEASE:
          Serial.print("RELEASE");
          break;
        case NOTEOFF:
          Serial.print("NOTEOFF");
          break;
      }
      Serial.println("");
      ++nVoices;

    } else if (error == 4) {
      Serial.print("Unknown error at address 0x");
      if (address < 16) {
        Serial.print("0");
      }
      Serial.println(address, HEX);
    }
  }
  if (nVoices == 0) {
    Serial.println("No Voices found\n");
    //for(;;);
  }
  //All sounds Off
  OnControlChange(0, 120, 0);
}

void i2cReceive(int bytesReceived) {
  while (Wire1.available()) {  // loop through all but the last
    byte value = Wire1.read();
    OnMidi(value);
  }
  blink = true;
}

void i2cRequest() {
}

//---------------------
// GESTION MIDI I2C
//---------------------

// DECLARATION POUR HANDLER MIDI
//------------------------------------------------
enum { MIDI_OTHER,
       MIDI_NOTE_OFF,
       MIDI_NOTE_ON,
       MIDI_CONTROL_CHANGE,
       MIDI_PROGRAM_CHANGE,
       MIDI_PITCH_BEND
};


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
        OnNoteOff(MidiChannel, 0, 0);
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
            OnNoteOff(MidiChannel, midibuffer[0], midibuffer[1]);
          } else if (midibuffer[1] != 0) {
            OnNoteOn(MidiChannel, midibuffer[0], midibuffer[1]);
          }
          break;
        case MIDI_CONTROL_CHANGE:
          OnControlChange(MidiChannel, midibuffer[0], midibuffer[1]);
          break;
        case MIDI_PITCH_BEND:
          OnPitchChange(MidiChannel, (midibuffer[0] << 7) + midibuffer[1]);
          break;
        default:
          break;
      }
    }
  }
}


void setup() {
  Wire.setSDA(18);
  Wire.setSCL(19);
  Wire.setClock(I2CclockFreq);
  Wire.begin();

  Wire1.setSDA(23);
  Wire1.setSCL(22);
  Wire1.setClock(I2CclockFreq);
  Wire1.begin(I2CAddress);
  Wire1.onReceive(i2cReceive);
  Wire1.onRequest(i2cRequest);

  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(9600);
  //while (!Serial);

  delay(4000);

  scanVoices();

  MIDI.begin(MIDI_CHANNEL_OMNI);
  MIDI.setHandleNoteOn(OnNoteOn);
  usbMIDI.setHandleNoteOn(OnNoteOn);
  MIDI.setHandleNoteOff(OnNoteOff);
  usbMIDI.setHandleNoteOff(OnNoteOff);
  MIDI.setHandleControlChange(OnControlChange);
  usbMIDI.setHandleControlChange(OnControlChange);
  MIDI.setHandlePitchBend(OnPitchChange);
  usbMIDI.setHandlePitchChange(OnPitchChange);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(400);  // Blink LED once at startup
  digitalWrite(LED_BUILTIN, LOW);
  myTimer.begin(blinkLED, 50000);
}

void blinkLED() {
  if (blink) {
    digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
    blink = false;
  } else digitalWrite(LED_BUILTIN, LOW);  // turn the LED off by making the voltage LOW
}

void loop() {
  usbMIDI.read();
  MIDI.read();
}
