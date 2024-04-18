from machine import Pin, I2C, ADC, Timer
import rp2
from mcp23017 import MCP23017
from neopixel import Neopixel
from rotary import Rotary
import ssd1306
from Midi_CC import *
from time import sleep
import _thread

from param import Parameter, Osc_shape, Osc_pwm, level, Osc_pitch






# Initialize I2C for MCP23017
i2c = I2C(0, scl=Pin(1), sda=Pin(0))

# Initialize I2C for Midi over I2C
i2c_midi = I2C(1, scl=Pin(3), sda=Pin(2))

#Initialize OLED screen
display_oled = ssd1306.SSD1306_I2C(128, 64, i2c)


display_oled.fill(0)
display_oled.fill_rect(0, 0, 32, 32, 1)
display_oled.fill_rect(2, 2, 28, 28, 0)
display_oled.vline(9, 8, 22, 1)
display_oled.vline(16, 2, 22, 1)
display_oled.vline(23, 8, 22, 1)
display_oled.fill_rect(26, 24, 2, 4, 1)
display_oled.text('poyUAnalog', 40, 0, 1)
display_oled.text('v0.1', 40, 12, 1)
display_oled.text('04/2024', 40, 24, 1)
display_oled.show()




"""
# Initialize MCP23017 devices and configure interrupts
mcp_devices = []
for i in range(8):
    address = 0x20 + i
    mcp = MCP23017(i2c, address)
    mcp.config(interrupt_mirror=True, interrupt_polarity=1)
    mcp.mode = 0xFFFF  # Set all pins as inputs
    mcp.pullup = 0xFFFF  # Enable pull-ups on all pins
    mcp.interrupt_enable = 0xFFFF  # Enable interrupts on all pins
    mcp_devices.append(mcp)
"""
is_mcpA_plugged = False
try:
    address = 0x24	# Harcoded
    mcpA = MCP23017(i2c, address)
    mcpA.config(interrupt_mirror=True, interrupt_polarity=1)
    mcpA.mode = 0xFFFF  # Set all pins as inputs
    mcpA.pullup = 0xFFFF  # Enable pull-ups on all pins
    mcpA.interrupt_enable = 0xFFFF  # Enable interrupts on all pins
    is_mcpA_plugged = True
except Exception as e:
    # Si une exception est levée, le périphérique n'est pas présent
    print("Erreur lors de l'initialisation du MCP23017 A :", e)

is_mcpB_plugged = False
try:
    address = 0x21
    mcpB = MCP23017(i2c, address)
    mcpB.config(interrupt_mirror=True, interrupt_polarity=1)
    mcpB.mode = 0xFFFF  # Set all pins as inputs
    mcpB.pullup = 0xFFFF  # Enable pull-ups on all pins
    mcpB.interrupt_enable = 0xFFFF  # Enable interrupts on all pins
    is_mcpA_plugged = True
except Exception as e:
    # Si une exception est levée, le périphérique n'est pas présent
    print("Erreur lors de l'initialisation du MCP23017 B :", e)    
"""
address = 0x23
mcpC = MCP23017(i2c, address)
mcpC.config(interrupt_mirror=True, interrupt_polarity=1)
mcpC.mode = 0xFFFF  # Set all pins as inputs
mcpC.pullup = 0xFFFF  # Enable pull-ups on all pins
mcpC.interrupt_enable = 0xFFFF  # Enable interrupts on all pins
"""
"""
address = 0x24
mcpD = MCP23017(i2c, address)
mcpD.config(interrupt_mirror=True, interrupt_polarity=1)
mcpD.mode = 0xFFFF  # Set all pins as inputs
mcpD.pullup = 0xFFFF  # Enable pull-ups on all pins
mcpD.interrupt_enable = 0xFFFF  # Enable interrupts on all pins
"""
"""
address = 0x25
mcpE = MCP23017(i2c, address)
mcpE.config(interrupt_mirror=True, interrupt_polarity=1)
mcpE.mode = 0xFFFF  # Set all pins as inputs
mcpE.pullup = 0xFFFF  # Enable pull-ups on all pins
mcpE.interrupt_enable = 0xFFFF  # Enable interrupts on all pins
"""

# LFO
is_mcpF_plugged = False
try:
    address = 0x20
    mcpF = MCP23017(i2c, address)
    mcpF.config(interrupt_mirror=True, interrupt_polarity=1)
    mcpF.mode = 0xFFFF  # Set all pins as inputs
    mcpF.pullup = 0xFFFF  # Enable pull-ups on all pins
    mcpF.interrupt_enable = 0xFFFF  # Enable interrupts on all pins
    is_mcpF_plugged = True
except Exception as e:
    # Si une exception est levée, le périphérique n'est pas présent
    print("Erreur lors de l'initialisation du MCP23017 B :", e)       

"""
address = 0x27
mcpG = MCP23017(i2c, address)
mcpG.config(interrupt_mirror=True, interrupt_polarity=1)
mcpG.mode = 0xFFFF  # Set all pins as inputs
mcpG.pullup = 0xFFFF  # Enable pull-ups on all pins
mcpG.interrupt_enable = 0xFFFF  # Enable interrupts on all pins
"""

