# -*- coding: utf-8 -*-
"""
Created on Mon May  6 20:07:33 2024

@author: Matthieu-bureau
"""

import numpy as np
import matplotlib.pyplot as plt

# Define parameters
V_ramp_max = 5  # maximum voltage of the ramp
duty_cycle = 0.3  # 50% duty cycle
V_PW = V_ramp_max * duty_cycle  # threshold voltage
V_ON = 3.3  # Voltage when the square wave is ON
time_period = 1  # Period of one oscillation
total_time = 2 * time_period  # Two oscillations
time_steps = np.linspace(0, total_time, 1000)  # Time steps

# Generate the voltage ramp (sawtooth wave)
voltage_ramp = (time_steps % time_period) / time_period * V_ramp_max

# Generate the square wave based on the threshold
square_wave = np.where(voltage_ramp > V_PW, 0, V_ON)

# Create the figure and axis objects
fig, ax1 = plt.subplots()

# Plot the voltage ramp
ax1.plot(time_steps, voltage_ramp, label='Voltage Ramp', color='blue', lw=3)
ax1.set_xlabel('Time (ms)', fontsize=16)
ax1.set_ylabel('Voltage (V)', color='blue', fontsize=16)
ax1.tick_params(axis='y', colors='blue')
ax1.axhline(y=V_PW, color='green', linestyle='--', label='Threshold $(V_{PW})$')
ax1.set_ylim(0, V_ramp_max)

# Create a second y-axis for the square wave
ax2 = ax1.twinx()
ax2.step(time_steps, square_wave, where='post', label='Square Wave', color='red', lw=3)
ax2.set_ylabel('Square Wave Voltage (V)', color='red', fontsize=16)
ax2.tick_params(axis='y', colors='red')
ax2.set_ylim(-0.1, 3.5)  # Extend to show the wave clearly

# Adding legend and grid
ax1.legend(loc='center right')
ax2.legend(loc=(0.7, 0.33))
ax1.grid(True)

# Show the plot
plt.savefig("threshold_square_wave.png", dpi=300)
plt.show()
