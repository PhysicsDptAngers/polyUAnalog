/**
 * @file adsr.h
 * @brief Envelope generator class implementing an ADSR (Attack, Decay, Sustain, Release) model.
 * 
 * A basic implementation of an Attack-Decay-Sustain-Release (ADSR) envelope. Nothing special.
 * 
 */

#ifndef ADSR_H
#define ADSR_H
#include <stdint.h>
#include "tables.h"
#include "aspin.h"

#define AMAX 0x7FFF

enum ADSRState { ATTACK, DECAY, SUSTAIN, RELEASE, NOTEOFF };

class ADSR {
private:
    ADSRState state;
    uint16_t attack_inc;
    uint16_t decay_inc;
    uint16_t release_inc;
    float attack_time;
    float decay_time;
    int16_t sustain;
    float release_time;
    int16_t value;
    uint32_t ADSR_RATE;  // ajout de ADSR_RATE comme membre de classe

public:
    ADSR(uint32_t rate);  // modifié pour accepter ADSR_RATE comme argument
    void gateOn();
    void gateOff();
    void soundOff();
    void setAttack(int8_t timems);
    void setDecay(int8_t timems);
    void setSustain(int8_t value);
    void setRelease(int8_t timems);
    void update();
    ADSRState getADSRState();

    int32_t veg;
    int32_t veg_a;
    int32_t veg_f;
};

#endif // ADSR_H
