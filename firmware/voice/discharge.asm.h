;
; Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
;
; SPDX-License-Identifier: BSD-3-Clause
;

; SET pin 0 should be mapped to your DCO GPIO


.program discharge
    pull block
    out y, 32
.wrap_target
    set pins, 1 [31]  ; Discharge on
    set pins, 0       ; Discharge off
    mov x, y
lp2:
    jmp x-- lp2   ; Delay for the same number of cycles again
.wrap             ; Loop forever!


% c-sdk {
// this is a raw helper function for use by the user which sets up the GPIO output, and configures the SM to output on a particular pin

void discharge_program_init(PIO pio, uint sm, uint offset, uint pin) {
   pio_gpio_init(pio, pin);
   pio_sm_set_consecutive_pindirs(pio, sm, pin, 1, true);
   pio_sm_config c = discharge_program_get_default_config(offset);
   sm_config_set_set_pins(&c, pin, 1);
   pio_sm_init(pio, sm, offset, &c);
}

%}



