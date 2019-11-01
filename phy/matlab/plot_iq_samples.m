function [] = plot_iq_samples(filename)

%Usage: plot_iq_samples('/home/zz4fap/backup/master/scatter/build/phy/srslte/examples/scatter_iq_dump_node_id_4660_20170717_151215_two_columns_format.dat')

columns = load(filename);

complex_samples = complex(columns(:,1),columns(:,2));

FFT_LENGTH = 2048;

offset = 0;

Fs = 23.04e6;            % Sampling frequency                    
T = 1/Fs;             % Sampling period       
L = FFT_LENGTH;             % Length of signal
t = (0:L-1)*T;        % Time vector

Y = fft(complex_samples(1+offset*FFT_LENGTH:L+offset*FFT_LENGTH));

P2 = abs(Y/L);
P1 = P2(1:L/2+1);
P1(2:end-1) = 2*P1(2:end-1);

f = Fs*(0:(L/2))/L;
plot(f,P1) 
title('Single-Sided Amplitude Spectrum of X(t)')
xlabel('f (Hz)')
ylabel('|P1(f)|')
