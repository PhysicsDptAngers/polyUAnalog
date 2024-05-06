import numpy as np
import matplotlib.pyplot as plt

# Time settings
pulse_period = 1e-3    # Period of each pulse in seconds
total_time = 2 * pulse_period  # Total time for two periods of the DCO
dt = 1e-6          # Time step in seconds (1 µs)
time = np.arange(0, total_time, dt)

# Capacitor voltage settings
Vmax = 5.0  # Maximum voltage of the capacitor
charge_rate = Vmax / pulse_period  # Voltage increase per period to reach 5V
discharge_rate = Vmax / (2e-5)  # Make discharge visible, slower discharge rate

# Control voltage pulse settings
pulse_duration = 5e-5  # Exaggerated pulse duration in seconds for visibility

# Generate control pulse signal
pulse = np.zeros_like(time)
for t in range(len(time)):
    if (time[t] % pulse_period) < pulse_duration:
        pulse[t] = 3.3

# Calculate capacitor voltage
cap_voltage = np.zeros_like(time)
for i in range(1, len(time)):
    if pulse[i-1] == 0:
        # Charging the capacitor
        cap_voltage[i] = cap_voltage[i-1] + charge_rate * dt
        cap_voltage[i] = min(cap_voltage[i], Vmax)
    else:
        # Discharging the capacitor
        cap_voltage[i] = cap_voltage[i-1] - discharge_rate * dt
        cap_voltage[i] = max(cap_voltage[i], 0)

# Create the figure and axis objects
fig, ax1 = plt.subplots()

# Plot the capacitor voltage
ax1.plot(time * 1e3, cap_voltage, 'b-', label='Capacitor Voltage', linewidth=3)
ax1.set_xlabel('Time (ms)', fontsize=18)
ax1.set_ylabel('Capacitor Voltage (V)', color='b', fontsize=18)
ax1.tick_params(axis='y', labelcolor='b', width=2, labelsize=18)  # Increase label size
ax1.spines['top'].set_linewidth(2)
ax1.spines['right'].set_linewidth(2)
ax1.spines['left'].set_linewidth(2)
ax1.spines['bottom'].set_linewidth(2)
ax1.xaxis.set_tick_params(width=2, labelsize=18)  # Increase x-axis label size

# Create a second y-axis for the control pulse
ax2 = ax1.twinx()
ax2.plot(time * 1e3, pulse, 'r-', label='Discharge Pulse', linewidth=3)
ax2.set_ylabel('Pulse Voltage (V)', color='r', fontsize=18)
ax2.tick_params(axis='y', labelcolor='r', width=2, labelsize=18)
ax2.spines['top'].set_linewidth(2)
ax2.spines['right'].set_linewidth(2)
ax2.spines['left'].set_linewidth(2)
ax2.spines['bottom'].set_linewidth(2)
ax2.set_ylim(-0.1, 3.5)  # Extend the y-axis to show the pulse clearly

# Combine legends from both axes
lines, labels = ax1.get_legend_handles_labels()
lines2, labels2 = ax2.get_legend_handles_labels()
ax2.legend(lines + lines2, labels + labels2, fontsize=12)

# Show the grid and adjust layout
ax1.grid(True)
fig.tight_layout()  # Adjust layout to make room for the second y-axis
plt.savefig("capacitor_discharge.png", dpi=300)
plt.show()
