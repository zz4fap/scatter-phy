clear all;close all;clc

%% F-OFDM vs. OFDM Modulation
%
% Here we compare Orthogonal Frequency Division Multiplexing (OFDM)
% with Filtered-OFDM (F-OFDM) and highlights the merits of the new
% candidate modulation scheme for emergent Fifth Generation (5G)
% communication systems.

%% Introduction
%
% This example compares Filtered-OFDM modulation with generic Cyclic Prefix
% OFDM (CP-OFDM) modulation. For F-OFDM, a well-designed filter is applied
% to the time domain OFDM symbol to improve the out-of-band radiation of
% the subband signal, while maintaining the complex-domain orthogonality of
% OFDM symbols.
%
% This example models Filtered-OFDM modulation with configurable
% parameters. It highlights the filter design technique and the basic
% transmit/receive processing.

s = rng(211);       % Set RNG state for repeatability

%% System Parameters
%
% Define system parameters for the example. These parameters can be
% modified to explore their impact on the system.

numFFT = 384;            % Number of FFT points
deltaF = 15000;          % Subcarrier spacing
numRBs = 25;             % Number of resource blocks
rbSize = 12;             % Number of subcarriers per resource block
cpLen  = 27;              % Cyclic prefix length in samples

numberOfOFDMSymbols = 1;

enableRXMatchedFilter = 1; % Enable or disable channel estimation and equalization

bitsPerSubCarrier = 6;   % 2: QPSK, 4: 16QAM, 6: 64QAM, 8: 256QAM
snrdB = 100;              % SNR in dB

toneOffset = 1; %2.5;        % Tone offset or excess bandwidth (in subcarriers)
L = 513;                 % Filter length (=filterOrder+1), odd

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
h = fvtool(fnum, 'Analysis', 'impulse', ...
    'NormalizedFrequency', 'off', 'Fs', numFFT*deltaF);
h.CurrentAxes.XLabel.String = 'Time (\mus)';
h.FigureToolbar = 'off';

% Use dsp filter objects for filtering
filtTx = dsp.FIRFilter('Structure', 'Direct form', ...
    'Numerator', fnum);
filtRx = clone(filtTx); % Matched filter for the Rx

FirTxFilter = filtTx.Numerator;

FirRxFilter = filtRx.Numerator;

error = 0;
for ii=1:1:length(FirTxFilter)
    local_error = (abs(FirTxFilter(ii)-FirRxFilter(ii)));
    if(local_error >= 1e-15)
        a = 1;
    end
    error = error + local_error;
end
error = error/length(FirTxFilter);

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
axis([-0.5 0.5 -220 -20]);
hold on;
grid on
xlabel('Normalized frequency');
ylabel('PSD (dBW/Hz)')
title(['F-OFDM, ' num2str(numRBs) ' Resource blocks, '  ...
    num2str(rbSize) ' Subcarriers each,' 'toneOffset: ' num2str(toneOffset)])

