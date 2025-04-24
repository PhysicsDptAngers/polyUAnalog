/*
  LED.h - Library for controlling an LED using GPIO on microcontrollers.
  Created by [Your Name], [Date]

  This library implements a simple LED control class that allows controlling
  the state of an LED connected to a microcontroller through GPIO.

  Usage:
  - Include this header file in your project.
  - Create an instance of the LED class with a specified GPIO pin.
  - Use the methods provided by the LED class to control the LED state.
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
