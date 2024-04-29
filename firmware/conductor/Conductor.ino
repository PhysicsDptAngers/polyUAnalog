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

    Orchestra conductor card

    v2.1
    - Multicast I2C transfert only
    - Optimize I2C preset transfert
    - Adding multitimbral function
    - Adding preset function

    To do
    - Adding preset transfert by SysEx
    - Adding second I2C interface control
    - One byte control change possibility

*/
#include <Wire.h>
#include <MIDI.h>
#include "preset.h"   // Include the header file containing presets

// Define the I2C clock frequency in Hz
#define I2CclockFreq 400000

// Define the I2C address used for communication
#define I2CAddress 0x10

// Create an instance of a MIDI object associated with Serial1
// This allows MIDI communication over the specified hardware serial port
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);


// MIDI message types
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

// Enumerate the different states of an ADSR (Attack, Decay, Sustain, Release) envelope
enum ADSRState {
  ATTACK,   // Attack phase of the envelope
  DECAY,    // Decay phase of the envelope
  SUSTAIN,  // Sustain phase of the envelope
  RELEASE,  // Release phase of the envelope
  NOTEOFF   // State indicating the note is turned off
};

// Define a structure to represent a voice in the synthesizer
struct listvoices {
  byte adr = ADR;       // I2C address of the voice (initialized to default ADR)
  ADSRState state = NOTEOFF;  // Current state of the voice (initialized to NOTEOFF)
  int note = -1;         // MIDI note number being played (-1 indicates no note is assigned)
  uint32_t count = 0;    // Counter used for voice management and priority
};


// Create an IntervalTimer object
IntervalTimer myTimer;

byte nVoices = 0;           // Number of active voices currently playing
struct listvoices voices[MAXVOICES];  // Array to store information about each voice
uint32_t counter = 0;       // Counter used for voice management and priority

bool blink = false;         // Flag to indicate if a visual blink effect should occur
bool MonoMode = false;      // Flag to indicate monophonic (true) or polyphonic (false) mode

byte controlValue[256];     // Array to store control values for MIDI parameters

// Function to load a preset into the synthesizer
void loadPreset(byte presetNumber) {
  // Checks if the preset number is valid
  if (presetNumber >= sizeof(Presets) / sizeof(Presets[0])) {
    // Check if preset number is out of range
    return;
  }

  const uint8_t* preset = Presets[presetNumber];  // Get the preset data from the Presets array

  Wire.beginTransmission(0);  // Begin transmission over I2C to address 0
  for (uint j = 0; j < NBPARAM; j += 2) {
    controlValue[preset[j]] = preset[j + 1];  // Set control values based on the preset data
    Wire.write(CtrChange);    // Send MIDI control change message
    Wire.write(preset[j]);    // Send MIDI control number
    Wire.write(preset[j + 1]); // Send MIDI control value
    blink = true;              // Set blink flag to indicate a change
  }
  Wire.endTransmission();  // End I2C transmission
}

// Function to obtain the controller name from its number
const char* obtenirNomDuControleur(byte numero) {
  for (byte i = 0; i < sizeof(controlleurNum); i++) {
    if (controlleurNum[i] == numero) return controlleurNom[i];  // Return the controller name if found
  }
  return "";  // Return an empty string if controller number is not found
}

// Function to display the current presets stored in memory
void printPreset(void) {
  byte i;
  Serial.print("const uint8_t Preset1[] = { ");
  for (i = 0; i < (sizeof(controlleurNum) - 1); i++) {
    if (i % 4 == 0) Serial.println("  ");  // Print new line for better formatting
    Serial.printf("%s, %d, ", controlleurNom[i], controlValue[controlleurNum[i]]);  // Print controller name and value
  }
  Serial.printf("%s, %d\n", controlleurNom[i], controlValue[controlleurNum[i]]);  // Print last controller name and value
  Serial.println("};");  // Print end of preset array
}