"""
address = 0x28
mcpH = MCP23017(i2c, address)
mcpH.config(interrupt_mirror=True, interrupt_polarity=1)
mcpH.mode = 0xFFFF  # Set all pins as inputs
mcpH.pullup = 0xFFFF  # Enable pull-ups on all pins
mcpH.interrupt_enable = 0xFFFF  # Enable interrupts on all pins
"""

# FunctionS to handle interrupts from MCP23017
# Most of the funtionalities are HARDCODED and correspond to OUR controler
def handle_mcp_interrupt_A(pin):
    """
    Controller for OSCA with 4 encoders/push-Button
    MCP pinout :
    - A0 and A1 : encoder for oscA waveform (bit : 0 and 1)
    - A2 : buton for waveform mute (bit : 2)
    - A3 and A4 : encoder for oscA PWM 
    - A5 : button for PWM mute
    - A6 : not connected
    - A7 : NA (SW5)
    - B0 and B1 : NA (Enc 5)
    - B2 : mute for level
    - B3 and B4 : encoder for level
    - B5 : button fine/coarse for pitch
    - B6 and B7 : encoder for pitch
    """
    flagged = mcpA.interrupt_flag	# which pin has triggered the IRQ
    captured = mcpA.interrupt_captured	# what are the captured state of the 16 pins at the moment of the IRQ
    
    # Checking if the set bit is in the high byte (bits 8 to 15)
    if flagged > 0xFF:
        # Checking the upper half of the high byte (bits 12 to 15)
        if flagged > 0xFFF:
            if flagged > 0x3FFF:
                # Bit_position is 15 of 14, that's B6 and B7
                clk = (captured >> 15) & 1
                dt =  (captured >> 14) & 1
                p_OscA_Pitch._process_rotary_pins(clk, dt)
                to_be_updated.append(p_OscA_Pitch)
            else:
                if flagged >= 0x2000:
                    #bit_position = 13 -> B5
                    val = (captured >> 13) & 1
                    p_OscA_Pitch.button_pressed(val)
                    to_be_updated.append(p_OscA_Pitch)
                else:
                    #bit_position = 12 -> B4
                    clk = (captured >> 11) & 1
                    dt =  (captured >> 12) & 1
                    p_OscA_level._process_rotary_pins(clk, dt)
                    to_be_updated.append(p_OscA_level)
        
        # Checking the lower half of the high byte (bits 8 to 11)
        else:
            if flagged > 0x3FF:
                if flagged >= 0x800:
                    # bit_position = 11 -> B3
                    clk = (captured >> 11) & 1
                    dt =  (captured >> 12) & 1
                    p_OscA_level._process_rotary_pins(clk, dt)
                    to_be_updated.append(p_OscA_level)
                else:
                    # bit_position = 10 -> B2
                    val = (captured >> 10) & 1
                    p_OscA_level.button_pressed(val)
                    to_be_updated.append(p_OscA_level)
            
            # B0 and B1 -> not used here
            #else:
            #    bit_position = 9 if flagged >= 0x200 else 8
    # Checking if the set bit is in the low byte (bits 0 to 7)
    else:
        # Checking the upper half of the low byte (bits 4 to 7)
        if flagged > 0xF:
            
            if flagged > 0x3F:
                # Not used here
                bit_position = 7 if flagged >= 0x80 else 6
            else:
                if flagged >= 0x20:
                    # bit_position = 5
                    val = (captured >> 5) & 1
                    p_OscA_PWM.button_pressed(val)
                    to_be_updated.append(p_OscA_PWM)
                else:
                    # bit_position = 4
                    clk = (captured >> 3) & 1
                    dt =  (captured >> 4) & 1
                    p_OscA_PWM._process_rotary_pins(clk, dt)
                    to_be_updated.append(p_OscA_PWM)
        # Checking the lower half of the low byte (bits 0 to 3)
        else:
            if flagged > 0x3:
                if flagged >= 0x8:
                    # bit_position = 3
                    clk = (captured >> 3) & 1
                    dt =  (captured >> 4) & 1
                    p_OscA_PWM._process_rotary_pins(clk, dt)
                    to_be_updated.append(p_OscA_PWM)
                else:
                    # bit_position = 3
                    val = (captured >> 2) & 1
                    p_OscA_waveform.button_pressed(val)
                    to_be_updated.append(p_OscA_waveform)
            else:
                # bit 0 and  1
                clk = (captured >> 0) & 1
                dt =  (captured >> 1) & 1
                p_OscA_waveform._process_rotary_pins(clk, dt)
                to_be_updated.append(p_OscA_waveform)

