clear all;close all;clc

%% System Parameters
%
% Define system parameters for the example. These parameters can be
% modified to explore their impact on the system.

numFFT = 384;            % Number of FFT points
deltaF = 15000;          % Subcarrier spacing
numRBs = 25;             % Number of resource blocks
rbSize = 12;             % Number of subcarriers per resource block
cpLen = 27;              % Cyclic prefix length in samples

bitsPerSubCarrier = 6;   % 2: QPSK, 4: 16QAM, 6: 64QAM, 8: 256QAM

toneOffset = 1; %2.5;     % Tone offset or excess bandwidth (in subcarriers)
L = 513;            % Filter length (=filterOrder+1), odd

%% Filtered-OFDM Filter Design
%
% Appropriate filtering for F-OFDM satisfies the following criteria:
%
% * Should have a flat passband over the subcarriers in the subband
% * Should have a sharp transition band to minimize guard-bands
% * Should have sufficient stop-band attenuation
%
% A filter with a rectangular frequency response, i.e. a sinc impulse
% response, meets these criteria. To make this causal, the low-pass filter
% is realized using a window, which, effectively truncates the impulse
% response and offers smooth transitions to zero on both ends [ <#11 3> ].

numDataCarriers = numRBs*rbSize;    % number of data subcarriers in subband
halfFilt = floor(L/2);
n = -halfFilt:halfFilt;

% Sinc function prototype filter
pb = sinc((numDataCarriers+2*toneOffset).*n./numFFT);

% Sinc truncation window
w = (0.5*(1+cos(2*pi.*n/(L-1)))).^1.2; %0.6

% Normalized lowpass filter coefficients
fnum = (pb.*w)/sum(pb.*w);

% Filter impulse response
if(0)
    h = fvtool(fnum, 'Analysis', 'impulse', ...
        'NormalizedFrequency', 'off', 'Fs', numFFT*deltaF);
    h.CurrentAxes.XLabel.String = 'Time (\mus)';
    h.FigureToolbar = 'off';
end

% Use dsp filter objects for filtering
filtTx = dsp.FIRFilter('Structure', 'Direct form', 'Numerator', fnum);
filtRx = clone(filtTx); % Matched filter for the Rx

FirTxFilter = filtTx.Numerator;

FirRxFilter = filtRx.Numerator;


% QAM Symbol mapper
qamMapper = comm.RectangularQAMModulator( ...
    'ModulationOrder', 2^bitsPerSubCarrier, 'BitInput', true, ...
    'NormalizationMethod', 'Average power');

%% F-OFDM Transmit Processing
%
% In F-OFDM, the subband CP-OFDM signal is passed through the designed
% filter. As the filter's passband corresponds to the signal's bandwidth,
% only the few subcarriers close to the edge are affected. A key
% consideration is that the filter length can be allowed to exceed the
% cyclic prefix length for F-OFDM [ <#11 1> ]. The inter-symbol
% interference incurred is minimized due to the filter design using
% windowing (with soft truncation).
%
% Transmit-end processing operations are shown in the following F-OFDM
% transmitter diagram.
%
% <<FOFDMTransmitDiagram.png>>

% Set up a figure for spectrum plot
hFig = figure('Position', figposition([46 50 30 30]), 'MenuBar', 'none');
axis([-0.5 0.5 -230 -20]);
hold on; 
grid on
xlabel('Normalized frequency');
ylabel('PSD (dBW/Hz)')
title(['F-OFDM, ' num2str(numRBs) ' Resource blocks, '  ...
    num2str(rbSize) ' Subcarriers each'])

% Generate data symbols
bitsIn = randi([0 1], bitsPerSubCarrier*numDataCarriers, 1);
symbolsIn = qamMapper(bitsIn);

% Pack data into an OFDM symbol 
offset = (numFFT-numDataCarriers)/2; % for band center
symbolsInOFDM = [zeros(offset,1); symbolsIn; ...
                 zeros(numFFT-offset-numDataCarriers,1)];
ifftOut = ifft(ifftshift(symbolsInOFDM));

% Prepend cyclic prefix
txSigOFDM = [ifftOut(end-cpLen+1:end); ifftOut]; 

% Filter, with zero-padding to flush tail. Get the transmit signal
txSigFOFDM = filtTx([txSigOFDM; zeros(L-1,1)]);

% Plot power spectral density (PSD) 
[psd,f] = periodogram(txSigFOFDM, rectwin(length(txSigFOFDM)), ...
                      numFFT*2, 1, 'centered'); 
plot(f,10*log10(psd)); 

% Compute peak-to-average-power ratio (PAPR)
PAPR = comm.CCDF('PAPROutputPort', true, 'PowerUnits', 'dBW');
[~,~,paprFOFDM] = PAPR(txSigFOFDM);
disp(['Peak-to-Average-Power-Ratio for F-OFDM = ' num2str(paprFOFDM) ' dB']);









%% ------------------------------------------------------
error = 0;
for i=1:1:length(FirRxFilter)
    error = error + (FirTxFilter(i) - FirRxFilter(i));
end
error = error/length(FirRxFilter);
if(error > 0)
   fprintf(1,'Something is wrong, error > 0: %f\n',error); 
end

for i=1:1:length(FirRxFilter)
    if(i < length(FirRxFilter))
        fprintf(1,'%e,\t',FirTxFilter(i));
    else
        fprintf(1,'%e\n',FirTxFilter(i));
    end
    if(mod(i,5)==0)
        fprintf(1,'\n');
    end
end