txSigFOFDM = complex(zeros(numberOfOFDMSymbols*(numFFT+cpLen),1),zeros(numberOfOFDMSymbols*(numFFT+cpLen),1));
for symb_cnt=1:1:numberOfOFDMSymbols
    
    % Generate data symbols
    bitsIn = randi([0 1], bitsPerSubCarrier*numDataCarriers, 1);
    symbolsIn = qamMapper(bitsIn);
    
    % Pack data into an OFDM symbol
    offset = (numFFT-numDataCarriers)/2; % for band center
    symbolsInOFDM = [zeros(offset,1); symbolsIn; zeros(numFFT-offset-numDataCarriers,1)];
    ifftOut = ifft(ifftshift(symbolsInOFDM));
    
    % Prepend cyclic prefix
    txSigOFDM = [ifftOut(end-cpLen+1:end); ifftOut];
    
    % Filter, with zero-padding to flush tail. Get the transmit signal
    zeroPaddedFOFDM = myFilter(FirTxFilter, [txSigOFDM; zeros(L-1,1)]);
    start_idx = (numFFT+cpLen)*(symb_cnt-1)+1; 
    end_idx = (numFFT+cpLen)*symb_cnt;
    nonPaddedSignal = zeroPaddedFOFDM(L:end);
    txSigFOFDM(start_idx:end_idx, 1) = nonPaddedSignal;
    
    % Plot power spectral density (PSD)
    [psd,f] = periodogram(zeroPaddedFOFDM, rectwin(length(zeroPaddedFOFDM)), numFFT*2, 1, 'centered');
    plot(f,10*log10(psd));
    
    % Compute peak-to-average-power ratio (PAPR)
    PAPR = comm.CCDF('PAPROutputPort', true, 'PowerUnits', 'dBW');
    [~,~,paprFOFDM] = PAPR(zeroPaddedFOFDM);
    disp(['Peak-to-Average-Power-Ratio for F-OFDM = ' num2str(paprFOFDM) ' dB']);
    
    
    %% OFDM Modulation with Corresponding Parameters
    %
    % For comparison, we review the existing OFDM modulation technique, using
    % the full occupied band, with the same length cyclic prefix.
    
    % Plot power spectral density (PSD) for OFDM signal
    [psd,f] = periodogram(txSigOFDM, rectwin(length(txSigOFDM)), numFFT*2, ...
        1, 'centered');
    hFig1 = figure('Position', figposition([46 15 30 30]));
    plot(f,10*log10(psd));
    grid on
    axis([-0.5 0.5 -100 -20]);
    xlabel('Normalized frequency');
    ylabel('PSD (dBW/Hz)')
    title(['OFDM, ' num2str(numRBs*rbSize) ' Subcarriers'])
    
    % Compute peak-to-average-power ratio (PAPR)
    PAPR2 = comm.CCDF('PAPROutputPort', true, 'PowerUnits', 'dBW');
    [~,~,paprOFDM] = PAPR2(txSigOFDM);
    disp(['Peak-to-Average-Power-Ratio for OFDM = ' num2str(paprOFDM) ' dB']);
    
end

%%
% Comparing the plots of the spectral densities for CP-OFDM and F-OFDM
% schemes, F-OFDM has lower sidelobes. This allows a higher utilization of
% the allocated spectrum, leading to increased spectral efficiency.
%
% Refer to the <matlab:doc('comm.OFDMModulator') comm.OFDMModulator> System
% object which can also be used to implement the CP-OFDM modulation.

%% F-OFDM Receiver with No Channel
%
% The example next highlights the basic receive processing for F-OFDM for a
% single OFDM symbol. The received signal is passed through a matched
% filter, followed by the normal CP-OFDM receiver. It accounts for both the
% filtering ramp-up and latency prior to the FFT operation.
%
% No fading channel is considered in this example but noise is added to the
% received signal to achieve the desired SNR.

% Add WGN
rxSig = awgn(txSigFOFDM, snrdB, 'measured');

%%
% Receive processing operations are shown in the following F-OFDM receiver
% diagram.
%
% <<FOFDMReceiveDiagram.png>>

