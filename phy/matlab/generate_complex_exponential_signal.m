clear all;close all;clc

NFFT= 1024;

fs = 23040000;

fo = 500*(fs/NFFT);

n = 1:1:NFFT;

arg = 2*pi*fo*n*(1/fs);
signal = cos(arg) + 1i*sin(arg);

freq_signal = fft(signal,NFFT);

plot(0:1:NFFT-1,abs(freq_signal))

fprintf(1,'fs: %1.3f [MHz] - fo: %1.3f [MHz]\n',fs/1000000,fo/1000000);