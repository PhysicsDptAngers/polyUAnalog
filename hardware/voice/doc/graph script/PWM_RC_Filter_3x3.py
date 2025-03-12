# -*- coding: utf-8 -*-
"""
Created on Thu Apr  6 13:29:17 2023

@author: utilisateur
"""


import numpy as np
import matplotlib.pyplot as plt
from scipy.signal import lfilter

# Constants
fundamental_freq = 32768  # Frequency of the PWM signal (32.768 kHz)
duty_cycle = 0.25
num_harmonics = 10  # Number of harmonics to consider
sampling_rate = 2 * (2 * num_harmonics - 1) * fundamental_freq  # Nyquist rate for the highest harmonic
dt = 1 / sampling_rate

# RC low-pass filter parameters
R = 1000  # Resistance (ohms)
C = 1e-7  # Capacitance (farads)
tau = R * C  # Time constant
omegac = 1/tau
fc = omegac / 2*np.pi




fcs = [100, 1000, 10000]
taus = 1 / (2*np.pi*np.array(fcs))

# Time array
duration = 500 / fundamental_freq  # 300 periods of the fundamental frequency
t = np.arange(0, duration, 1 / sampling_rate)
oversample_factor = 100
dt_ovs = 1 / (sampling_rate*oversample_factor)
t_oversampled = np.arange(0, duration, 1 / (sampling_rate*oversample_factor))


nb_oversampled_point_per_period = int((1 / fundamental_freq ) * (sampling_rate*oversample_factor))
print("nb_oversampled_point_per_period", nb_oversampled_point_per_period)

def create_pwm_signal(freq, duty_cycle, time):
    period = 1 / freq
    return np.array([1 if (t % period) < (period * duty_cycle) else 0 for t in time])


# Row and column headers
row_headers = ['fc=' + str(fcs[0]) + " Hz", 'fc=' + str(fcs[1]) + " Hz", 'fc=' + str(fcs[2]) + " Hz"]
col_headers = ['Spectrum', 'Settling time', 'Residual Ripple']

fig, axes = plt.subplots(3, 3, figsize=(12, 12))


i = 0
for fc in fcs:

    
    tau = 1/(2*np.pi*fc)
    print(tau)

    idx_permanent = np.searchsorted(t_oversampled, 5*tau)


    idx_8_tau = np.searchsorted(t_oversampled, 8*tau)

    
    
    
    # Generate PWM signal
    pwm_signal = create_pwm_signal(fundamental_freq, duty_cycle, t)
    pwm_signal_oversampled = create_pwm_signal(fundamental_freq, duty_cycle, t_oversampled)
    
    # Apply RC low-pass filter
    
    alpha = dt / (tau + dt)

    filtered_signal = lfilter([alpha], [1, alpha - 1], pwm_signal)
    alpha_oversample = dt_ovs / (tau + dt_ovs)
    filtered_signal_oversampled = lfilter([alpha_oversample], [1, alpha_oversample - 1], pwm_signal_oversampled)
    
    # Calculate the spectra
    freqs = np.fft.fftfreq(len(t), 1 / sampling_rate)
    pwm_spectrum = np.fft.fft(pwm_signal)
    filtered_spectrum = np.fft.fft(filtered_signal)
    
    # Filter's frequency response
    filter_freqs = np.logspace(0, 5, 1000)
    filter_response = 1 / np.sqrt(1 + (2 * np.pi * filter_freqs * tau) ** 2)
    

     
    # Plot Frequency response
    #axes[i, 0].plot(freqs[:len(freqs)//2], 20*np.log10(np.abs(pwm_spectrum[:len(freqs)//2])), label='PWM Spectrum', alpha=0.5)
    #axes[i, 0].plot(freqs[:len(freqs)//2], 20*np.log10(np.abs(filtered_spectrum[:len(freqs)//2])), label='Filtered Spectrum', alpha=0.5)
    #axes[i, 0].set_ylim(0,100)
    max_spec = np.max(np.abs(pwm_spectrum[:len(freqs)//2]))
    print(max_spec)
    axes[i, 0].plot(freqs[:len(freqs)//2], np.abs(pwm_spectrum[:len(freqs)//2])/max_spec, label='PWM Spectrum', alpha=0.5)
    axes[i, 0].plot(freqs[:len(freqs)//2], np.abs(filtered_spectrum[:len(freqs)//2])/max_spec, label='Filtered Spectrum', alpha=0.5)
   
    # axes[i, 0].set_title('Frequency Spectrum')
    axes[i, 0].set_xlabel('Frequency (Hz)')
    axes[i, 0].set_ylabel('Magnitude (ua)')
    axes[i, 0].legend()
    axes[i, 0].legend(loc='lower left')    
    
    ax3 = axes[i, 0].twinx()
    ax3.semilogx(filter_freqs, 20 * np.log10(filter_response), 'r', label='Filter Response')
    ax3.set_xlim(10, sampling_rate/2)
    #ax3.plot(filter_freqs, 20 * np.log10(filter_response), 'r', label='Filter Response')
    ax3.set_ylabel('Filter Response (dB)')
    ax3.legend(loc='top right')    
    
    # Plot Ripple
    axes[i, 1].set_xlabel('Time (µs)')
    axes[i, 1].set_ylabel('Amplitude')
    axes[i, 1].plot(t_oversampled[0:idx_permanent]*1E6, pwm_signal_oversampled[0:idx_permanent], label='PWM Signal')
    axes[i, 1].plot(t_oversampled[0:idx_permanent]*1E6, filtered_signal_oversampled[0:idx_permanent], label='Filtered Signal')
    axes[i, 2].legend()
    
    # Plot Settle time
    axes[i, 2].set_xlabel('Time (µs)')
    axes[i, 2].set_ylabel('Amplitude')    
    t_settle = t_oversampled[idx_8_tau:idx_8_tau+nb_oversampled_point_per_period]*1E6
    filtered_signal_settle =  filtered_signal_oversampled[idx_8_tau:idx_8_tau+nb_oversampled_point_per_period]
    axes[i, 2].plot(t_settle, pwm_signal_oversampled[idx_8_tau:idx_8_tau+nb_oversampled_point_per_period], label='PWM Signal')
    axes[i, 2].plot(t_settle, filtered_signal_settle, label='Filtered Signal')    
    axes[i, 2].hlines(duty_cycle, t_settle[0],t_settle[-1], alpha=0.3)
    
    axes[i, 2].hlines(np.max(filtered_signal_settle), t_settle[0],t_settle[-1],  alpha=0.3)
    axes[i, 2].hlines(np.min(filtered_signal_settle), t_settle[0],t_settle[-1], alpha=0.3)
    axes[i, 2].legend()

    # ax1.set_title('Time Domain')
    axes[i, 2].set_xlabel('Time (s)')
    axes[i, 2].set_ylabel('Amplitude')
        
    i += 1


# Set row headers
for i, ax in enumerate(axes[:, 0]):
    ax.annotate(row_headers[i],
                xy=(0, 0.5),
                xytext=(-ax.yaxis.labelpad - 5, 0),
                textcoords='offset points',
                fontsize=16,
                ha='right',
                va='center',
                rotation=90,
                xycoords=ax.yaxis.label)

# Set column headers
for i, ax in enumerate(axes[0, :]):
    fig.text((i + 0.6) / 3, 0.95, col_headers[i],
             fontsize=16,
             ha='center',
             va='baseline')


# Adjust layout and display the plot
fig.tight_layout(rect=[0, 0, 1, 0.92])
plt.show()