import numpy as np
import matplotlib.pyplot as plt

# Define the value of Vcc
Vcc = 12

# Define time array
t = np.linspace(0, 1, 500)

# Define ramp function
ramp = Vcc * t

# Define second curve function
curve2 = np.piecewise(t, [t < 5/24, (t >= 5/24) & (t < 5/12), t >= 5/12],
                       [lambda t: Vcc * t, lambda t: Vcc * (5/12 - t), 0])

# Create the plot
fig, ax = plt.subplots()

# Plot the ramp
ax.plot(t, ramp, label='IN : Ramp', lw=3)

# Plot the second curve
ax.plot(t, curve2, label='OUT : Waveshapper', lw=3)

# Add horizontal lines
ax.axhline(y=5/24*Vcc, xmin=0, xmax=5/24.0*1.08, color='r', linestyle='--', alpha=0.3)
ax.axhline(y=5/12*Vcc, xmin=0, xmax=5/12.0*1.02, color='r', linestyle='--', alpha=0.3)
ax.axhline(y=Vcc, color='r', linestyle='--', alpha=0.3)
ax.xaxis.set_tick_params(width=2, labelsize=18)  # Increase x-axis label size
ax.spines['top'].set_linewidth(2)
ax.spines['right'].set_linewidth(2)
ax.spines['left'].set_linewidth(2)
ax.spines['bottom'].set_linewidth(2)






# Add vertical lines at the point where the second curve intersects the horizontal lines
ax.axvline(x=5/24, ymin=0.05, ymax=5/24.0, color='g', linestyle='--', alpha=0.3)
ax.axvline(x=5/12, ymin=0.05, ymax=5/12.0, color='g', linestyle='--', alpha=0.3)

ax.annotate('', xy=(1, Vcc), xytext=(0.8, 0.8*Vcc), arrowprops=dict(facecolor='black', shrink=0.5), alpha=0.5)


# Set labels
ax.set_xlabel('Voltage OUT (V)', fontsize=16)
ax.set_ylabel('Voltage IN (V)', fontsize=16)

# Remove ticks
ax.xaxis.set_ticks([])
ax.yaxis.set_ticks([0, 5/24*Vcc, 5/12*Vcc, Vcc])  # set y-ticks locations

# Add labels to y-ticks
ax.set_yticklabels(['0', '5/24 Vcc', '5/12 Vcc', 'Vcc'], rotation=90)  # set labels

ax.text(0.05, 2.7, "saw")
ax.text(0.14, 5.2, "triangle")
ax.text(0.45, 0.2, "clipped triangle")


# Add a legend
ax.legend(fontsize=12)

# Show the plot
plt.savefig("waveshapper_tranfert_function.png", dpi=300)
plt.tight_layout()
plt.show()