// Function to handle MIDI Note On event
void OnNoteOn(byte channel, byte note, byte velocity) {
  uint32_t idx;
  byte voicefree = 255;

  if (MonoMode) {
    // Send Note On message via I2C if in monophonic mode
    sendI2c(0, NoteOn, note + channel, velocity);
    blink = true;  // Set blink flag to indicate a change
  } else {
    // Check if a channel is already playing the note; if so, return
    for (int i = 0; i < nVoices; i++) {
      if (voices[i].note == note) {
        return;
      }
    }
    // Look for a free voice to play the note
    for (int i = 0; i < nVoices; i++) {
      Wire.requestFrom((int)voices[i].adr, 1);
      while (Wire.available()) {
        voices[i].state = (ADSRState)Wire.read();  // Update voice state from I2C
      }
      if (voices[i].state == NOTEOFF) {
        // Assign the free voice to play the note
        voices[i].state = ATTACK;
        voices[i].note = note;
        counter++;
        voices[i].count = counter;
        // Send Note On message via I2C
        sendI2c(voices[i].adr, NoteOn + channel, note, velocity);
        blink = true;  // Set blink flag to indicate a change
        return;
      }
    }
    // Search for the oldest channel in RELEASE state to reassign
    idx = counter + 1;
    for (int i = 0; i < nVoices; i++) {
      if ((voices[i].state != RELEASE) && (idx > voices[i].count)) {
        idx = voices[i].count;
        voicefree = i;
      }
    }
    if (voicefree == 255) {
      // Search for the oldest voice if no free voices found
      for (int i = 0; i < nVoices; i++) {
        if (idx > voices[i].count) {
          idx = voices[i].count;
          voicefree = i;
        }
      }
    }
    if (voicefree < nVoices) {
      // Assign the free voice to play the note
      voices[voicefree].state = ATTACK;
      voices[voicefree].note = note;
      counter++;
      voices[voicefree].count = counter;
      // Send Note On message via I2C
      sendI2c(voices[voicefree].adr, NoteOn + channel, note, velocity);
      blink = true;  // Set blink flag to indicate a change
    }
  }
}

// Function to handle MIDI Note Off event
void OnNoteOff(byte channel, byte note, byte velocity) {
  if (MonoMode) {
    // Send Note Off message via I2C if in monophonic mode
    sendI2c(0, NoteOff + channel, note, velocity);
    blink = true;  // Set blink flag to indicate a change
  } else {
    // Check each voice for the note to turn off
    for (int i = 0; i < nVoices; i++) {
      Wire.requestFrom((int)voices[i].adr, 1);
      while (Wire.available()) {
        voices[i].state = (ADSRState)Wire.read();  // Update voice state from I2C
      }
      if (voices[i].note == note) {
        // Reset note and change voice state to RELEASE if not already released or turned off
        voices[i].note = -1;
        if ((voices[i].state != RELEASE) && (voices[i].state != NOTEOFF)) {
          voices[i].state = RELEASE;
          counter++;
          voices[i].count = counter;
          // Send Note Off message via I2C
          sendI2c(voices[i].adr, NoteOff + channel, note, velocity);
          blink = true;  // Set blink flag to indicate a change
        }
      }
    }
  }
}

// Function to handle MIDI Control Change events
void OnControlChange(byte channel, byte controler, byte value) {
  controlValue[controler] = value;  // Update control value in array

  // Handle specific control change events based on controller number
  if (controler == GLBDETUNE) {
    // Apply detune to all active voices
    for (int i = 0; i < nVoices; i++) {
      if (value) {
        // Generate random detune value and send via I2C
        sendI2c(voices[i].adr, CtrChange + channel, GLBDETUNE, (rand() % value) / 2);
      } else {
        // Send zero detune value if controller value is zero
        sendI2c(voices[i].adr, CtrChange + channel, GLBDETUNE, 0);
      }
      blink = true;  // Set blink flag to indicate a change
    }
  } else if (controler == WHEEL) {
    int Cutoff = value << 7;  // Shift value for cutoff frequency control

    // Send cutoff frequency control message via I2C
    sendI2c(0, CtrChange + channel, FILTERFCLOW, Cutoff & 0x7F);
    sendI2c(0, CtrChange + channel, FILTERFC, Cutoff >> 7);
    blink = true;  // Set blink flag to indicate a change

  } else if (controler == DEFAULTCONF) {
    // Load preset based on the controller value
    loadPreset(value);
  } else if (controler <= 124) {
    // Send general control change message via I2C for controllers <= 124
    sendI2c(0, CtrChange + channel, controler, value);
    blink = true;  // Set blink flag to indicate a change

  } else if (controler == 126) {
    // Set synthesizer to monophonic operation
    MonoMode = true;
    // Turn off all sounds (All Notes Off)
    OnControlChange(channel, 120, 0);
  } else if (controler == 127) {
    // Set synthesizer to polyphonic operation
    MonoMode = false;
    // Turn off all sounds (All Notes Off)
    OnControlChange(channel, 120, 0);
  }
}