def handle_mcp_interrupt_B(pin):
    """
    Controller for OSCB with 4 encoders/push-Button
    MCP pinout :
    - A0 and A1 : encoder for oscA waveform
    - A2 : buton for waveform mute
    - A3 and A4 : encoder for oscA PWM
    - A5 : button for PWM mute
    - A6 : not connected
    - A7 : NA (SW5)
    - B0 and B1 : NA (Enc 5)
    - B2 : mute for level
    - B3 and B4 : encoder for level
    - B5 : button fine/coarse for pitch
    - B6 and B7 : encoder for pitch
    """
    flagged = mcpB.interrupt_flag	# which pin has triggered the IRQ
    captured = mcpB.interrupt_captured	# what are the captured state of the 16 pins at the moment of the IRQ
    
    # Checking if the set bit is in the high byte (bits 8 to 15)
    if flagged > 0xFF:
        # Checking the upper half of the high byte (bits 12 to 15)
        if flagged > 0xFFF:
            if flagged > 0x3FFF:
                # Bit_position is 15 of 14, that's B6 and B7
                clk = (captured >> 15) & 1
                dt =  (captured >> 14) & 1
                p_OscB_Pitch._process_rotary_pins(clk, dt)
                to_be_updated.append(p_OscB_Pitch)
            else:
                if flagged >= 0x2000:
                    #bit_position = 13 -> B5
                    val = (captured >> 13) & 1
                    p_OscB_Pitch.button_pressed(val)
                    to_be_updated.append(p_OscB_Pitch)
                else:
                    #bit_position = 12 -> B4
                    clk = (captured >> 11) & 1
                    dt =  (captured >> 12) & 1
                    p_OscB_level._process_rotary_pins(clk, dt)
                    to_be_updated.append(p_OscB_level)
        
        # Checking the lower half of the high byte (bits 8 to 11)
        else:
            if flagged > 0x3FF:
                if flagged >= 0x800:
                    # bit_position = 11 -> B3
                    clk = (captured >> 11) & 1
                    dt =  (captured >> 12) & 1
                    p_OscB_level._process_rotary_pins(clk, dt)
                    to_be_updated.append(p_OscB_level)
                else:
                    # bit_position = 10 -> B2
                    val = (captured >> 10) & 1
                    p_OscB_level.button_pressed(val)
                    to_be_updated.append(p_OscB_level)
            
            # B0 and B1 -> not used here
            #else:
            #    bit_position = 9 if flagged >= 0x200 else 8
    # Checking if the set bit is in the low byte (bits 0 to 7)
    else:
        # Checking the upper half of the low byte (bits 4 to 7)
        if flagged > 0xF:
            
            if flagged > 0x3F:
                # Not used here
                bit_position = 7 if flagged >= 0x80 else 6
            else:
                if flagged >= 0x20:
                    # bit_position = 5
                    val = (captured >> 5) & 1
                    p_OscB_PWM.button_pressed(val)
                    to_be_updated.append(p_OscB_PWM)
                else:
                    # bit_position = 4
                    clk = (captured >> 3) & 1
                    dt =  (captured >> 4) & 1
                    p_OscB_PWM._process_rotary_pins(clk, dt)
                    to_be_updated.append(p_OscB_PWM)
        # Checking the lower half of the low byte (bits 0 to 3)
        else:
            if flagged > 0x3:
                if flagged >= 0x8:
                    # bit_position = 3
                    clk = (captured >> 3) & 1
                    dt =  (captured >> 4) & 1
                    p_OscB_PWM._process_rotary_pins(clk, dt)
                    to_be_updated.append(p_OscB_PWM)
                else:
                    # bit_position = 3
                    val = (captured >> 2) & 1
                    p_OscB_waveform.button_pressed(val)
                    to_be_updated.append(p_OscB_waveform)
            else:
                # bit 0 and  1
                clk = (captured >> 0) & 1
                dt =  (captured >> 1) & 1
                p_OscB_waveform._process_rotary_pins(clk, dt)
                to_be_updated.append(p_OscB_waveform)


def handle_mcp_interrupt_C(pin):
    """
    Controller for Filter with 6 encoders with 4 push-Button
    MCP pinout :
    - A0  : SW5 : NA
    - A1 and A2: Enc 5 : Amount
    - A3 : SW Coarse/Fine attack
    - A4 and A5 : Enc Attack
    - A6 and A7 : Enc Decay
    - B0  : SW4 : Coarse/Fine Release
    - B1 and B2 : Enc Release
    - B3 and B4 : Enc Sustain
    - B5 : SW6 on/off Filter FM
    - B6 and B7 : encoder for FM
    """
    flagged = mcpC.interrupt_flag	# which pin has triggered the IRQ
    captured = mcpC.interrupt_captured	# what are the captured state of the 16 pins at the moment of the IRQ
    
    # Checking if the set bit is in the high byte (bits 8 to 15)
    if flagged > 0xFF:
        # Checking the upper half of the high byte (bits 12 to 15)
        if flagged > 0xFFF:
            if flagged > 0x3FFF:
                clk = (captured >> 14) & 1
                dt =  (captured >> 15) & 1
                p_VCF_FM._process_rotary_pins(clk, dt)
            else:
                if flagged >= 0x2000:
                    # bit_position = 13
                    val = (captured >> 13) & 1
                    p_VCF_FM.button_pressed(val)
                else:
                    # bit_position = 12
                    clk = (captured >> 12) & 1
                    dt =  (captured >> 11) & 1
                    p_VCF_S._process_rotary_pins(clk, dt)
                    
        # Checking the lower half of the high byte (bits 8 to 11)
        else:
            if flagged > 0x3FF:
                if flagged >= 0x800:
                    # bit_position = 11
                    clk = (captured >> 12) & 1
                    dt =  (captured >> 11) & 1
                    p_VCF_S._process_rotary_pins(clk, dt)
                    
                else:
                    # bit_position = 10
                    clk = (captured >> 9) & 1
                    dt =  (captured >> 10) & 1
                    p_VCF_R._process_rotary_pins(clk, dt)
            else:
                if flagged >= 0x200:
                    # bit_position = 9
                    clk = (captured >> 9) & 1
                    dt =  (captured >> 10) & 1
                    p_VCF_R._process_rotary_pins(clk, dt)
                else:
                    # bit_position = 8
                    val = (captured >> 8) & 1
                    p_VCF_R.button_pressed(val)
                    
    # Checking if the set bit is in the low byte (bits 0 to 7)
    else:
        # Checking the upper half of the low byte (bits 4 to 7)
        if flagged > 0xF:
            if flagged > 0x3F:
                clk = (captured >> 7) & 1
                dt =  (captured >> 6) & 1
                p_VCF_D._process_rotary_pins(clk, dt)
            else:
                clk = (captured >> 7) & 1
                dt =  (captured >> 6) & 1
                p_VCF_A._process_rotary_pins(clk, dt)
        # Checking the lower half of the low byte (bits 0 to 3)
        else:
            if flagged > 0x3:
                if flagged >= 0x8:
                    #bit_position = 3
                    val = (captured >> 3) & 1
                    p_VCF_A.button_pressed(val)
                else:
                    #bit_position = 2
                    clk = (captured >> 1) & 1
                    dt =  (captured >> 2) & 1
                    p_VCF_Amount._process_rotary_pins(clk, dt)
            else:
                if flagged >= 0x2:
                    clk = (captured >> 1) & 1
                    dt =  (captured >> 2) & 1
                    p_VCF_Amount._process_rotary_pins(clk, dt)
                else:
                    pass
    
