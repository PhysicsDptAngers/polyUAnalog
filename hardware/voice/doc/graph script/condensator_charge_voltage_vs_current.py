# -*- coding: utf-8 -*-
"""
Created on Thu May  2 13:58:46 2024

@author: utilisateur
"""

import numpy as np
import matplotlib.pyplot as plt

# Constants for constant voltage charging
E0 = 1.0  # Asymptotic voltage in volts
R = 1.0   # Resistance in ohms
C = 1.0   # Capacitance in farads
tau = R * C  # Time constant

# Time settings for constant voltage charging
t_max = 5 * tau  # Maximum time to 5 times the time constant
t_cv = np.linspace(0, t_max, 500)  # Time vector for constant voltage

# Voltage calculations for constant voltage charging
V_cv = E0 * (1 - np.exp(-t_cv / tau))

# Constants for constant current charging
R_cc = 1.0   # Resistance in ohms, for reference in calculating I0
C_cc = 1E-6  # Capacitance in farads
I0 = 0.5     # Current in amperes for constant current charging
V_target = 10.0  # Target voltage in volts

# Time settings for constant current charging
t_max_cc = V_target * C_cc / I0  # Time to reach target voltage in seconds
t_cc = np.linspace(0, t_max_cc, 500)  # Time vector for constant current

# Voltage calculations for constant current charging
V_cc = (I0 / C_cc) * t_cc  # Voltage as a function of time

# Plotting
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(12, 6))  # 1 row, 2 columns

# Plot for constant voltage charging
ax1.plot(t_cv / tau, V_cv, label='V(t) = $E_0(1 - e^{-t/RC})$', linestyle='-')
ax1.axhline(y=E0, color='r', linestyle='--', label='$E_0$ Asymptote')
ax1.axvline(x=1, color='b', linestyle='--', label='$t = RC$ (tau)')
ax1.set_title('Constant Voltage Charging')
ax1.set_xlabel('Time (fractions of $RC$)')
ax1.set_ylabel('Voltage (V)')
ax1.legend()
ax1.set_ylim([0, 1.1 * E0])
ax1.set_yticks([0, E0])
ax1.set_yticklabels(['0', 'E0'])
ax1.set_xticks(np.arange(0, 6, 1))

# Plot for constant current charging
ax2.plot(t_cc * 1E6, V_cc, label='V(t) = $(I_0/C) \cdot t$', linestyle='-')
ax2.text(t_max_cc * 1E6 / 4, V_target / 2, 'Slope = $I_0/C$', fontsize=12, verticalalignment='center')
ax2.axhline(y=V_target, color='r', linestyle='--', label='Target Voltage = $I_0 \cdot t / C$')
ax2.set_title('Constant Current Charging')
ax2.set_xlabel('Time (μs)')
ax2.set_ylabel('Voltage (V)')
ax2.legend()
ax2.set_ylim([0, 1.1 * V_target])
ax2.set_yticks([0, V_target])
ax2.set_yticklabels(['0', '$I_0 \cdot t_{max} / C$'])
ax2.set_xlim([0, t_max_cc * 1E6])
ax2.set_xticks(np.linspace(0, t_max_cc * 1E6, 5))

plt.tight_layout()
plt.savefig("condensator_charge_voltage_vs_current.png", dpi=300)
plt.show()