// Function to handle MIDI Pitch Change events
void OnPitchChange(byte channel, int pitch) {
  for (int i = 0; i < nVoices; i++) {
    // Send pitch bend message via I2C to each active voice
    sendI2c(voices[i].adr, PitchBend + channel, pitch >> 7, pitch & 0x7F);
    blink = true;  // Set blink flag to indicate a change
  }
}


// Function: sendI2c
// Description: Sends a custom MIDI-like message via I2C to a specified address
// Parameters:
//   - address: The I2C address of the target device
//   - midicode: The MIDI-like command code to be sent
//   - value1: The first value (data byte) associated with the command
//   - value2: The second value (data byte) associated with the command (16-bit value)
void sendI2c(byte address, byte midicode, byte value1, int value2) {
  // Begin an I2C transmission to the specified address
  Wire.beginTransmission(address);

  // Send the MIDI-like command code (midicode) over I2C
  Wire.write(midicode);

  // Send the first value (data byte) over I2C
  Wire.write(value1);

  // Send the second value (16-bit data) over I2C
  // Note: The value2 parameter is split into two bytes for transmission
  Wire.write((byte)(value2 & 0xFF));    // Send the low byte (LSB) of value2
  Wire.write((byte)((value2 >> 8) & 0xFF));  // Send the high byte (MSB) of value2

  // End the I2C transmission
  Wire.endTransmission();
}


// Function: scanVoices
// Description: Scans for connected voices/modules via I2C communication and retrieves their state
void scanVoices() {
  byte address = 0;

  // Iterate over possible I2C addresses to scan for connected voices/modules
  for (int i = 0; i < MAXVOICES; i++) {
    address = ADR + i;  // Calculate the I2C address to scan
    Wire.beginTransmission(address);
    byte error = Wire.endTransmission();

    // Check if the device at the current address responded without errors (error code 0)
    if (error == 0) {
      // Print information about the discovered voice/module
      Serial.print("Voice ");
      Serial.print(nVoices, DEC);
      Serial.print(" found at address 0x");
      Serial.print(address, HEX);
      voices[i].adr = address;  // Store the discovered address in the voices array

      // Request state information from the voice/module
      Wire.requestFrom((int)voices[i].adr, 1);
      while (Wire.available()) {
        voices[i].state = (ADSRState)Wire.read();  // Read and store the state
      }

      // Print the state of the discovered voice/module
      Serial.print(" State: ");
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
      ++nVoices;  // Increment the count of discovered voices
    } else if (error == 4) {
      // Handle unknown error (no response) at the current address
      Serial.print("Unknown error at address 0x");
      if (address < 16) {
        Serial.print("0");  // Print leading zero for addresses less than 16
      }
      Serial.println(address, HEX);
    }
  }

  // If no voices/modules were found, print a message indicating no devices were detected
  if (nVoices == 0) {
    Serial.println("No voices/modules found\n");
    // Perform action to turn all sounds off (reset)
    OnControlChange(0, 120, 0);
  }
}


