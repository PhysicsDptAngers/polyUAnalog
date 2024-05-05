#ifndef MIDIMANAGER_H
#define MIDIMANAGER_H



// DECLARATION POUR HANDLER MIDI
//------------------------------------------------
enum { MIDI_OTHER,
       MIDI_NOTE_OFF,
       MIDI_NOTE_ON,
       MIDI_CONTROL_CHANGE,
       MIDI_PROGRAM_CHANGE,
       MIDI_PITCH_BEND
};

float midiToFreq(uint8_t midikey, int8_t transpose, int8_t detune) {
  float freq = 8.1757989156 * pow(2.0, ((midikey + transpose + GlbTranspose) / 12.0) + ((detune) / 1200.0));
  return freq;
}

//---------------------
// GESTIONN MIDI IN
//---------------------

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
    case WHEEL:
      lfo1.setAmplitude(value);
      break;
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
      balance = PWMResVCA * value / 128;
      as.set_Balance_cv(balance);
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
      Filter_freqHigh = value;
      Filter_freq = (Filter_freqHigh << 7) + Filter_freqLow;
      break;
    case FILTERFCLOW:
      Filter_freqHigh = value;
      Filter_freq = (Filter_freqHigh << 7) + Filter_freqLow;
      break;
    case FILTERRES:
      Filter_res = value << 7;
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




#endif