for symb_cnt=1:1:numberOfOFDMSymbols
    
    rxSig_symb = rxSig((numFFT+cpLen)*(symb_cnt-1)+1:(numFFT+cpLen)*symb_cnt, 1);
    
    if(enableRXMatchedFilter==1)
        % Receive matched filter
        rxSigFilt_myfilter = myFilter(FirRxFilter, [rxSig_symb; zeros(L-1,1)]);
        rxSigFilt = rxSigFilt_myfilter;
        
        % Account for filter delay
        rxSigFiltSync = rxSigFilt(L:end);
    else
        rxSigFiltSync = rxSig_symb;
    end
    
    splittedSymbol = rxSigFiltSync;
    
    % Remove cyclic prefix
    rxSymbol = splittedSymbol(cpLen+1:end);
    
    % Perform FFT
    RxSymbols = fftshift(fft(rxSymbol));
    
    % Channel Estimation
    if(enableRXMatchedFilter==0)
        H = RxSymbols(offset+(1:numDataCarriers))./symbolsInOFDM(offset+(1:numDataCarriers));
    end
    
    % Select data subcarriers
    dataRxSymbols = RxSymbols(offset+(1:numDataCarriers));
    
    % Channel Equalization
    if(enableRXMatchedFilter==0)
        eqOFDMSymbolSig = dataRxSymbols./H;
        dataRxSymbols = eqOFDMSymbolSig;
    end
    
    % Plot received symbols constellation
    switch bitsPerSubCarrier
        case 2  % QPSK
            refConst = qammod((0:3).', 4, 'UnitAveragePower', true);
        case 4  % 16QAM
            refConst = qammod((0:15).', 16,'UnitAveragePower', true);
        case 6  % 64QAM
            refConst = qammod((0:63).', 64,'UnitAveragePower', true);
        case 8  % 256QAM
            refConst = qammod((0:255).', 256,'UnitAveragePower', true);
    end
    constDiagRx = comm.ConstellationDiagram( ...
        'ShowReferenceConstellation', true, ...
        'ReferenceConstellation', refConst, ...
        'Position', figposition([20 15 25 30]), ...
        'MeasurementInterval', length(dataRxSymbols), ...
        'Title', 'F-OFDM Demodulated Symbols', ...
        'Name', 'F-OFDM Reception', ...
        'XLimits', [-1.5 1.5], 'YLimits', [-1.5 1.5]);
    constDiagRx(dataRxSymbols);
    
    % Channel equalization is not necessary here as no channel is modeled
    
    % Demapping and BER computation
    qamDemod = comm.RectangularQAMDemodulator('ModulationOrder', ...
        2^bitsPerSubCarrier, 'BitOutput', true, ...
        'NormalizationMethod', 'Average power');
    BER = comm.ErrorRate;
    
    % Perform hard decision and measure errors
    rxBits = qamDemod(dataRxSymbols);
    ber = BER(bitsIn, rxBits);
    
    disp(['F-OFDM Reception, BER = ' num2str(ber(1)) ' at SNR = ' ...
        num2str(snrdB) ' dB']);
    
end

% Restore RNG state
rng(s);

%%
% As highlighted, F-OFDM adds a filtering stage to the existing CP-OFDM
% processing at both the transmit and receive ends. The example models the
% full-band allocation for a user, but the same approach can be applied for
% multiple bands (one per user) for an uplink asynchronous operation.
%
% Refer to the <matlab:doc('comm.OFDMDemodulator') comm.OFDMDemodulator>
% System object which can be used to implement the CP-OFDM demodulation
% after receive matched filtering.

%% Conclusion and Further Exploration
%
% The example presents the basic characteristics of the F-OFDM modulation
% scheme at both transmit and receive ends of a communication system.
% Explore different system parameter values for the number of resource
% blocks, number of subcarriers per blocks, filter length, tone offset and
% SNR.
%
% Universal Filtered Multi-Carrier (UFMC) modulation scheme is another
% approach to subband filtered OFDM. For more information, see the
% <OFDMvsUFMCExample.html UFMC vs. OFDM Modulation> example. This F-OFDM
% example uses a single subband while the UFMC example uses multiple
% subbands.
%
% F-OFDM and UFMC both use time-domain filtering with subtle differences in
% the way the filter is designed and applied. For UFMC, the length of
% filter is constrained to be equal to the cyclic-prefix length, while for
% F-OFDM, it can exceed the CP length.
%
% Refer to <comm5GWaveformsLTE.html 5G Waveforms with LTE> example for how
% the scheme processes multiple symbols for the LTE PDSCH channel. The
% example shows the SISO and MIMO link-level processing, including channel
% estimation and equalization for F-OFDM, and compares it with UFMC and
% CP-OFDM.

%% Selected Bibliography
% # Abdoli J., Jia M. and Ma J., "Filtered OFDM: A New Waveform for
% Future Wireless Systems," 2015 IEEE 16th International Workshop on
% Signal Processing Advances in Wireless Communications (SPAWC),
% Stockholm, 2015, pp. 66-70.
% # R1-162152. "OFDM based flexible waveform for 5G." 3GPP TSG RAN WG1
% meeting 84bis. Huawei; HiSilicon. April 2016.
% # R1-165425. "F-OFDM scheme and filter design." 3GPP TSG RAN WG1 meeting
% 85. Huawei; HiSilicon. May 2016.

displayEndOfDemoMessage(mfilename)