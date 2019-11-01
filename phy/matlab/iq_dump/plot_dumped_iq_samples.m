clear all;close all;clc

filename = "/home/zz4fap/backup/rx_thread/scatter/build/phy/srslte/examples/scatter_iq_dump_node_id_4660_20170802_184636.dat";
fileID = fopen(filename);

BUFFER_LENGTH = 23040;

offset = 0;

nfft = 512;

Fs = 23.04e6;            % Sampling frequency
T = 1/Fs;             % Sampling period
L = BUFFER_LENGTH;             % Length of signal
t = (0:L-1)*T;        % Time vector
f = Fs*(0:(L/2))/L;

fseek(fileID, offset, 'bof');

for i=1:1:5
    
    A = fread(fileID, 2*BUFFER_LENGTH, 'short');
    
    re = A(1:2:end);
    im = A(2:2:end);
    
    signal = complex(re,im);
    
    figure(i);
    spectrogram(signal, kaiser(nfft,0.5), nfft/4, nfft, Fs ,'centered');
    title('waterfall');
    
end
fclose(fileID);



