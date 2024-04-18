#ifndef ASPIN_H
#define ASPIN_H
// Définition des broches d'entrée sortie

#define VWFA_MSB_CV 10
#define VWFA_LSB_CV 11
#define VWFB_MSB_CV 12
#define VWFB_LSB_CV 13
#define DCOA_PW_CV 2
#define DCOB_PW_CV 3
#define BALANCE_CV 4

#define MOD_AMOUNT_CV 1
#define FILTER_FREQ_CV 7
#define FILTER_RES_CV 6

#define VCA_CV 5
#define PAN_CV 0

#define DSO_MSB 14
#define DSO_LSB 15

#define DCOA_FREQ 19
#define DCOB_FREQ 9
#define GAMME_A 18
#define GAMME_B 8

#define WS_BIT0 16
#define WS_BIT1 22

#define INPUT_RAMPE_A 0
#define INPUT_RAMPE_B 1

#define I2C_ADR_READ 2

#define MIDI_RX 17

// Définition des constantes pour le calcul de la rampe et de la fréquence
const float Rt = 13200;
const float Ct = 1.8E-9;
const uint8_t RampeDivisor = 4;
const float convFactor = (RampeDivisor * 3.3) / 4096;

//Gains du PI Kp et Ki
const float Kp = Rt * Ct * 16 * RampeDivisor;  //Le x16 c'est pour passer de 12bits (ADC In) à 16bits (PWM out)
const float Ki = 50;

// Définition des constantes pour le PWM
const int PWMRes = 256;
const int PWMResFilter = 4096;

// Définition des constantes pour le Wave select control
#define WAVE_NONE 0
#define WAVE_A 1
#define WAVE_B 2
#define WAVE_AB 3

#endif
