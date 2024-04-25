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

The AS3397 features a stereo Voltage Controlled Amplifier, which generates a current that must be converted into a corresponding voltage through a "transimpedance" circuit, typically one utilizing an Operational Amplifier (OpAmp).


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


## Deep Dive into the VCO Part of the Chip

### Digitally Controlled Oscillator (DCO)

The AS3397 features two oscillators that are of the DCO type. The operation of this type of oscillator is particularly well described in [Stargirl Flower's blog post on the oscillators of the Juno 6](https://blog.thea.codes/the-design-of-the-juno-dco/).

An oscillator must, almost by definition, produce a periodic signal. Here, it is obtained by charging a capacitor followed by its abrupt discharge. The discharge is triggered by a pulse from a digital circuit (typically a microcontroller in 2024). If the discharge signal is periodic, then the voltage across the capacitor will have the same periodicity, and we indeed have an oscillator.

#### Charging a Capacitor with Constant Current

The heartbeat within the oscillator is the charging of a capacitor with charges. Charging a capacitor might spontaneously make you think of an exponential curve that reaches the charging voltage after a time $\tau=RC$. Indeed, this is what happens when the capacitor is charged with a *constant voltage*. The more the capacitor is charged, the more it opposes the increase in its charge until reaching a status quo where the charging voltage equals the voltage generated by the charges accumulated on the capacitor's walls. This would be analogous to filling a water bottle that is upside down (i.e., with a neck pointing towards the ground into which a pipe delivering water comes). As the level rises, the hydrostatic pressure opposes the filling of the bottle until it equals the pressure in the pipe at the entrance of the neck.

Here, the charging of the capacitor is not at constant voltage but at *constant current*. Continuing the analogy started above, this is akin to filling a bottle "normally", i.e., without it being upside down. The water level inside the bottle increases linearly with time since nothing prevents its filling. Returning to the capacitor, when it is charged with constant current, its voltage increases linearly across its terminals. We thus obtain a voltage ramp over time.

##### The AS3397's Current Source

A very classic way to obtain a constant current charge of a capacitor is to include it in a feedback loop of an OpAmp whose linearity condition (V+=V-), forces the current to be constant within the RC series circuit.

The principle schematic of the AS3397 suggests that the chip uses another system to obtain a current source, namely a transistor placed in the feedback loop followed by a current mirror (CM).

As mentioned earlier, the operational amplifier in its linear regime acts to maintain a constant voltage difference between its inputs (positive and negative terminals). In the context of a current generator, a reference voltage is typically applied to one of the op-amp's inputs (here the + input). Here, this voltage is "WaveShape CV", noted $V_{wf}$ which, as its name indicates, will play a crucial role in controlling the waveform of the oscillator.

The resistor $R_{TA}$ sets the current. ([Reference](http://www.ecircuitcenter.com/Circuits/curr_src1/curr_src1.htm))

The other input of the op-amp, here the - input, is connected to ground via the resistor $R_{T}$, so that the linear operation of the OpAmp forces the voltage across $R_{T}$ to be equal to $V_{wf}$. The current within $R_{T}$, which is also the same as the one that will serve as input to the current mirror, is $I = V_{wf}/R_{T}$.

The OpAmp's feedback loop contains a transistor. More specifically, the OpAmp's output is connected to the transistor's base, and the output voltage of the OpAmp thus controls the current flowing between the base and the emitter. Indeed, the base-emitter part of the transistor (here NPN), behaves like a diode where the base current $I_{B}$ varies exponentially with the base-emitter voltage $V_{BE}$. Furthermore, the collector current $I_{C}$ traversing the transistor, which is the same as the current $I$ traversing $R_{T}$, depends linearly on the base current (via the current gain $\beta$). In total, a modification of the OpAmp's output voltage translates into a modification of the base voltage $V_{BE}$ and thus of the collector current $I_{C}$ which traverses $R_{T}$. The operation of the OpAmp requires the simultaneous achievement of two conditions:
- The voltage across $R_{T}$ must be equal to $V_{wf}$, setting a current $I=V_{wf}/R_{T}$
- The OpAmp's output voltage, which drives the transistor via its base, must be such that the current in the collector also equals $I$

In summary, the choice of the $V_{wf}$ voltage and the $R_{T}$ resistor sets a stable current $I$ that, at least in theory, does not depend on the load in which the current flows. Thus, we are indeed dealing with a current generator. The load is typically placed between the collector and the base of the transistor. The AS3397 uses a current mirror before sending this copy of the current to the capacitor to be charged.

##### Explanation of a Current Mirror

The somewhat schematic view of the AS3397 does not allow us to know the exact nature of the current mirror used here. Nevertheless, we can revisit the principle of a current mirror by taking the simplest example: connecting two transistors, T1 and T2 (as identical as possible), in the manner presented in Figure X. Since the two transistors share the same voltage between the base and the emitter, the output current passing through the collector of T2 is the same as the input current passing through the collector of T1.



https://www.analog.com/en/resources/analog-dialogue/articles/current-output-circuit-techniques-add-versatility.html


Transconductance amplifier

SChéma en Falstad


#### Capacitor Discharge

In a DCO, the capacitor must be discharged at a frequency that defines the pitch of the note to be played. The AS3397 follows the traditional scheme, which involves using a transistor acting as a switch controlled by the microcontroller. If the control voltage sent to the base of the transistor is zero, the transistor remains open, and the capacitor charges from the current source. If the control voltage rises to a high level (here 3.3V), the transistor closes, connecting both terminals of the capacitor to the ground, causing it to discharge almost instantly.

The arrangement shown in the AS3397 datasheet includes some additional components. The discharge pulse (symbolized by a star) first passes through a high-pass filter acting as a derivative filter. Indeed, the discharge pulse must have the correct duration, specifically:
- Not too short, to ensure the capacitor fully discharges.
- Not too long, to allow the capacitor to begin charging again as soon as it is empty.

TODO: Create a diagram.

Typical components from the 1980s produced pulses that were too long, and the derivative filter was intended to create a shorter pulse. Perhaps as a reminder, a high-pass RC filter acts as a differentiator when the frequency of the signal (its fundamental) is much higher than the cutoff frequency. The transfer function is then simply \( H(\omega) = j\omega \), that is, the operation of deriving the signal.

Nowadays, the derivative assembly is no longer very useful because the pulses produced by microcontrollers can be particularly short. For example, with the RP2040 @ 125MHz, pulses can have a duration of 8ns.

After the derivative RC filter, there is a resistor before the base of the bipolar transistor intended to ensure that, given the voltage of the pulses, the base current is sufficient to open the transistor.

In an effort to reduce the number of components, we have chosen to simplify this part of the assembly:
- By removing the derivative part and directly setting the adequate duration of the pulses generated by the RP2040.
- By using a MOSFET transistor instead of a bipolar one, allowing it to be driven directly by voltage and eliminating the need for the bias resistor.

We have chosen the BS170 transistor, which is easy to source, very inexpensive, and especially has a Gate Threshold Voltage (VGS(th)) of about 2V, which is below the 3.3V, ensuring that the 3.3V pulses from the RP2040 can trigger its opening.

Note that the resistance of the BS170 when open (Static Drain−Source On−Resistance, typically noted as RDS(ON)) is not negligible and is about 1.5 Ohms here. This should be taken into account during the discharge of the capacitor and makes this process less instantaneous. Figure x shows the time required for the total discharge of the capacitor. This figure also indicates that the duration of the pulse fully allows the capacitor to discharge completely, even considering the increased time due to this residual RDS(ON) resistance.


### Washapper et contrôle de la forme d'onde

COmme dit plus haut, les DCO permettent d'avoir une fréquence parfaitement fixe (sans avoir d'ailleurs à se préocuper des convertiseur exponentielle  de tension de contrôle de la fréquence puisque cela est géré en software par le micro-controleur).
Cela créé cependant un problème : l'amplitude de la rampe de tension générée depend de la fréquence. En effet, pour le temps de charge du condensateur pour les basses fréquences est plus long que pour les hautes fréquences. Et donc, tout chose égale par ailleurs, la tension atteinte par la rampe, et donc le volume du VCO, est plus grande dans les basses fréquences que dans les hautes fréquences.

TODO un graphe d'explication avt et après contrôle de la tension de charge.

Ce problème est d'autant plus problèmatique que, dans la puce AS397, la rampe de tension passe dans un circuit de waveshapper afin d'avoir une plus grande palette de forme d'onde.

Laissons pour l'instant de coté la gestion de l'amplitude de la rampe de tension et penchons nous sûr le washapper. La datasheet ne décrit pas ses composants internes mais donne son fonctionnement qui est résumé sur la figure X pour une tension d'alimentation de l'AS3397 de 12V.

Pour les tensions de rampe aux bornes inférieure le waveshapper à un comportement linéaire et l'onde en sortie reste en dent de scie. Pour les tensions comprises entre XV et YV, le waveshapper inverse la direction de la rampe ce qui permet d'obtenir une rampe montante complète suivie d'une partie de rampe descendante. Pour une tension de rampe de ZV, la partie descedante dure aussi longtemps que la partie montante : on a une onde trinagulaire, pour une tension de rampe supérieure à ZV, le waveshapper la transforme en tension nulle. La forme d'onde en sortie est donc ce que la datasheet qualifie de "clipped " triangle. Au delà de ZV, la partie à 0V de la forme d'onde grandit et on se rappoche d'une impulsion.

Le contrôle de la tension max de la rampe en sortie du condensateur a donc un double problématique :
- Il faut que cette tension max soit la même pour toutes les fréquences sinon la forme d'onde (i.e. le timbre) ne sera pas le même d'une note à l'autre.
- Il faut regler la tension maximale de la rampe du condensateur pour avoir la forme d'onde souhaitée en sortie du waveshapper.

### Le comparateur et les formes d'onde carrées

### Le quad mixer
