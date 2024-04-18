# Monophonic Synth Based on the AS3397 Chip

Welcome to the hardware documentation for our monophonic synthesizer voice module, a core component of the polyUAnalog project. This document outlines the design and capabilities of the voice module, centered around the remarkable AS3397 chip.

## The AS3397 Chip

The AS3397 chip is manufactured by  ALFA RPAR, a Latvian semiconductor manufacturer. It's a modern reissue of the CEM3396, a chip that found its home in iconic synthesizers like the Oberheim Matrix-6. 

[Datasheet AS3397](AS3397_datasheet.pdf)

![AS3397 schematics](../../ressources/schema_AS3397.png)


## Sonic Possibilities of the AS3397



### Voltage-Controlled Oscillators (VCOs)

At the heart of the AS3397's sound generation capabilities are two Digital Controlled Oscillators (DCOs), which offer a range of sonic textures from saw waves to sharp, clipped triangles. The wave shaper allows for dynamic waveform manipulation, adding depth and variety to the sound. Additionally, the module can mix square waveforms from both DCOs.

- **Dual VCOs**: Two DCOs 
- **Wave Shaping**: offer a wide array of waveforms, from saw to clipped triangle, enhancing the module's versatility.
- **Balance Control**: The balance between the two VCOs can be adjusted, enabling fine-tuning of the voice's timbral characteristics.

### External Signal Addition

The AS3397 chip allows for the integration of an external audio signa.

### Voltage-Controlled Filter (VCF)

The VCF is a resonant 4-pole low-pass filter that shapes the sound by controlling its harmonic content. Its resonance capability can emphasize certain frequencies, adding a distinct character to the sound.

### Voltage-Controlled Amplifier (VCA)


## Driving the AS3397 Chip with an RP2040 (Pi Pico)

The integration of the AS3397 chip with the RP2040 microcontroller, particularly through the Pi Pico board, forms the backbone of our voice module's functionality. This setup allows for precise control over the analog synthesizer chip, leveraging digital technologies for enhanced musical expression and reliability.

### Discharge Pulse Generation for DCO

The RP2040 plays a crucial role in generating discharge pulses for the Digital Controlled Oscillator's (DCO) capacitor. This ensures the oscillator's frequency stability and precise pitch control, a testament to the seamless integration between analog sound generation and digital control.

### Managing Control Voltages

To fully harness the capabilities of the AS3397, managing numerous control voltages is imperative. Specifically, the chip requires management of 10 control voltages. While employing a Digital to Analog Converter (DAC) and multiplexers could achieve this, such a setup complicates the assembly and increases the overall cost. Instead, we opted for a more efficient approach, utilizing the RP2040's Pulse Width Modulation (PWM) outputs. These outputs are filtered through a low-pass filter to retain only the DC component, ensuring minimal ripple and response times that are well-suited for audio applications. The cut-off frequency of the filter has been meticulously chosen to balance between reducing ripple and maintaining an appropriate response time.

### Dynamic Ramp Voltage Control in DCO

Controlling the ramp voltage amplitude within a DCO presents its challenges. Our not so common approach involves continuously sampling the maximum voltage of the DCO's capacitor ramp using the RP2040's Analog to Digital Converters (ADCs). We then dynamically adjust this value using a digital Proportional-Integral-Derivative (PID) feedback loop, ensuring consistent DCO amplitude across different frequencies and hence the same waveform at the exit of the waveshaper.

### Digital Signal Oscillator (DSO)

The second core of the RP2040 is dedicated to generating an audio signal, functioning as a Digital Signal Oscillator (DSO). This digital audio signal is mixed with the analog signal from the AS3397 before reaching the Voltage-Controlled Filter (VCF), enriching the synthesizer's sonic palette.

### Voltage Level Shifting

Considering the AS3397 chip was designed in the 1980s to operate at +5V, while the RP2040 outputs 3.3V, level shifting is necessary. We've implemented level shifters to convert the 3.3V from the RP2040's PWM outputs to the 5V required by the AS3397, ensuring compatibility without sacrificing signal integrity.

### Control Voltage Scaling

For certain control voltages of the AS3397 that can accept broader ranges, we've employed operational amplifiers (Op-Amps) to scale the 0-3.3V output from the RP2040 to the required range. This approach allows for full utilization of the AS3397's control parameters without introducing unnecessary complexity.

### Power Supply

The power supply requirements (+12V, -5V for the AS3397, and +5V for the Pi Pico) are provided externally, for instance, by the conductor board. The Pi Pico employs its internal buck-boost converter to generate the appropriate operating voltage for the RP2040, streamlining the power management across the module.


### External Information Transfer via I²C

The transfer of information from external sources to the RP2040, which in turn controls the AS3397, is facilitated through the I²C bus emulating the MIDI protocol. This setup enables comprehensive control over various musical aspects, including note management (both 'note on' and 'note off'), velocity, aftertouch, control changes (MIDI CC), among other parameters. To surpass the traditional MIDI protocol's limitation of 127 levels, extended MIDI functionality has been integrated, providing greater nuance and control.

A interesting feature of our system is the automatic addressing of each voice board upon the conductor board's startup. Specifically, a voltage, derived by dividing the 3.3V generated by the conductor board and uniquely distributed to each voice board, is measured by the third ADC of the voice board's RP2040. This process assigns a specific I²C channel to each voice board, enabling precise and individualized control. At initialization, the conductor board scans all the connected I²C ports, thus determining the total polyphony capability of the synthesizer, seamlessly integrating hardware and digital controls.

