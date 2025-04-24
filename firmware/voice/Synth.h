#ifndef SYNTH_H
#define SYNTH_H

#include "hardware/pwm.h"
#include "hardware/dma.h"
#include "hardware/clocks.h"
#include "as3397.h"
#include "LFO.h"
#include "ADSR.h"
#include "DSO.h"


// use #define for AUDIO_RATE, not a constant
// Hz, powers of 2 are most reliable
const uint32_t AUDIO_RATE = F_CPU / (16 * 256);
// Taille du buffer audio
const uint32_t BUFFER_AUDIO_SIZE = 2048;
// use #define for CONTROL_RATE, not a constant
// Hz, powers of 2 are most reliable
const uint32_t CONTROL_RATE = F_CPU / (256 * BUFFER_AUDIO_SIZE);

#define BENDRANGE 12
const float bendfactor = ((BENDRANGE * 100.0) / 8190.0);

As3397 as(AUDIO_RATE);
LFO lfo1(CONTROL_RATE);
ADSR eg1(CONTROL_RATE);
ADSR eg2(CONTROL_RATE);
DSO dso(AUDIO_RATE);

static uint8_t waveformA = 1;
static int8_t pwmA = 0;
static int8_t transposeA = 64;
static int8_t detuneA = 64;
static uint8_t waveformB = 1;
static int8_t pwmB = 0;
static int8_t transposeB = 64;
static int8_t detuneB = 64;
static int8_t transposeDSO = 64;
static int8_t detuneDSO = 64;
static int8_t PwDSO = 0;
static int16_t balance = 64;
static int8_t noisemix = 0;
static int32_t Filter_freq = 4095;
static int8_t Filter_freqHigh = 127;
static int8_t Filter_freqLow = 127;
static int32_t Filter_res = 32;
static int32_t Filter_key = 0;
static int32_t Filter_env = 0;
static int8_t mod_amount = 0;
static int32_t Pan = 64;

static uint8_t key;
static float freqA = 440;
static float freqB = 220;
static float freqDSO;
static int64_t velocity;
static int64_t volume = 64;
static int8_t GlbTranspose = 0;
static int8_t GlbDetune = 0;
static int16_t Glide = 0;

static int32_t bend = 0;

static int32_t Lfo1ToPwmA = 0;
static int32_t Lfo1ToPwmB = 0;
static int32_t Lfo1ToPwmDso = 0;
static int32_t Lfo1ToFreq = 0;
static int32_t Lfo1ToFilter = 0;
static int32_t Lfo1ToRes = 0;
static int32_t Lfo1ToPan = 32;

static int64_t VelToVca = 127;
static int32_t VelToFilter = 0;

static int32_t AFTToVca = 0;
static int32_t AFTToFilter = 0;

static int32_t keyToPan = 64;

static int32_t Eg2ToFreq = 0;

static bool RAZPid = true;

static float yA = 440;
static float yB = 440;
static float yD = 440;


// Buffers contenant les échantillons 16 bits (séparés en MSB et LSB)
static uint32_t audio_buffer_A[BUFFER_AUDIO_SIZE];
static uint32_t audio_buffer_B[BUFFER_AUDIO_SIZE];

// DMA channels
static int dma_chan_audio;
static uint8_t active_buffer = 0;  // 0: attente, 1: remplir A, 2: attente, 3: remplir B
static uint8_t buffer_full = 0;

// Fonction d'interruption DMA pour basculer entre les buffers
void dma_irq_handler() {
  // Acknowledge the DMA interrupt request
  dma_channel_acknowledge_irq1(dma_chan_audio);

  // Basculer entre les buffers A et B
  if (active_buffer) {
    // Charger le buffer B
    dma_channel_set_read_addr(dma_chan_audio, audio_buffer_B, true);
    active_buffer = 0;  // Passer à buffer A
  } else {
    // Charger le buffer B
    dma_channel_set_read_addr(dma_chan_audio, audio_buffer_A, true);
    active_buffer = 1;  // Passer à buffer B
  }
}

