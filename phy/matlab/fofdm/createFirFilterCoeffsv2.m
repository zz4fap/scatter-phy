clear all;close all;clc

% Set RNG state for repeatability
s = rng(211);

%% System Parameters
%
% Define system parameters for the example. These parameters can be
% modified to explore their impact on the system.

phy_bw_to_be_used = 5;

filterOrder = 64;

numFFT = [128 256 384 768 1536];            % Number of FFT points
deltaF = 15000;          % Subcarrier spacing
numRBs = [6 15 25 50 100];             % Number of resource blocks
rbSize = 12;             % Number of subcarriers per resource block
cpLen  = [9 18 27 32 64];             % Cyclic prefix length in samples

bitsPerSubCarrier = 6;   % 2: QPSK, 4: 16QAM, 6: 64QAM, 8: 256QAM

toneOffset = 1;          % Tone offset or excess bandwidth (in subcarriers)
L = filterOrder+1;                 % Filter length (=filterOrder+1), odd

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

numDataCarriers = numRBs(phy_bw_to_be_used)*rbSize;    % number of data subcarriers in subband
halfFilt = floor(L/2);
n = -halfFilt:halfFilt;

% Sinc function prototype filter
pb = sinc((numDataCarriers+2*toneOffset).*n./numFFT(phy_bw_to_be_used));

% Sinc truncation window
w = (0.5*(1+cos(2*pi.*n/(L-1)))).^1.2;

% Normalized lowpass filter coefficients
fnum = (pb.*w)/sum(pb.*w);

% Filter impulse response
if(0)
    h = fvtool(fnum, 'Analysis', 'impulse', 'NormalizedFrequency', 'off', 'Fs', numFFT*deltaF);
    h.CurrentAxes.XLabel.String = 'Time (\mus)';
    h.FigureToolbar = 'off';
end

% Use dsp filter objects for filtering
filtTx = dsp.FIRFilter('Structure', 'Direct form', 'Numerator', fnum);
filtRx = clone(filtTx); % Matched filter for the Rx

FirTxFilter = filtTx.Numerator;

FirRxFilter = filtRx.Numerator;

%error_tx_rx_filter = calculateError(FirTxFilter,FirRxFilter);

% QAM Symbol mapper.
qamMapper = comm.RectangularQAMModulator('ModulationOrder', 2^bitsPerSubCarrier, 'BitInput', true, 'NormalizationMethod', 'Average power');

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

% Generate data symbols
bitsIn = randi([0 1], bitsPerSubCarrier*numDataCarriers, 1);
symbolsIn = qamMapper(bitsIn);

% Pack data into an OFDM symbol 
offset = (numFFT(phy_bw_to_be_used)-numDataCarriers)/2; % for band center
symbolsInOFDM = [zeros(offset,1); symbolsIn; zeros(numFFT(phy_bw_to_be_used)-offset-numDataCarriers,1)];
ifftOut = ifft(ifftshift(symbolsInOFDM));

% Prepend cyclic prefix
txSigOFDM = [ifftOut(end-cpLen(phy_bw_to_be_used)+1:end); ifftOut];

% Zero-padding to flush tail.
%txSigOFDMPadded = [txSigOFDM; txSigOFDM; zeros(L-1,1)];
packet1 = [txSigOFDM];
packet2 = [txSigOFDM; zeros(L-1,1);];
txSigOFDMPadded = [packet1; packet2];

% Filter signal. Get the transmit signal
txSigFOFDM = filtTx(txSigOFDMPadded);

% Apply implemented filter.
txSigFOFDM2 = myFilterv2(FirTxFilter, txSigOFDMPadded);

error_filters = calculateError(txSigFOFDM,txSigFOFDM2);
if(error_filters > 1e-14)
   fprintf(1,'Average error too big!\n'); 
end

% Plot power spectral density (PSD)
signal2plot = txSigFOFDM2;
hFig = figure('Position', figposition([1.5*46 1.5*50 1.5*30 1.5*30]), 'MenuBar', 'none');
axis([-0.5 0.5 -230 10]);
hold on; 
grid on
xlabel('Normalized frequency');
ylabel('PSD (dBW/Hz)')
title(['F-OFDM, ' num2str(numRBs(phy_bw_to_be_used)) ' Resource blocks, ' num2str(rbSize) ' Subcarriers each, ', 'Excess bandwidth: ', num2str(toneOffset)])
[psd,f] = periodogram(signal2plot, rectwin(length(signal2plot)), numFFT(phy_bw_to_be_used)*2, 1, 'centered'); 
plot(f,10*log10(psd)); 

%% ------------------------------------------------------
% N = 1000;
% input_signal = randn(1,N);
% %input_signal(1) = 1;
% 
% fir1 = filter(FirTxFilter,1,[input_signal zeros(1,L-1)]);
% 
% fir2 = filter(FirTxFilter,1,fir1);
% 
% 
% a=1;

%% ------------------------------------------------------
if(1)
signalToPrint = FirTxFilter; %txSigFOFDM2;
is_complex = 0;
if(is_complex==1)
    for i=1:1:length(signalToPrint)
        if(i < length(signalToPrint))
            if(imag(signalToPrint(i)) < 0)
                fprintf(1,'%1.20e %1.20ei,\t',real(signalToPrint(i)),imag(signalToPrint(i)));
            else
                fprintf(1,'%1.20e +%1.20ei,\t',real(signalToPrint(i)),imag(signalToPrint(i)));
            end
        else
            if(imag(signalToPrint(i)) < 0)
                fprintf(1,'%1.20e %1.20ei\n',real(signalToPrint(i)),imag(signalToPrint(i)));
            else
               fprintf(1,'%1.20e +%1.20ei\n',real(signalToPrint(i)),imag(signalToPrint(i))); 
            end
        end
        if(mod(i,5)==0)
            fprintf(1,'\n');
        end
    end
else
    for i=1:1:length(signalToPrint)
        if(i < length(signalToPrint))
            fprintf(1,'%1.20e,\t',signalToPrint(i));
        else
            fprintf(1,'%1.20e\n',signalToPrint(i));
        end
        if(mod(i,5)==0)
            fprintf(1,'\n');
        end
    end
end
end