def handle_mcp_interrupt_D(pin):
    """
    Controller for VCA with 6 encoders with 4 push-Button
    MCP pinout :
    MCP pinout :
    - A0  : SW5 : NA
    - A1 and A2: Enc 5 : Amount
    - A3 : SW Coarse/Fine attack
    - A4 and A5 : Enc Attack
    - A6 and A7 : Enc Decay
    - B0  : SW4 : Coarse/Fine Release
    - B1 and B2 : Enc Release
    - B3 and B4 : Enc Sustain
    - B5 : SW6 MUTE master
    - B6 and B7 : encoder Master
    """
    flagged = mcpD.interrupt_flag	# which pin has triggered the IRQ
    captured = mcpD.interrupt_captured	# what are the captured state of the 16 pins at the moment of the IRQ
    
    
    # Checking if the set bit is in the high byte (bits 8 to 15)
    if flagged > 0xFF:
        # Checking the upper half of the high byte (bits 12 to 15)
        if flagged > 0xFFF:
            if flagged > 0x3FFF:
                clk = (captured >> 14) & 1
                dt =  (captured >> 15) & 1
                p_VCA_Master._process_rotary_pins(clk, dt)
            else:
                if flagged >= 0x2000:
                    # bit_position = 13
                    val = (captured >> 13) & 1
                    p_VCA_Master.button_pressed(val)
                else:
                    # bit_position = 12
                    clk = (captured >> 12) & 1
                    dt =  (captured >> 11) & 1
                    p_VCA_S._process_rotary_pins(clk, dt)
                    
        # Checking the lower half of the high byte (bits 8 to 11)
        else:
            if flagged > 0x3FF:
                if flagged >= 0x800:
                    # bit_position = 11
                    clk = (captured >> 12) & 1
                    dt =  (captured >> 11) & 1
                    p_VCA_S._process_rotary_pins(clk, dt)
                    
                else:
                    # bit_position = 10
                    clk = (captured >> 9) & 1
                    dt =  (captured >> 10) & 1
                    p_VCA_R._process_rotary_pins(clk, dt)
            else:
                if flagged >= 0x200:
                    # bit_position = 9
                    clk = (captured >> 9) & 1
                    dt =  (captured >> 10) & 1
                    p_VCA_R._process_rotary_pins(clk, dt)
                else:
                    # bit_position = 8
                    val = (captured >> 8) & 1
                    p_VCA_R.button_pressed(val)
                    
    # Checking if the set bit is in the low byte (bits 0 to 7)
    else:
        # Checking the upper half of the low byte (bits 4 to 7)
        if flagged > 0xF:
            if flagged > 0x3F:
                clk = (captured >> 7) & 1
                dt =  (captured >> 6) & 1
                p_VCA_D._process_rotary_pins(clk, dt)
            else:
                clk = (captured >> 7) & 1
                dt =  (captured >> 6) & 1
                p_VCA_A._process_rotary_pins(clk, dt)
        # Checking the lower half of the low byte (bits 0 to 3)
        else:
            if flagged > 0x3:
                if flagged >= 0x8:
                    #bit_position = 3
                    val = (captured >> 3) & 1
                    p_VCA_A.button_pressed(val)
                else:
                    #bit_position = 2
                    clk = (captured >> 1) & 1
                    dt =  (captured >> 2) & 1
                    p_VCA_Amount._process_rotary_pins(clk, dt)
            else:
                if flagged >= 0x2:
                    clk = (captured >> 1) & 1
                    dt =  (captured >> 2) & 1
                    p_VCA_Amount._process_rotary_pins(clk, dt)
                else:
                    pass

