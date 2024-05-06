import numpy as np
import matplotlib.pyplot as plt

# Frequencies
f0 = 440  # Low frequency
f1 = 880  # High frequency, twice f0

# Periods for each frequency
T0 = 1 / f0
T1 = 1 / f1

# Time array for plotting one period of f0 and two periods of f1
t = np.linspace(0, T0, 1000)

# Voltage ramps: using the same slope for both
slope = 0.5  # Arbitrary slope
voltage_ramp0 = slope * (t % T0)
voltage_ramp1 = slope * (t % T1)

# Normalizing the voltage to the maximum of the lower frequency ramp
max_voltage = max(voltage_ramp0)
voltage_ramp0 /= max_voltage
voltage_ramp1 /= max_voltage

# Convert time from seconds to microseconds for the x-axis
t_ms = t * 1e3

# Create the plot
plt.figure(figsize=(10, 5))
plt.plot(t_ms, voltage_ramp0, label=f'Ramp at {f0} Hz ', linewidth=3)
plt.plot(t_ms, voltage_ramp1, label=f'Ramp at {f1} Hz ', linewidth=3)
plt.title('Normalized Voltage Ramp Comparison for Different Frequencies')
plt.xlabel('Time (microseconds)', fontsize=14)
plt.ylabel('Normalized Voltage', fontsize=14)

# Setting thicker axes and tick marks
ax = plt.gca()  # Get current axes
ax.spines['top'].set_linewidth(1.5)
ax.spines['right'].set_linewidth(1.5)
ax.spines['left'].set_linewidth(1.5)
ax.spines['bottom'].set_linewidth(1.5)
ax.xaxis.set_tick_params(width=1.5)
ax.yaxis.set_tick_params(width=1.5)

plt.legend()
plt.grid(True)
plt.savefig("dco_voltage_ramp_wo_control.png", dpi=300)
plt.show()
