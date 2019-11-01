clear all;close all;clc

filename = "/home/zz4fap/backup/rx_thread/scatter/build/phy/srslte/examples/scatter_iq_dump_node_id_4660_20170802_184636.dat";
fileID = fopen(filename);

FFT_LENGTH = 2048;

offset = 0;

Fs = 23.04e6;            % Sampling frequency
T = 1/Fs;             % Sampling period
L = FFT_LENGTH;             % Length of signal
t = (0:L-1)*T;        % Time vector
f = Fs*(0:(L/2))/L;

for i=1:1:20
    
    A = fread(fileID, 2*FFT_LENGTH, 'short');
    
    re = A(1:2:end);
    im = A(2:2:end);
    
    signal = complex(re,im);
    
    Y = fft(signal(1+offset*FFT_LENGTH:L+offset*FFT_LENGTH));
    
    P2 = abs(Y/L);
    P1 = P2(1:L/2+1);
    P1(2:end-1) = 2*P1(2:end-1);
    
    figure(i)
    plot(f,P1)
    title('Single-Sided Amplitude Spectrum of X(t)')
    xlabel('f (Hz)')
    ylabel('|P1(f)|')
    
end

fclose(fileID);