// Function: i2cReceive
// Description: Handles incoming I2C data received by the device
// Parameters:
//   - bytesReceived: Number of bytes received (not explicitly used in this function)
void i2cReceive(int bytesReceived) {
  // Loop through all available bytes received via I2C
  while (Wire1.available()) {
    byte value = Wire1.read();  // Read the received byte
    OnMidi(value);  // Process the MIDI data based on the received byte
  }
  blink = true;  // Set blink flag to true for LED indication
}


// Function: i2cRequest
// Description: Placeholder function for handling I2C data requests (not implemented)
void i2cRequest() {
  // This function is currently empty and serves as a placeholder
  // for handling I2C data requests if needed in future development.
  // It can be implemented to respond to specific I2C data requests from master devices.
}


//---------------------
// MIDI I2C MANAGEMENT
//---------------------

// MIDI HANDLER DECLARATION
//------------------------------------------------
enum MidiMessageType {
  MIDI_OTHER,           // Other or unrecognized MIDI message
  MIDI_NOTE_OFF,        // MIDI Note Off message
  MIDI_NOTE_ON,         // MIDI Note On message
  MIDI_CONTROL_CHANGE,  // MIDI Control Change message
  MIDI_PROGRAM_CHANGE,  // MIDI Program Change message
  MIDI_PITCH_BEND       // MIDI Pitch Bend message
};


void OnMidi(uint8_t Midibyte) {
  static uint8_t midimode = MIDI_OTHER;   // Current MIDI message mode
  static int8_t midibytesleft = -1;       // Number of bytes left to read for the current MIDI message
  static uint8_t midibuffer[2];           // Buffer to store MIDI message data bytes
  static byte MidiChannel;                // MIDI channel associated with the incoming MIDI message

  //------------
  // STATUS BYTE
  //------------
  if (Midibyte >= 0x80) {  // Check if the received byte is a status byte (>= 128)
    if (Midibyte <= 0xEF) {
      MidiChannel = (Midibyte & 0x0F);    // Extract MIDI channel (lower 4 bits)
      Midibyte = Midibyte & 0xF0;         // Extract MIDI status type (upper 4 bits)
    }
    switch (Midibyte) {
      // MIDI NOTE OFF
      case 0x80:
        midimode = MIDI_NOTE_OFF;         // Set mode to MIDI Note Off
        midibytesleft = 2;                 // Two bytes (note and velocity) to read
        OnNoteOff(MidiChannel, 0, 0);      // Trigger Note Off action (channel, note, velocity)
        break;
      // MIDI NOTE ON
      case 0x90:
        midimode = MIDI_NOTE_ON;          // Set mode to MIDI Note On
        midibytesleft = 2;                 // Two bytes (note and velocity) to read
        break;
      // MIDI CONTROL CHANGE
      case 0xB0:
        midimode = MIDI_CONTROL_CHANGE;   // Set mode to MIDI Control Change
        midibytesleft = 2;                 // Two bytes (controller number and value) to read
        break;
      // MIDI PITCH-BEND
      case 0xE0:
        midimode = MIDI_PITCH_BEND;       // Set mode to MIDI Pitch Bend
        midibytesleft = 2;                 // Two bytes (LSB and MSB) to read
        break;
      default:
        midimode = MIDI_OTHER;            // Unknown status, reset mode
        midibytesleft = -1;               // No bytes left to read
        break;
    }
  }
  //------------------------
  // OTHERWISE DATA BYTES
  //------------------------
  else {
    // Store bytes to be read if necessary
    if (midibytesleft > 0) {              // Are there any bytes left to read?
      midibuffer[0] = midibuffer[1];      // Byte swap: shift previous byte to the first position
      midibuffer[1] = Midibyte;           // Store current byte in the buffer
      midibytesleft--;                    // Decrement the byte counter
    }
    // Process completed MIDI message
    if (midibytesleft == 0) {             // All bytes of the MIDI message have been received
      midibytesleft = -1;                 // Reset byte counter
      switch (midimode) {
        case MIDI_NOTE_OFF:
          //handleNoteOff(MidiChannel, midibuffer[0], midibuffer[1]);  // Handle Note Off event
          break;
        case MIDI_NOTE_ON:
          // Turn note off if velocity is zero, otherwise turn note on
          if (midibuffer[1] == 0) {
            OnNoteOff(MidiChannel, midibuffer[0], midibuffer[1]);
          } else if (midibuffer[1] != 0) {
            OnNoteOn(MidiChannel, midibuffer[0], midibuffer[1]);
          }
          break;
        case MIDI_CONTROL_CHANGE:
          OnControlChange(MidiChannel, midibuffer[0], midibuffer[1]);  // Handle Control Change event
          break;
        case MIDI_PITCH_BEND:
          OnPitchChange(MidiChannel, (midibuffer[0] << 7) + midibuffer[1]);  // Handle Pitch Bend event
          break;
        default:
          break;
      }
    }
  }
}


