/**
 * @file midicontrol.h
 * @brief Defines the MIDI CC numbers assigned to each parameter.
 */

#ifndef MIDICTRL_H
#define MIDICTRL_H

//Whell Mod
#define WHEEL 1

//OSCILLATOR 1
#define OSC1WAVE 9
#define OSC1TRANSPOSE 10
#define OSC1PWM 11
#define OSC1DETUNE 18

//OSCILLATOR 2
#define OSC2WAVE 12
#define OSC2TRANSPOSE 13
#define OSC2DETUNE 14
#define OSC2PWM 15

//MIXER
#define BALANCE 16

//DSO
#define DSOWAVE 38
#define DSOTRANSPOSE 39
#define DSODETUNE 40
#define DSOPW 41
#define DSOMIX 42
#define MIXNOISE 44 

//ENVELOPPE GENERATOR 1
#define EG1ATTACK 19
#define EG1DECAY 20
#define EG1SUSTAIN 21
#define EG1RELEASE 22
//ENVELOPPE GENERATOR 2
#define EG2ATTACK 23
#define EG2DECAY 24
#define EG2SUSTAIN 25
#define EG2RELEASE 26
#define EG22FREQ 51

//LOW PASS FILTER
#define FILTERFC 74
#define FILTERFCLOW 75
#define FILTERRES 27
#define FILTERKEY 28
#define FILTERENV 29
#define MODAMOUNT 37

//LOW FREQUENCY OSCILLATOR 1
#define LFO1WAVE 30
#define LFO1FREQ 31
#define LFO12PWA 35
#define LFO12PWB 36
#define LFO12FREQ 85
#define LFO12FILTER 86
#define LFO12PWDSO 43
#define LFO12PAN 45 

//Velocity
#define VEL2VCA 46
#define VEL2FILTER 47

//Pan
#define KEY2PAN 48

//After Touch
#define AFT2VCA 49
#define AFT2FILTER 50

//GLOBAL PARAMETERS
#define GLBDETUNE 104
#define GLBTRANSPOSE 105
#define GLBVELOCITY 106
#define GLBVOLUME 7
#define GLBGLIDE 5
#define PAN 32

#define ALLSOUNDOFF 120
#define ALLNOTESOFF 123

#endif