def handle_mcp_interrupt_E(pin):
    """
    Controller for Enveloppe with 6 encoders with 4 push-Button
    MCP pinout :
    - A0 and A1 : encoder for oscA waveform
    - A2 : buton for waveform mute
    - A3 and A4 : encoder for oscA PWM
    - A5 : button for PWM mute
    - A6 : not connected
    - A7 : NA (SW5)
    - B0 and B1 : NA (Enc 5)
    - B2 : mute for level
    - B3 and B4 : encoder for level
    - B5 : button fine/coarse for pitch
    - B6 and B7 : encoder for pitch
    """
    flagged = mcpE.interrupt_flag	# which pin has triggered the IRQ
    captured = mcE.interrupt_captured	# what are the captured state of the 16 pins at the moment of the IRQ
    
    # Checking if the set bit is in the high byte (bits 8 to 15)
    if flagged > 0xFF:
        # Checking the upper half of the high byte (bits 12 to 15)
        if flagged > 0xFFF:
            if flagged > 0x3FFF:
                clk = (captured >> 14) & 1
                dt =  (captured >> 15) & 1
                p_Env_Dest._process_rotary_pins(clk, dt)
            else:
                if flagged >= 0x2000:
                    # bit_position = 13
                    val = (captured >> 13) & 1
                    p_Env_Dest.button_pressed(val)
                else:
                    # bit_position = 12
                    clk = (captured >> 12) & 1
                    dt =  (captured >> 11) & 1
                    p_Env_S._process_rotary_pins(clk, dt)
                    
        # Checking the lower half of the high byte (bits 8 to 11)
        else:
            if flagged > 0x3FF:
                if flagged >= 0x800:
                    # bit_position = 11
                    clk = (captured >> 12) & 1
                    dt =  (captured >> 11) & 1
                    p_Env_S._process_rotary_pins(clk, dt)
                    
                else:
                    # bit_position = 10
                    clk = (captured >> 9) & 1
                    dt =  (captured >> 10) & 1
                    p_Env_R._process_rotary_pins(clk, dt)
            else:
                if flagged >= 0x200:
                    # bit_position = 9
                    clk = (captured >> 9) & 1
                    dt =  (captured >> 10) & 1
                    p_Env_R._process_rotary_pins(clk, dt)
                else:
                    # bit_position = 8
                    val = (captured >> 8) & 1
                    p_Env_R.button_pressed(val)
                    
    # Checking if the set bit is in the low byte (bits 0 to 7)
    else:
        # Checking the upper half of the low byte (bits 4 to 7)
        if flagged > 0xF:
            if flagged > 0x3F:
                clk = (captured >> 7) & 1
                dt =  (captured >> 6) & 1
                p_Env_D._process_rotary_pins(clk, dt)
            else:
                clk = (captured >> 7) & 1
                dt =  (captured >> 6) & 1
                p_Env_A._process_rotary_pins(clk, dt)
        # Checking the lower half of the low byte (bits 0 to 3)
        else:
            if flagged > 0x3:
                if flagged >= 0x8:
                    #bit_position = 3
                    val = (captured >> 3) & 1
                    p_Env_A.button_pressed(val)
                else:
                    #bit_position = 2
                    clk = (captured >> 1) & 1
                    dt =  (captured >> 2) & 1
                    p_Env_Amount._process_rotary_pins(clk, dt)
            else:
                if flagged >= 0x2:
                    clk = (captured >> 1) & 1
                    dt =  (captured >> 2) & 1
                    p_Env_Amount._process_rotary_pins(clk, dt)
                else:
                    pass
    
def handle_mcp_interrupt_F(pin):
    """
    Controller for LFO with 5 encoders/push-Button
    MCP pinout :
    - A0 and A1 : encoder for LFO Shape
    - A2 : buton for LFO shape
    - A3 and A4 : encoder for Rate
    - A5 : button Rate
    - A6 : not connected
    - A7 : Button Dest 2
    - B0 and B1 : Encoder Dest 2
    - B2 : Button for dest 1
    - B3 and B4 : Encoder for Dest 1
    - B5 : button for Amount
    - B6 and B7 : encoder for Amount
    """
    flagged = mcpF.interrupt_flag	# which pin has triggered the IRQ
    captured = mcpF.interrupt_captured	# what are the captured state of the 16 pins at the moment of the IRQ
    
    
    # Checking if the set bit is in the high byte (bits 8 to 15)
    if flagged > 0xFF:
        # Checking the upper half of the high byte (bits 12 to 15)
        if flagged > 0xFFF:
            if flagged > 0x3FFF:
                # Bit_position is 15 of 14, that's B6 and B7
                clk = (captured >> 15) & 1
                dt =  (captured >> 14) & 1
                p_LFO_amount._process_rotary_pins(clk, dt)
                to_be_updated.append(p_LFO_amount)
            else:
                if flagged >= 0x2000:
                    #bit_position = 13 -> B5
                    val = (captured >> 13) & 1
                    p_LFO_amount.button_pressed(val)
                    to_be_updated.append(p_LFO_amount)
                else:
                    #bit_position = 12 -> B4
                    clk = (captured >> 11) & 1
                    dt =  (captured >> 12) & 1
                    p_LFO_Dest_1._process_rotary_pins(clk, dt)
                    to_be_updated.append(p_LFO_Dest_1)
        
        # Checking the lower half of the high byte (bits 8 to 11)
        else:
            if flagged > 0x3FF:
                if flagged >= 0x800:
                    # bit_position = 11 -> B3
                    clk = (captured >> 11) & 1
                    dt =  (captured >> 12) & 1
                    p_LFO_Dest_1._process_rotary_pins(clk, dt)
                    to_be_updated.append(p_LFO_Dest_1)
                else:
                    # bit_position = 10 -> B2
                    val = (captured >> 10) & 1
                    p_LFO_Dest_1.button_pressed(val)
                    to_be_updated.append(p_LFO_Dest_1)
                        
            else:
                clk = (captured >> 8) & 1
                dt =  (captured >> 9) & 1
                p_LFO_Dest_2._process_rotary_pins(clk, dt)
                to_be_updated.append(p_LFO_Dest_2)
                    
    # Checking if the set bit is in the low byte (bits 0 to 7)
    else:
        # Checking the upper half of the low byte (bits 4 to 7)
        if flagged > 0xF:
            
            if flagged > 0x3F:
                #bit_position = 7 if flagged >= 0x80 else 6
                val = (captured >> 7) & 1
                p_LFO_Dest_1.button_pressed(val)
                to_be_updated.append(p_LFO_Dest_1)
                to_be_updated.append(p_LFO_Dest_2)
            else:
                if flagged >= 0x20:
                    # bit_position = 5
                    val = (captured >> 5) & 1
                    p_LFO_rate.button_pressed(val)
                    to_be_updated.append(p_LFO_rate)
                else:
                    # bit_position = 4
                    clk = (captured >> 3) & 1
                    dt =  (captured >> 4) & 1
                    p_LFO_rate._process_rotary_pins(clk, dt)
                    to_be_updated.append(p_LFO_rate)
        # Checking the lower half of the low byte (bits 0 to 3)
        else:
            if flagged > 0x3:
                if flagged >= 0x8:
                    # bit_position = 3
                    clk = (captured >> 3) & 1
                    dt =  (captured >> 4) & 1
                    p_LFO_rate._process_rotary_pins(clk, dt)
                    to_be_updated.append(p_LFO_rate)
                else:
                    # bit_position = 3
                    val = (captured >> 2) & 1
                    p_LFO_shape.button_pressed(val)
                    to_be_updated.append(p_LFO_shape)
            else:
                # bit 0 and  1
                clk = (captured >> 0) & 1
                dt =  (captured >> 1) & 1
                p_LFO_shape._process_rotary_pins(clk, dt)
                to_be_updated.append(p_LFO_shape)
                
