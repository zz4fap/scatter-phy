clear all;close all;clc

filename = "/home/zz4fap/work/mcf_tdma/scatter/build/phy/srslte/examples/scatter_iq_dump_plus_sensing_node_id_15_20180417_135234_970961.dat";
fileID = fopen(filename);

MAX_SHORT_VALUE = 32767;

FFT_LENGTH = 2048;

%SUBFRAME_SIZE = 5760; % 5 MHZ
SUBFRAME_SIZE = 1920; % 1.4 MHZ

NumberOfSamplesToRead = 23.04e6/2;

N = SUBFRAME_SIZE;

index = 0; %1.5e6;
offset = 2*4*index; % 2 samples to form I+j*Q times 4 bytes per component, i.e., I or Q.

fseek(fileID, offset, 'bof');

A = fread(fileID, 2*NumberOfSamplesToRead, 'short');

fclose(fileID);

re = A(1:2:end);
im = A(2:2:end);

signal = complex(re,im)./MAX_SHORT_VALUE;

% Downconvert signal
t_idx = (0:1:length(signal)-1).';
sc_spacing = 15e3;
freq_offset_1dot4MHz_in_20MHz = 128*15000; % offset in Hz: 1.92 MHz
sc_idx_offset = (freq_offset_1dot4MHz_in_20MHz/sc_spacing);
nfft_20MHz = 1536;
nfft_1dot4MHz = 128;
nfft_20MHz_standard = 2048;
sampling_rate_20MHz_standard = 30.72e6;
sampling_rate_20MHz = sampling_rate_20MHz_standard*nfft_20MHz/nfft_20MHz_standard;

% Baseband FIR for 1.4MHz Rx,
fir_coef = fir1(512, 1.4e6/sampling_rate_20MHz).';
len_fir_half = (length(fir_coef)-1)/2;
%freqz(fir_coef,1,512)

complex_exponential = exp(+1i.*2.*pi.*(4*sc_idx_offset).*t_idx./nfft_20MHz);
bb_signal = signal.*complex_exponential;
bb_signal = conv(bb_signal, fir_coef);
bb_signal = bb_signal((len_fir_half+1):(end-len_fir_half));
bb_signal = bb_signal(1:(nfft_20MHz/nfft_1dot4MHz):end);

h1 = figure(1);
title('IQ samples')
plot(real(signal))
saveas(h1,'iq_samples.png')

h2 = figure(2);
title('Spectrogram')
fs = 23.04e6;
overlap = 512;
spectrogram(signal,kaiser(FFT_LENGTH,5), overlap ,FFT_LENGTH , fs ,'centered');
saveas(h2,'spectrogram.png')

h2 = figure(3);
title('Spectrogram - Down-converted signal (base band)')
fs = 23.04e6/12;
overlap = 512;
spectrogram(bb_signal, kaiser(FFT_LENGTH,5), overlap, FFT_LENGTH, fs, 'centered');
