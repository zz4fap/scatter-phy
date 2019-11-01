clear all;close all;clc

NFFT = 2048; % for 20 MHz BW.
num_of_ofdm_symbols = 7;

slot_duration = 0.5e-3;
subframe_duration = 1e-3; % 1 milisecond.

Ncp = 160;
delta_f = (num_of_ofdm_symbols*(Ncp+NFFT)) / (slot_duration*NFFT);
fprintf(1,'delta_f %f\n',delta_f);