## Power Supply Architecture

The synthesizer requires three voltage rails for proper operation:

- **+12V** rail for the AS3397 chip  
- **+5V** rail for the microcontrollers, various digital ICs, and as the positive supply for the operational amplifiers (OpAmps)  
- **−5V** rail required by the AS3397 and as the negative supply for the OpAmps  

Several strategies have been considered for implementing the power supply. 

In the proposed design, we use a USB-C power supply unit that supports Power Delivery (PD) mode to provide a stable 12V/5A rail. 
	
A buck converter based on the TPS5430 chip, that can easily be obtained as an affordable off-the-shelf module, is used to generate the \SI{+5}{\volt} and \SI{-5}{\volt} rails. With a \SI{3}{\ampere} capacity, this setup is more than sufficient to power both the conductor board and the various voice boards.

Alternative configurations have also been explored:

- In early prototypes, a **compact ATX power supply** was used for +12V and +5V in combination with a **7805** regulator to generate the −5V rail.  
- In the **monophonic version**, the system is powered from a **standard 12V wall adapter** (or a USB-C PD unit), with a 7805 regulator providing the +5V rail, and **charge-pump circuits** used to derive the −5V rail.

Other solutions, such as **off-the-shelf AC-DC modules** from manufacturers like **Meanwell**, may also be considered for increased robustness and ease of integration — particularly when direct connection to the mains is desirable.