def handle_mcp_interrupt_G(pin):
    """
    Controller for DSO with 4 encoders/push-Button
    MCP pinout :
    - A0 and A1 : encoder for oscA waveform
    - A2 : buton for waveform mute
    - A3 and A4 : encoder for oscA PWM
    - A5 : button for PWM mute
    - A6 : not connected
    - A7 : NA (SW5)
    - B0 and B1 : NA (Enc 5)
    - B2 : mute for level
    - B3 and B4 : encoder for level
    - B5 : button fine/coarse for pitch
    - B6 and B7 : encoder for pitch
    """
    flagged = mcp.interrupt_flag	# which pin has triggered the IRQ
    captured = mcp.interrupt_captured	# what are the captured state of the 16 pins at the moment of the IRQ
    
    
def handle_mcp_interrupt_H(pin):
    """
    Controller for POLY and MODE with 4 encoders/push-Button
    MCP pinout :
    - A0 and A1 : encoder for oscA waveform
    - A2 : buton for waveform mute
    - A3 and A4 : encoder for oscA PWM
    - A5 : button for PWM mute
    - A6 : not connected
    - A7 : NA (SW5)
    - B0 and B1 : NA (Enc 5)
    - B2 : mute for level
    - B3 and B4 : encoder for level
    - B5 : button fine/coarse for pitch
    - B6 and B7 : encoder for pitch
    """
    flagged = mcp.interrupt_flag	# which pin has triggered the IRQ
    captured = mcp.interrupt_captured	# what are the captured state of the 16 pins at the moment of the IRQ
    

handle_mcp_interrupt = [handle_mcp_interrupt_A, handle_mcp_interrupt_B, handle_mcp_interrupt_C, handle_mcp_interrupt_D, handle_mcp_interrupt_E, handle_mcp_interrupt_F, handle_mcp_interrupt_G, handle_mcp_interrupt_H]
# RP2040 GPIO pins for MCP23017 interrupts
i = 0
for pin_num in range(4, 12):
    pin = Pin(pin_num, Pin.IN, Pin.PULL_DOWN)
    pin.irq(trigger=Pin.IRQ_RISING, handler=handle_mcp_interrupt[i])
    i += 1



for pin_num in range(13, 21):
    led_pin = Pin(pin_num, Pin.OUT) 

neopixels_A = Neopixel(num_leds=4, state_machine=0, pin=12, mode="RGB")
neopixels_B = Neopixel(num_leds=4, state_machine=1, pin=13, mode="RGB")
"""
neopixels_C = Neopixel(num_leds=6, state_machine=2, pin=14, mode="RGB")
neopixels_D = Neopixel(num_leds=6, state_machine=3, pin=15, mode="RGB")
neopixels_E = Neopixel(num_leds=6, state_machine=4, pin=16, mode="RGB")
"""
neopixels_F = Neopixel(num_leds=5, state_machine=5, pin=17, mode="RGB")
"""
neopixels_G = Neopixel(num_leds=4, state_machine=6, pin=18, mode="RGB")
neopixels_H = Neopixel(num_leds=4, state_machine=7, pin=19, mode="RGB")
"""


