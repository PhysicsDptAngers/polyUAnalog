import time

# Maximum size for parameter name
MAX_NAME_SIZE = 16
STRIP_PIXEL_BRIGHTNESS = 6



_DIR_CW = const(0x10)  # Clockwise step
_DIR_CCW = const(0x20)  # Counter-clockwise step

# Rotary Encoder States
_R_START = const(0x0)
_R_CW_1 = const(0x1)
_R_CW_2 = const(0x2)
_R_CW_3 = const(0x3)
_R_CCW_1 = const(0x4)
_R_CCW_2 = const(0x5)
_R_CCW_3 = const(0x6)
_R_ILLEGAL = const(0x7)

_transition_table = [

    # |------------- NEXT STATE -------------|            |CURRENT STATE|
    # CLK/DT    CLK/DT     CLK/DT    CLK/DT
    #   00        01         10        11
    [_R_START, _R_CCW_1, _R_CW_1,  _R_START],             # _R_START
    [_R_CW_2,  _R_START, _R_CW_1,  _R_START],             # _R_CW_1
    [_R_CW_2,  _R_CW_3,  _R_CW_1,  _R_START],             # _R_CW_2
    [_R_CW_2,  _R_CW_3,  _R_START, _R_START | _DIR_CW],   # _R_CW_3
    [_R_CCW_2, _R_CCW_1, _R_START, _R_START],             # _R_CCW_1
    [_R_CCW_2, _R_CCW_1, _R_CCW_3, _R_START],             # _R_CCW_2
    [_R_CCW_2, _R_START, _R_CCW_3, _R_START | _DIR_CCW],  # _R_CCW_3
    [_R_START, _R_START, _R_START, _R_START]]             # _R_ILLEGAL

_transition_table_half_step = [
    [_R_CW_3,            _R_CW_2,  _R_CW_1,  _R_START],
    [_R_CW_3 | _DIR_CCW, _R_START, _R_CW_1,  _R_START],
    [_R_CW_3 | _DIR_CW,  _R_CW_2,  _R_START, _R_START],
    [_R_CW_3,            _R_CCW_2, _R_CCW_1, _R_START],
    [_R_CW_3,            _R_CW_2,  _R_CCW_1, _R_START | _DIR_CW],
    [_R_CW_3,            _R_CCW_2, _R_CW_3,  _R_START | _DIR_CCW],
    [_R_START,           _R_START, _R_START, _R_START],
    [_R_START,           _R_START, _R_START, _R_START]]

_STATE_MASK = const(0x07)
_DIR_MASK = const(0x30)



red = (255, 0, 0)
orange = (255, 165, 0)
yellow = (255, 150, 0)
green = (0, 255, 0)
blue = (0, 0, 255)
indigo = (75, 0, 130)
violet = (138, 43, 226)

