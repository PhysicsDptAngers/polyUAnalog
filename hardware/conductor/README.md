## Conductor Board


**NB:** What is presented here is, for now, a prototype version of the conductor board. A more refined version is currently in development. The voice boards are connected to the conductor board using veroboard (this part is still undocumented for now).

This board receives input from external sources, processes it, and distributes the relevant data to the voice boards.

The conductor board, built around the RP2040 microcontroller—specifically using the Pico-W prototyping board—receives musical input in MIDI format from various sources, processes it, and dispatches it to each voice via the I²C protocol (which also emulates standard MIDI messages).

The supported inputs are:
- A MIDI signal from the integrated MIDI ports.
- MIDI over Wi-Fi: the Pico-W runs a lightweight local web server that receives custom UDP packets encapsulating MIDI messages.
- MIDI messages from the controller board, typically used to modify synthesizer parameters. These MIDI messages also transit via the I²C bus.