void setup_dma() {
  // Configurer le premier canal DMA pour le MSB
  dma_chan_audio = dma_claim_unused_channel(true);
  dma_channel_config c = dma_channel_get_default_config(dma_chan_audio);
  channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
  channel_config_set_read_increment(&c, true);
  channel_config_set_write_increment(&c, false);
  channel_config_set_dreq(&c, DREQ_PWM_WRAP0 + 7);  // Déclenchement par PWM

  // Setup the channel and set it going
  dma_channel_configure(
    dma_chan_audio,
    &c,
    &pwm_hw->slice[7].cc,  // Destination : registre de comparaison PWM
    audio_buffer_A,        // Source
    BUFFER_AUDIO_SIZE,     // Nombre d'échantillons
    true                   // Démarrer immédiatement
  );
  // Interruption DMA pour gérer la bascule entre les buffers
  dma_channel_set_irq1_enabled(dma_chan_audio, true);  // Activer l'interruption pour le canal DMA MSB
  irq_set_exclusive_handler(DMA_IRQ_1, dma_irq_handler);
  irq_set_enabled(DMA_IRQ_1, true);
}

void setup_pwm() {
  uint slice_num = as.getSliceDsoMSB();  // Slice 7 pour GPIO 14 et 15

  pwm_set_clkdiv(slice_num, 4);

  /*pwm_config config = pwm_get_default_config();

  uint32_t clk_sys_hz = clock_get_hz(clk_sys);  // Fréquence actuelle du RP2040
  float clkdiv = (float)clk_sys_hz / (AUDIO_RATE * PWMRes);

  //pwm_config_set_clkdiv(&config, clkdiv);
  pwm_config_set_wrap(&config, PWMRes);  // 8 bits => 256 niveaux

  pwm_init(slice_num, &config, true);  // Active le PWM sur le slice 7
  */
}

void updateCtrl() {
  int32_t tmp;
  float FMmod;

  //
  // Control Signals
  //
  lfo1.update();
  eg1.update();
  eg2.update();

  //
  // DCOs
  //
  tmp = bend + (lfo1.vlfo * Lfo1ToFreq / 128) + (eg2.veg_a * Eg2ToFreq / 128) + GlbDetune;
  FMmod = pow(2, tmp / 1200.0);

  //Glide
  if (Glide) {
    yA += (freqA - yA) / Glide;
    yB += (freqB - yB) / Glide;
    yD += (freqDSO - yD) / Glide;
  } else {
    yA = freqA;
    yB = freqB;
    yD = freqDSO;
  }

  as.set_DcoA_freq(FMmod * yA);
  as.set_DcoB_freq(FMmod * yB);
  dso.setFrequency(FMmod * yD);

  //
  // VCF
  //
  as.set_Filter_freq_cv(Filter_freq + (eg2.veg_a * Filter_env / 8) + ((key - 64) * Filter_key)
                        + ((lfo1.vlfo * Lfo1ToFilter) / 4) + (velocity * VelToFilter / 8));
  as.set_Filter_res_cv(Filter_res + ((lfo1.vlfo * Lfo1ToRes) / 8));

  as.set_DcoA_pw_cv(pwmA + (lfo1.vlfo * Lfo1ToPwmA / 256));
  as.set_DcoB_pw_cv(pwmB + (lfo1.vlfo * Lfo1ToPwmB / 256));

  dso.setPw(PwDSO + (lfo1.vlfo * Lfo1ToPwmDso / 256));

  //
  // VCA
  //
  as.set_Vca_cv(eg1.veg * velocity * VelToVca * volume * PWMResVCA/ (128 * 128 * 128 * 256));
  as.set_Pan_cv(Pan + (lfo1.vlfo * Lfo1ToPan / 128) + ((key - 64) * keyToPan / 128));
}

// Fonction de bruit rapide
uint16_t fast_noise() {
static uint32_t lfsr = 0xACE1u; // Valeur de départ (seed)

    lfsr ^= lfsr >> 7;
    lfsr ^= lfsr << 9;
    lfsr ^= lfsr >> 13;
    return lfsr & 0x7FFF; // Valeur entre 0 et 32767
}

void updateAudio(uint32_t *audio_buffer) {
  static int32_t sample, wavea, waveinc;

  for (int i = 0; i < (BUFFER_AUDIO_SIZE / 16); i++) {
    //
    //DSO
    //
    dso.update();

    //
    // Osc Mix
    //
    sample = ((fast_noise() * noisemix) >> 7) + dso.vdso + 32768;
    //For linear interpolation 
    waveinc = (sample - wavea) >> 4;
    for (int j = 0; j < 16; j++) {
      wavea+= waveinc;  // linear interpolation 
      audio_buffer[i * 16 + j] = ((wavea >> 8) & 0xFF) + ((wavea & 0xFF) << 16);
    }
  }
}


void update_synth() {
  if (buffer_full != active_buffer) {
    if (active_buffer) {
      updateAudio(audio_buffer_B);
    } else {
      updateAudio(audio_buffer_A);
    }
    buffer_full = active_buffer;
    updateCtrl();
  }
}



#endif