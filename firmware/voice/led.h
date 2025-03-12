/**
 * @file Led.h
 * @brief The Digital Signal Oscillator (DSO) class and instructions. 
 * Library for controlling an LED using GPIO on microcontrollers.
 */


#ifndef LED_H
#define LED_H

#include <hardware/gpio.h>  // Include library for GPIO control

class LED {
public:
  // Constructor: Initializes an LED object with the specified pin number.
  LED(int pin);

  // Turn on the LED.
  void Set(void);

  // Turn off the LED.
  void Clr(void);

  // Toggle the state of the LED (on to off or off to on).
  void Toggle(void);

  // Get the current state of the LED (true if on, false if off).
  bool State(void);

  // Update the LED state based on blinking behavior.
  // This method should be called periodically to manage blinking.
  void Update(void);

  // Trigger the LED to blink for a predefined duration.
  void Blink(void);

private:
  int ledPin = 0;          // GPIO pin connected to the LED
  bool state = false;      // Current state of the LED (false = off, true = on)
  int blink = 0;           // Counter to control LED blinking
};

#endif