p_OscA_waveform = Osc_shape(name="OscA_waveform", current_value=0, min_value=0, max_value=127, step=1, midi_cc=DCO_1_WAVE, midi_cc_button=MUTE_WFA, oled_screen=display_oled, neopixel_strip=neopixels_A, num_LED=0, i2C_midi=i2c_midi)
p_OscA_PWM = Osc_pwm(name="PWM_Osc_A", current_value=90, min_value=0, max_value=127, step=1, midi_cc=DCO1_PW, midi_cc_button=MUTE_PWA, oled_screen=display_oled, neopixel_strip=neopixels_A, num_LED=1, i2C_midi=i2c_midi)
p_OscA_Pitch = Osc_pitch(name="Pitch_Osc_A", current_value=90, min_value=0, max_value=127, step=1, midi_cc=DCO1_DETUNE, midi_cc_button=DCO1_PITCH_COARSE, oled_screen=display_oled, neopixel_strip=neopixels_A, num_LED=2, i2C_midi=i2c_midi)
p_OscA_level = level(name="Level_Osc_A", current_value=90, min_value=0, max_value=127, step=1, midi_cc=OSCA_LEVEL, midi_cc_button=MUTE_OSCA, oled_screen=display_oled, neopixel_strip=neopixels_A, num_LED=3, i2C_midi=i2c_midi)

#FIXME les midi CC

p_OscB_waveform = Osc_shape(name="OscB_waveform", current_value=0, min_value=0, max_value=127, step=1, midi_cc=DCO_1_WAVE, midi_cc_button=MUTE_WFA, oled_screen=display_oled, neopixel_strip=neopixels_B, num_LED=0, i2C_midi=i2c_midi)
p_OscB_PWM = Osc_pwm(name="PWM_Osc_B", current_value=90, min_value=0, max_value=127, step=1, midi_cc=DCO1_PW, midi_cc_button=MUTE_PWA, oled_screen=display_oled, neopixel_strip=neopixels_B, num_LED=1, i2C_midi=i2c_midi)
p_OscB_Pitch = Osc_pitch(name="Pitch_Osc_B", current_value=90, min_value=0, max_value=127, step=1, midi_cc=DCO1_DETUNE, midi_cc_button=DCO1_PITCH_COARSE, oled_screen=display_oled, neopixel_strip=neopixels_B, num_LED=2, i2C_midi=i2c_midi)
p_OscB_level = level(name="Level_Osc_B", current_value=90, min_value=0, max_value=127, step=1, midi_cc=OSCA_LEVEL, midi_cc_button=MUTE_OSCA, oled_screen=display_oled, neopixel_strip=neopixels_B, num_LED=3, i2C_midi=i2c_midi)
"""
p_VCF_A = ASDR(name="VCF Att", current_value=25, min_value=0, max_value=127, step=1, midi_cc=DCO_1_WAVE, midi_cc_button=MUTE_WFA, oled_screen=display_oled, neopixel_strip=neopixels_C, num_LED=0, i2C_midi=i2c_midi)
p_VCF_D = ASDR(name="VCF Dec", current_value=25, min_value=0, max_value=127, step=1, midi_cc=DCO_1_WAVE, midi_cc_button=MUTE_WFA, oled_screen=display_oled, neopixel_strip=neopixels_C, num_LED=1, i2C_midi=i2c_midi)
p_VCF_S = ASDR(name="VCF Sus", current_value=25, min_value=0, max_value=127, step=1, midi_cc=DCO_1_WAVE, midi_cc_button=MUTE_WFA, oled_screen=display_oled, neopixel_strip=neopixels_C, num_LED=2, i2C_midi=i2c_midi)
p_VCF_R = ASDR(name="VCF Rel", current_value=25, min_value=0, max_value=127, step=1, midi_cc=DCO_1_WAVE, midi_cc_button=MUTE_WFA, oled_screen=display_oled, neopixel_strip=neopixels_C, num_LED=3, i2C_midi=i2c_midi)
p_VCF_Amount = level(name="VCF Amount", current_value=25, min_value=0, max_value=127, step=1, midi_cc=DCO_1_WAVE, midi_cc_button=MUTE_WFA, oled_screen=display_oled, neopixel_strip=neopixels_C, num_LED=4, i2C_midi=i2c_midi)
p_VCF_FM = level(name="VCF FM", current_value=25, min_value=0, max_value=127, step=1, midi_cc=DCO_1_WAVE, midi_cc_button=MUTE_WFA, oled_screen=display_oled, neopixel_strip=neopixels_C, num_LED=5, i2C_midi=i2c_midi)

p_VCA_A = ASDR(name="VCA Att", current_value=25, min_value=0, max_value=127, step=1, midi_cc=DCO_1_WAVE, midi_cc_button=MUTE_WFA, oled_screen=display_oled, neopixel_strip=neopixels_C, num_LED=0, i2C_midi=i2c_midi)
p_VCA_D = ASDR(name="VCA Dec", current_value=25, min_value=0, max_value=127, step=1, midi_cc=DCO_1_WAVE, midi_cc_button=MUTE_WFA, oled_screen=display_oled, neopixel_strip=neopixels_C, num_LED=1, i2C_midi=i2c_midi)
p_VCA_S = ASDR(name="VCA Sus", current_value=25, min_value=0, max_value=127, step=1, midi_cc=DCO_1_WAVE, midi_cc_button=MUTE_WFA, oled_screen=display_oled, neopixel_strip=neopixels_C, num_LED=2, i2C_midi=i2c_midi)
p_VCA_R = ASDR(name="VCA Rel", current_value=25, min_value=0, max_value=127, step=1, midi_cc=DCO_1_WAVE, midi_cc_button=MUTE_WFA, oled_screen=display_oled, neopixel_strip=neopixels_C, num_LED=3, i2C_midi=i2c_midi)
p_VCA_Amount = level(name="VCA Amount", current_value=25, min_value=0, max_value=127, step=1, midi_cc=DCO_1_WAVE, midi_cc_button=MUTE_WFA, oled_screen=display_oled, neopixel_strip=neopixels_C, num_LED=4, i2C_midi=i2c_midi)
p_VCA_Master = level(name="Master", current_value=25, min_value=0, max_value=127, step=1, midi_cc=DCO_1_WAVE, midi_cc_button=MUTE_WFA, oled_screen=display_oled, neopixel_strip=neopixels_C, num_LED=5, i2C_midi=i2c_midi)

p_Env_A = ASDR(name="Env Att", current_value=25, min_value=0, max_value=127, step=1, midi_cc=DCO_1_WAVE, midi_cc_button=MUTE_WFA, oled_screen=display_oled, neopixel_strip=neopixels_C, num_LED=0, i2C_midi=i2c_midi)
p_Env_D = ASDR(name="Env Dec", current_value=25, min_value=0, max_value=127, step=1, midi_cc=DCO_1_WAVE, midi_cc_button=MUTE_WFA, oled_screen=display_oled, neopixel_strip=neopixels_C, num_LED=1, i2C_midi=i2c_midi)
p_Env_S = ASDR(name="Env Sus", current_value=25, min_value=0, max_value=127, step=1, midi_cc=DCO_1_WAVE, midi_cc_button=MUTE_WFA, oled_screen=display_oled, neopixel_strip=neopixels_C, num_LED=2, i2C_midi=i2c_midi)
p_Env_R = ASDR(name="Env Rel", current_value=25, min_value=0, max_value=127, step=1, midi_cc=DCO_1_WAVE, midi_cc_button=MUTE_WFA, oled_screen=display_oled, neopixel_strip=neopixels_C, num_LED=3, i2C_midi=i2c_midi)
p_Env_Amount = level(name="Env Amount", current_value=25, min_value=0, max_value=127, step=1, midi_cc=DCO_1_WAVE, midi_cc_button=MUTE_WFA, oled_screen=display_oled, neopixel_strip=neopixels_C, num_LED=4, i2C_midi=i2c_midi)
p_Env_Dest = ModDest(name="Env Dest", current_value=25, min_value=0, max_value=127, step=1, midi_cc=DCO_1_WAVE, midi_cc_button=MUTE_WFA, oled_screen=display_oled, neopixel_strip=neopixels_C, num_LED=5, i2C_midi=i2c_midi)
"""