// SETUP FUNCTION
void setup() {
  // Configure I2C for primary and secondary buses
  Wire.setSDA(18);          // Set SDA pin for primary Wire library
  Wire.setSCL(19);          // Set SCL pin for primary Wire library
  Wire.setClock(I2CclockFreq);  // Set I2C clock frequency for primary Wire library
  Wire.begin();             // Initialize primary Wire library

  Wire1.setSDA(23);         // Set SDA pin for secondary Wire library
  Wire1.setSCL(22);         // Set SCL pin for secondary Wire library
  Wire1.setClock(I2CclockFreq);  // Set I2C clock frequency for secondary Wire library
  Wire1.begin(I2CAddress);  // Initialize secondary Wire library with specified address
  Wire1.onReceive(i2cReceive);   // Set callback for I2C receive event
  Wire1.onRequest(i2cRequest);   // Set callback for I2C request event

  pinMode(LED_BUILTIN, OUTPUT);  // Set LED pin as output

  Serial.begin(9600);       // Initialize serial communication
  //while (!Serial);         // Wait for serial port to open

  delay(4000);               // Delay to allow for initialization

  scanVoices();              // Scan and initialize connected voices

  // Initialize MIDI interfaces and set event handlers
  MIDI.begin(MIDI_CHANNEL_OMNI);       // Initialize MIDI interface for all channels
  MIDI.setHandleNoteOn(OnNoteOn);      // Set callback for MIDI Note On messages
  usbMIDI.setHandleNoteOn(OnNoteOn);   // Set callback for USB MIDI Note On messages
  MIDI.setHandleNoteOff(OnNoteOff);    // Set callback for MIDI Note Off messages
  usbMIDI.setHandleNoteOff(OnNoteOff); // Set callback for USB MIDI Note Off messages
  MIDI.setHandleControlChange(OnControlChange);  // Set callback for MIDI Control Change messages
  usbMIDI.setHandleControlChange(OnControlChange);   // Set callback for USB MIDI Control Change messages
  MIDI.setHandlePitchBend(OnPitchChange);   // Set callback for MIDI Pitch Bend messages
  usbMIDI.setHandlePitchChange(OnPitchChange); // Set callback for USB MIDI Pitch Bend messages

  digitalWrite(LED_BUILTIN, HIGH);   // Turn on the built-in LED
  delay(400);  // Blink LED once at startup
  digitalWrite(LED_BUILTIN, LOW);    // Turn off the built-in LED

  myTimer.begin(blinkLED, 50000);    // Start timer for blinking LED at regular intervals
  loadPreset(0);  // Load default preset
}

// BLINK LED FUNCTION (TIMER CALLBACK)
void blinkLED() {
  if (blink) {
    digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED on (HIGH is the voltage level)
    blink = false;  // Reset blink flag
  } else {
    digitalWrite(LED_BUILTIN, LOW);   // Turn the LED off by making the voltage LOW
  }
}

// SERIAL EVENT HANDLER
void serialEvent() {
  String input;
  if (Serial.available() > 0) {
    input = Serial.readString();   // Read serial input as string
    if (input.startsWith("p")) {   // Check if input starts with 'p'
      printPreset();               // Trigger function to print current preset
    }
  }
}

// MAIN LOOP
void loop() {
  usbMIDI.read();   // Read incoming USB MIDI messages
  MIDI.read();      // Read incoming MIDI messages
}
