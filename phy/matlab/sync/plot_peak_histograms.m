clear all;close all;clc

peak_values_noisy_only = load('/home/zz4fap/work/dcf_tdma_turbo_decoder_higher_mcs/scatter/build/peak_values_noise_only.dat');

peak_values_signal_present = load('/home/zz4fap/work/dcf_tdma_turbo_decoder_higher_mcs/scatter/build/peak_values_signal_present.dat');


h1 = histogram(peak_values_noisy_only);
hold on
h2 = histogram(peak_values_signal_present);

h1.Normalization = 'probability';
h1.BinWidth = 0.5;

h2.Normalization = 'probability';
h2.BinWidth = 0.5;

legend('Noise', 'Signal');
hold off





figure;
h3 = histogram(peak_values_signal_present);
h3.Normalization = 'probability';
h3.BinWidth = 0.5;


figure;
h4 = histogram(peak_values_noisy_only);
h4.Normalization = 'probability';
h4.BinWidth = 0.05;

max_noise_only = max(peak_values_noisy_only);

[a b] = find(peak_values_noisy_only >= 2.7);

above_threshold_ratio = length(a)/length(peak_values_noisy_only);