p_LFO_shape = level(name="LFO Shape", current_value=25, min_value=0, max_value=127, step=1, midi_cc=DCO_1_WAVE, midi_cc_button=MUTE_WFA, oled_screen=display_oled, neopixel_strip=neopixels_F, num_LED=0, i2C_midi=i2c_midi)
p_LFO_rate = level(name="LFO rate", current_value=25, min_value=0, max_value=127, step=1, midi_cc=DCO_1_WAVE, midi_cc_button=MUTE_WFA, oled_screen=display_oled, neopixel_strip=neopixels_F, num_LED=1, i2C_midi=i2c_midi)
p_LFO_amount = level(name="LFO Amount", current_value=25, min_value=0, max_value=127, step=1, midi_cc=DCO_1_WAVE, midi_cc_button=MUTE_WFA, oled_screen=display_oled, neopixel_strip=neopixels_F, num_LED=2, i2C_midi=i2c_midi)
p_LFO_Dest_1 = level(name="LFO dest1", current_value=25, min_value=0, max_value=127, step=1, midi_cc=DCO_1_WAVE, midi_cc_button=MUTE_WFA, oled_screen=display_oled, neopixel_strip=neopixels_F, num_LED=3, i2C_midi=i2c_midi)
p_LFO_Dest_2 = level(name="LFO dest2", current_value=25, min_value=0, max_value=127, step=1, midi_cc=DCO_1_WAVE, midi_cc_button=MUTE_WFA, oled_screen=display_oled, neopixel_strip=neopixels_F, num_LED=4, i2C_midi=i2c_midi)

current_param = p_OscA_waveform

# Other initialization (ADCs, Second I2C, etc.) follows the same as the previous example.

# Main loop remains the same as before.

to_be_updated = []

def core_1():             #This function will be run in the second core
    global to_be_updated     #because it is started with the _thread.start command
    global display_oled
    global current_param
    print("start Core1")
    i = 0
    while True:
        
        while to_be_updated:
            current_param = to_be_updated.pop()
            current_param.update()
        
        i += 1
        if i > 5:
            i = 0            
            #display_oled.fill(0)
            #display_oled.text(current_param.name, 0, 0, 1)        
            #display_oled.text(str(current_param.value), 0, 32, 1)
            #display_oled.show()        
        sleep(0.05)



_thread.start_new_thread(core_1,()) #Starts the thread running in core 1

while True:
    mcpA.interrupt_captured
    mcpB.interrupt_captured
    mcpF.interrupt_captured
    print("loop")
    sleep(1)
