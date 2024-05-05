#include "led.h"

// Constructor: Initializes the LED object with the specified pin.
LED::LED(int pin) {
  ledPin = pin;                    // Store the pin number
  gpio_init(ledPin);               // Initialize GPIO for the LED pin
  gpio_set_dir(ledPin, GPIO_OUT);  // Set the LED pin direction to output
}

// Turn on the LED.
void LED::Set(void) {
  if (state) return;        // If LED is already on, return early
  state = true;             // Update LED state to on
  gpio_put(ledPin, state);  // Set the LED pin to HIGH (on) state
}

// Turn off the LED.
void LED::Clr(void) {
  if (!state) return;       // If LED is already off, return early
  state = false;            // Update LED state to off
  gpio_put(ledPin, state);  // Set the LED pin to LOW (off) state
}

// Toggle the LED state (from on to off or vice versa).
void LED::Toggle(void) {
  state = !state;           // Invert the LED state
  gpio_put(ledPin, state);  // Set the LED pin to the new state (on or off)
}

// Get the current state of the LED (true if on, false if off).
bool LED::State(void) {
  return state;  // Return the current LED state
}

// Update the LED state based on blink status.
void LED::Update(void) {
  if (blink) {  // If blink count is non-zero
    blink--;    // Decrement the blink count
    Toggle();   // Toggle the LED state (on or off)
  }
}

// Trigger the LED to blink (for a set duration).
void LED::Blink(void) {
  blink += 2;  // Increment the blink count by 2 (for a blink duration)
}
