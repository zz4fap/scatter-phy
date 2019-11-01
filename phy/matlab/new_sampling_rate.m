clear all;close all;clc

delta_f = 13000; % 15 KHz.
NFFT = 2048; % for 20 MHz BW.

% Standard Normal CP

subframe_duration = 1e-3; % 1 milisecond.

% Sampling Frequency.
Ts = 1/(delta_f*NFFT);

total_cp_duration = (1*160*Ts) + (6*144*Ts); % 2 slots with 6 CPs.
total_symbol_duration = 7*2048*Ts;

2*total_symbol_duration + 2*total_cp_duration