def _wrap(value, incr, lower_bound, upper_bound):
    range = upper_bound - lower_bound + 1
    value = value + incr

    if value < lower_bound:
        value += range * ((lower_bound - value) // range + 1)

    return lower_bound + (value - lower_bound) % range


def _bound(value, incr, lower_bound, upper_bound):
    return min(upper_bound, max(lower_bound, value + incr))


ADDR_SYNTH = 0x10

class Parameter:
    def __init__(self, name, current_value, min_value, max_value, step, midi_cc, midi_cc_button, oled_screen, neopixel_strip, num_LED, i2C_midi, continuous=True, choices=None, reverse=False, invert=False):
        self.name = name
        self.value = current_value
        self.min_value = min_value
        self.max_value = max_value
        self.step = step
        self.midi_cc = midi_cc
        self.midi_cc_button = midi_cc_button
        self.oled_screen = oled_screen
        self.switch_state = False
        self.continuous = continuous
        self.choices = choices if choices is not None else []
        self.neopixel_strip = neopixel_strip
        self.num_LED = num_LED
        self._state = _R_START
        self._reverse = -1 if reverse else 1
        self._invert = invert
        self._half_step = False
        self.i2C_midi = i2C_midi
        self._incr = 1
        
        self.neopixel_strip.brightness(STRIP_PIXEL_BRIGHTNESS)
        
    def _process_rotary_pins(self, clk, dt):
        #print(self.name)
        #print("process_rotary")
        #print("clk : ", clk)
        #print("dt : ", dt)
        old_value = self.value
        clk_dt_pins = (clk << 1) | dt
                       
        if self._invert:
            clk_dt_pins = ~clk_dt_pins & 0x03
            
        # Determine next state
        if self._half_step:
            self._state = _transition_table_half_step[self._state &
                                                      _STATE_MASK][clk_dt_pins]
        else:
            self._state = _transition_table[self._state &
                                            _STATE_MASK][clk_dt_pins]
        direction = self._state & _DIR_MASK

        incr = 0
        if direction == _DIR_CW:
            incr = self._incr
            #print("ClockWise")
        elif direction == _DIR_CCW:
            incr = -self._incr
            #print("COUNTER ClockWise")

        incr *= self._reverse
        """
        if self._range_mode == self.RANGE_WRAP:
            self._value = _wrap(
                self._value,
                incr,
                self._min_val,
                self._max_val)
            
        elif self._range_mode == self.RANGE_BOUNDED:
            self._value = _bound(
                self._value,
                incr,
                self._min_val,
                self._max_val)
        else:
        
            self._value = self._value + incr
        """
        self.value = self.value + incr
        #TODO scale value to MIDI 0 - 127
        #print("value : ", self.value)
        #print(self.value)
        #self.display()
        #self.set_color()
            
    def value(self):
        return self.value
            
    def button_pressed(self, val):
        print("Button pressed")
        self.switch_state = val
        
        
    def update(self):
        #print("update")
        print(self.name)
        print(self.value)
        self.set_color()
        #self.display()
        #self.i2c_send_data()
                
    def display(self):
        self.oled_screen.fill(0)
        self.oled_screen.text(self.name, 0, 0, 1)        
        self.oled_screen.text(str(self.value), 0, 32, 1)
        self.oled_screen.show()

    def i2c_send_data(self):
        # Virtual method to send data via I2C. Placeholder implementation.
        #self.i2C_buff[1] = bytes(self.value)
        try:
            # Essayer d'envoyer les données via I2C
            self.i2C_midi.writeto(ADDR_SYNTH, bytes([self.midi_cc, self.value]))
        except OSError as e:
            # Si une exception OSError est levée, afficher un message d'erreur
            print("Erreur lors de l'envoi des données via I2C :", e)
        except ValueError as e:
            # Si une exception ValueError est levée (bytes value out of range), afficher un message d'erreur
            print("Erreur de valeur lors de la conversion en bytes :", e)            

    def set_color(self):
        # Virtual method to set color. Placeholder implementation.
        pass

class level(Parameter):
    def __init__(self, name, current_value, min_value, max_value, step, midi_cc, midi_cc_button, oled_screen, neopixel_strip, num_LED, i2C_midi, continuous=True, choices=None, reverse=None, invert=False):
        super().__init__(name, current_value, min_value, max_value, step, midi_cc, midi_cc_button, oled_screen, neopixel_strip, num_LED, i2C_midi, continuous, choices, reverse, invert)  
        self.is_muted = False
    
    def button_pressed(self, val):
        #print("button_pressed", val)
        # There is a pull-up so val ==  1 mean pressed
        if val == 0:
            self.is_muted = not self.is_muted
        if self.is_muted:
            print("Muted !")
        else:
            print("unmute")
        #TODO send corresponding midi signal
        self.set_color()
        #TODO screen display
        #i2C_midi.writeto("add", midi_cc_button) + La valeur !
        
    def set_color(self):
        #FIXME gerer les couleurs en fonction de la valeur du paramètre et aussi gerer le mute
        if self.is_muted :
            # red
            color = (self.value*2, 0, 0)
        else:
            # green
            color = (0, self.value*2, 0)
        self.neopixel_strip.set_pixel(self.num_LED, color)
        time.sleep(0.01)
        self.neopixel_strip.show()
        
class Osc_shape(level):
    def __init__(self, name, current_value, min_value, max_value, step, midi_cc, midi_cc_button, oled_screen, neopixel_strip, num_LED, i2C_midi, continuous=True, choices=None, reverse=None, invert=False):
        super().__init__(name, current_value, min_value, max_value, step, midi_cc, midi_cc_button, oled_screen, neopixel_strip, num_LED, i2C_midi, continuous, choices, reverse, invert)  

    
    def set_color(self):
        #FIXME gerer les couleurs en fonction de la valeur du paramètre et aussi gerer le mute
        if self.is_muted :
            # red
            color = (self.value*2, 0, 0)
        else:
            # from blue to green
            color = (self.value*2, 0, 255-self.value*2)
        self.neopixel_strip.set_pixel(self.num_LED, color)
        time.sleep(0.01)
        self.neopixel_strip.show()       
        
class Osc_pwm(level):
    def __init__(self, name, current_value, min_value, max_value, step, midi_cc, midi_cc_button, oled_screen, neopixel_strip, num_LED, i2C_midi, continuous=True, choices=None, reverse=None, invert=False):
        super().__init__(name, current_value, min_value, max_value, step, midi_cc, midi_cc_button, oled_screen, neopixel_strip, num_LED, i2C_midi, continuous, choices, reverse, invert)  

    def set_color(self):
        #FIXME gerer les couleurs en fonction de la valeur du paramètre et aussi gerer le mute
        if self.is_muted :
            # red
            color = (self.value*2, 0, 0)
        else:
            # yellow
            color = (self.value, self.value, 0)
        self.neopixel_strip.set_pixel(self.num_LED, color)
        time.sleep(0.01)
        self.neopixel_strip.show()
        
class Osc_pitch(Parameter):
    def __init__(self, name, current_value, min_value, max_value, step, midi_cc, midi_cc_button, oled_screen, neopixel_strip, num_LED, i2C_midi, continuous=True, choices=None, reverse=None, invert=False):
        super().__init__(name, current_value, min_value, max_value, step, midi_cc, midi_cc_button, oled_screen, neopixel_strip, num_LED, i2C_midi, continuous, choices, reverse, invert)  
        self.is_coarse = True
    
    def button_pressed(self, val):
        print("button")
        self.is_coarse = val
        #TODO send corresponding midi signal    
        
    def set_color(self):
        #FIXME gerer les couleurs en fonction de la valeur du paramètre et aussi gerer le mute
        color = (255, 0, 0)
        self.neopixel_strip.set_pixel(self.num_LED, color)
        time.sleep(0.01)
        self.neopixel_strip.show()            
 
 
class ASDR(Parameter):
    def __init__(self, name, current_value, min_value, max_value, step, midi_cc, midi_cc_button, oled_screen, neopixel_strip, num_LED, i2C_midi, continuous=True, choices=None, reverse=None, invert=False):
        super().__init__(name, current_value, min_value, max_value, step, midi_cc, midi_cc_button, oled_screen, neopixel_strip, num_LED, i2C_midi, continuous, choices, reverse, invert)  
        self.is_coarse = True
    
    def button_pressed(self, val):
        print("button")
        self.is_coarse = val
        #TODO send corresponding midi signal
        
    def set_color(self):
        #FIXME gerer les couleurs en fonction de la valeur du paramètre et aussi gerer le mute
        color = (255, 0, 0)
        self.neopixel_strip.set_pixel(self.num_LED, color)
        time.sleep(0.01)
        self.neopixel_strip.show()            
 
 
"""
class DSO_mode(Parameter):
    def __init__(self, name, current_value, min_value, max_value, step, midi_cc, midi_cc_button, oled_screen, neopixel_strip, num_LED, i2C_midi, continuous=True, choices=None, reverse=None, invert=False):
        super().__init__(name, current_value, min_value, max_value, step, midi_cc, midi_cc_button, oled_screen, neopixel_strip, num_LED, i2C_midi, continuous, choices, reverse, invert)  
        #FIXME voir ce qui est dispo
        
        
    def encoder_turned(self, is_clockwise):
        if(is_clockwise):
            self.current_value += self.step
        else:
            self.current_value -= self.step
        self.current_value = max(self.min_value, min(self.current_value, self.max_value))        
        
    def set_color(self):
        #FIXME gerer les couleurs en fonction de la valeur du paramètre et aussi gerer le mute
        if self.value == 0:
            color = (128, 128, 128)
        else if self.value == 1:
            color = red
        else if self.value == 2:
            color = green
        else if self.value == 3:
            color = blue
            
        self.neopixel_strip.set_pixel(self.num_LED, color)
        time.sleep(0.01)
        self.neopixel_strip.show()
        
    def display(self):
        self.oled_screen.fill(0)
        self.oled_screen.text(self.name, 0, 0, 1)
        if self.value == 0:
            text = "noise"
        else if self.value == 1:
            text = "sine"
        else if self.value == 2:
            text = "wavetable"
        else if self.value == 3:
            text = "fm"
        self.oled_screen.text(self.value, 12, 0, 1)
        self.oled_screen.show()

"""    
        
