clear all;close all;clc

% Set RNG state for repeatability
s = rng(211);

%% System Parameters

bw_idx = 3;

SUBFRAME_LENGTH = [1920 3840 5760 11520 15360 23040];
numFFT = [128 256 384 768 1024 1536];       % Number of FFT points
deltaF = 15000;                             % Subcarrier spacing
numRBs = [6 15 25 50 75 100];               % Number of resource blocks
rbSize = 12;                                % Number of subcarriers per resource block
cpLen_1st_symb  = [10 20 30 60 80 120];     % Cyclic prefix length in samples only for very 1st symbol.
cpLen = [9 18 27 54 72 108];

enableRXMatchedFilter = 1; % Enable or disable channel estimation and equalization

bitsPerSubCarrier = 6;     % 2: QPSK, 4: 16QAM, 6: 64QAM, 8: 256QAM
snrdB = 180;               % SNR in dB

toneOffset = 1;          % Tone offset or excess bandwidth (in subcarriers)
L = 256 + 1;             % Filter length (= filterOrder + 1), odd

%% Filtered-OFDM Filter Design
numDataCarriers = numRBs(bw_idx)*rbSize;    % number of data subcarriers in subband
halfFilt = floor(L/2);
n = -halfFilt:halfFilt;

% Sinc function prototype filter
pb = sinc((numDataCarriers+2*toneOffset).*n./numFFT(bw_idx));

% Sinc truncation window
w = (0.5*(1+cos(2*pi.*n/(L-1)))).^1.2; %0.6

% Normalized lowpass filter coefficients
fnum = (pb.*w)/sum(pb.*w);

% Use dsp filter objects for filtering
filtTx = dsp.FIRFilter('Structure', 'Direct form', 'Numerator', fnum);
filtRx = clone(filtTx); % Matched filter for the Rx

FirTxFilter = filtTx.Numerator;

FirRxFilter = filtRx.Numerator;

% QAM Symbol mapper
qamMapper = comm.RectangularQAMModulator('ModulationOrder', 2^bitsPerSubCarrier, 'BitInput', true, 'NormalizationMethod', 'Average power');

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

% Channel equalization is not necessary here as no channel is modeled

% Demapping and BER computation
qamDemod = comm.RectangularQAMDemodulator('ModulationOrder', 2^bitsPerSubCarrier, 'BitOutput', true, 'NormalizationMethod', 'Average power');
BER = comm.ErrorRate;

%% F-OFDM Transmit Processing
bitsIn = randi([0 1], bitsPerSubCarrier*numDataCarriers, 1);

% Generate data symbols
symbolsIn = qamMapper(bitsIn(1:end));
    
% Pack data into an OFDM symbol
offset = (numFFT(bw_idx)-numDataCarriers)/2; % for band center
symbolsInOFDM = [zeros(offset,1); symbolsIn; zeros(numFFT(bw_idx)-offset-numDataCarriers,1)];
ifftOut = ifft(ifftshift(symbolsInOFDM));
    
% Prepend cyclic prefix
txSigOFDM = [ifftOut(end-cpLen(bw_idx)+1:end); ifftOut];

% Filter, with zero-padding to flush tail. Get the transmit signal
txSigFOFDM_myfilter = myFilter(FirTxFilter, [txSigOFDM; zeros(L-1,1)]);
txSigFOFDM = txSigFOFDM_myfilter;

% Compute peak-to-average-power ratio (PAPR)
PAPR = comm.CCDF('PAPROutputPort', true, 'PowerUnits', 'dBW');
[~,~,paprFOFDM] = PAPR(txSigFOFDM);
disp(['Peak-to-Average-Power-Ratio for F-OFDM = ' num2str(paprFOFDM) ' dB']);


%% OFDM Modulation with Corresponding Parameters
% Compute peak-to-average-power ratio (PAPR)
PAPR2 = comm.CCDF('PAPROutputPort', true, 'PowerUnits', 'dBW');
[~,~,paprOFDM] = PAPR2(txSigOFDM);
disp(['Peak-to-Average-Power-Ratio for OFDM = ' num2str(paprOFDM) ' dB']);

%% SC-FDMA Modulation with Corresponding Parameters

% Pack data into an OFDM symbol
offset = (numFFT(bw_idx)-numDataCarriers)/2; % for band center

symbolsInSCFDMA = [zeros(offset,1); symbolsIn; zeros(numFFT(bw_idx)-offset-numDataCarriers,1)];

aux = fft(symbolsInSCFDMA);

ifftOut = ifft(ifftshift(aux));
    
% Prepend cyclic prefix
txSigSCFDMA = [ifftOut(end-cpLen(bw_idx)+1:end); ifftOut];

% Compute peak-to-average-power ratio (PAPR)
PAPR3 = comm.CCDF('PAPROutputPort', true, 'PowerUnits', 'dBW');
[~,~,paprSCFDMA] = PAPR3(txSigSCFDMA);
disp(['Peak-to-Average-Power-Ratio for SC-FDMA = ' num2str(paprSCFDMA) ' dB']);


