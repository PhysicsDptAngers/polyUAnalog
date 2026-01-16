# Conductor Card

The *Conductor Card* serves as the central hub for the synthesizer, managing communication, power distribution, and analog signal summing. Its architecture is designed to interface external controls with internal voice modules.

## Key Functionalities

### 1. Data I/O & Connectivity

The card manages multiple communication protocols to interface with the outside world:

* **MIDI:** Standard MIDI IN port and an optional MIDI OUT header.
* **I2C**:
	* Dedicated header for optional *"Controllers"* (e.g., a front panel equipped with knobs and sliders for real-time parameter control).
	* Dedicated header for optional *"FX card"*
* **Wireless (UDP):** Support for UDP over Wi-Fi when utilizing a *Raspberry Pi Pico 2 W*.
* **Storage:** Integrated SD card slot for storing and retrieving presets and system data.

### 2. Voice Dispatch & Power Distribution

The Conductor Card serves as the central interface for system data and power management:

* **Power Input:** Features a dedicated input port for the main power rails (*+12V, -12V, +5V*).
* **Power Routing:** Distributes stable power rails from the main input to all connected internal modules and voice cards.
* **Signal Dispatch:** Routes digital control data—including **Note On/Off, MIDI CC, and Clock** signals—to the individual voice cards via the *I2C bus*.

### 3. Analog Signal Processing & Mixing

The card features a dedicated analog summing stage to consolidate the synthesizer's output:

* **Current-to-Voltage Conversion:** Collects the current outputs from the *AS3397-based voice cards* via a Trans-Impedance Amplifier (TIA).
* **FX Integration:** Bi-directional interfacing with an optional FX card (Send/Return or post-summing injection).
* **Final Mix:** Sums the voice signals with the FX return to produce the final audio output.

## Connectors Pinouts

### 1. Voice Connector (J13)
Single row header (1x9).

```text
┌───┬───┬───┬───┬───┬───┬───┬───┬───┐
│ 1 │ 2 │ 3 │ 4 │ 5 │ 6 │ 7 │ 8 │ 9 │
└───┴───┴───┴───┴───┴───┴───┴───┴───┘
  │   │   │   │   │   │   │   │   │
  │   │   │   │   │   │   │   │   └─ -5V
  │   │   │   │   │   │   │   └───  +3.3V (for i2c Address)
  │   │   │   │   │   │   └───── +5V
  │   │   │   │   │   └─────── GND
  │   │   │   │   └───────── +12V
  │   │   │   └─────────── I2C_Voices_SDA
  │   │   └───────────── I2C_Voices_SCL
  │   └─────────────── Audio_Analog_Right
  └───────────────── Audio_Analog_Left

```

### 2. Power Input (J7 - Conn_POWER)

Dual row header (2x10).

```text
       ┌───────────┐
   -5V │ 1       2 │ GND
   GND │ 3       4 │ +5V
   GND │ 5       6 │ +12V
   GND │ 7       8 │ GND
   GND │ 9      10 │ GND
       └───────────┘

```

### 3. I2C Controller (J15)

Single row header (1x4).

```text
  ┌───┬───┬───┬───┐
  │ 1 │ 2 │ 3 │ 4 │
  └───┴───┴───┴───┘
    │   │   │   │
    │   │   │   └─ GND
    │   │   └─── +3.3V
    │   └───── I2C_CTRL_SDA
    └─────── I2C_CTRL_SCL

```

### 4. FX Connector (J9/J10)

Dual row header (2x10).

```text
       ┌───────────┐
  Dry_L│ 1       2 │ I2C_CTRL_SCL
  Wet_L│ 3       4 │ I2C_CTRL_SDA
  Dry_R│ 5       6 │ -5V
  Wet_R│ 7       8 │ +12V
    GND│ 9      10 │ +5V
       └───────────┘

```

