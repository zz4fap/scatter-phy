clear all;close all;clc

%% Set RNG state for repeatability.
s = rng(211);

%% System Parameters and Initialization.

bw_idx = 3;

SUBFRAME_LENGTH = [1920 3840 5760 11520 15360 23040];
numFFT = [128 256 384 768 1024 1536];       % Number of FFT points
deltaF = 15000;                             % Subcarrier spacing
numRBs = [6 15 25 50 75 100];               % Number of resource blocks
rbSize = 12;                                % Number of subcarriers per resource block
cpLen_1st_symb  = [10 20 30 60 80 120];     % Cyclic prefix length in samples only for very 1st symbol.
cpLen = [9 18 27 54 72 108];                % Cylic prefix length for all the symbols different from the 1st.

bitsPerSubCarrier = 6;                      % 2: QPSK, 4: 16QAM, 6: 64QAM, 8: 256QAM
snrdB = 180;                                % SNR in dB

numDataCarriers = numRBs(bw_idx)*rbSize;    % number of data subcarriers in subband

ofdm_symbol_len = numFFT(bw_idx)+cpLen(bw_idx);

% Demapping and BER computation.
qamDemod = comm.RectangularQAMDemodulator('ModulationOrder', 2^bitsPerSubCarrier, 'BitOutput', true, 'NormalizationMethod', 'Average power');
BER = comm.ErrorRate;

% Compute peak-to-average-power ratio (PAPR)
%ccdf = comm.CCDF('PAPROutputPort', true, 'PowerUnits', 'dBW');
ccdf = comm.CCDF('PAPROutputPort',true,'MaximumPowerLimit', 50); % 50 dBm.

%% Filtered-OFDM Filter Design.
toneOffset = 2.5;          % Tone offset or excess bandwidth (in subcarriers)
L = 128 + 1;             % Filter length (= filterOrder + 1), odd

halfFilt = floor(L/2);
n = -halfFilt:halfFilt;

% Sinc function prototype filter
pb = sinc((numDataCarriers+2*toneOffset).*n./numFFT(bw_idx));

% Sinc truncation window
w = (0.5*(1+cos(2*pi.*n/(L-1)))).^0.6;

% Normalized lowpass filter coefficients
fnum = (pb.*w)/sum(pb.*w);

% Use dsp filter objects for filtering
filtTx = dsp.FIRFilter('Structure', 'Direct form', 'Numerator', fnum);

FirTxFilter = filtTx.Numerator;

%% Main loop

numFrames = 100;

txSigQAM = repmat(zeros(numDataCarriers, 1), numFrames, 1);

txSigOFDM = repmat(zeros(ofdm_symbol_len, 1), numFrames, 1);

txSigFOFDM = repmat(zeros(ofdm_symbol_len, 1), numFrames, 1);

for k = 1:numFrames
    
    % ----------------------------- 64QAM ---------------------------------
    % Generate random data symbols.
    data = randi([0 63], numDataCarriers, 1);
    % Apply 64-QAM modulation
    tmp_txSigQAM = qammod(data, 64);
    
    % ----------------------------- OFDM ----------------------------------
    % Pack data into an OFDM symbol
    offset = (numFFT(bw_idx)-numDataCarriers)/2; % for band center
    symbolsInOFDM = [zeros(offset,1); tmp_txSigQAM; zeros(numFFT(bw_idx)-offset-numDataCarriers,1)];
    ifftOut = ifft(ifftshift(symbolsInOFDM));
    
    % Prepend cyclic prefix
    tmp_txSigOFDM = [ifftOut(end-cpLen(bw_idx)+1:end); ifftOut];
    
    % ----------------------------- F-OFDM ----------------------------------
    % Filter, with zero-padding to flush tail. Get the transmit signal.
    tmp_txSigFOFDM = myFilter(FirTxFilter, [tmp_txSigOFDM; zeros(L-1,1)]);
    tmp_txSigFOFDM = tmp_txSigFOFDM(L:end);
    
    % ----------------------- Save the signal data ------------------------
    txSigQAM((1:[numDataCarriers 1])+(k-1)*numDataCarriers) = tmp_txSigQAM;
    
    txSigOFDM((1:([ofdm_symbol_len 1]))+(k-1)*(ofdm_symbol_len)) = tmp_txSigOFDM;
    
    txSigFOFDM((1:([ofdm_symbol_len 1]))+(k-1)*(ofdm_symbol_len)) = tmp_txSigFOFDM;
end

[Fy,Fx,PAPR] = ccdf([txSigQAM(1:6000), txSigOFDM(1:6000), txSigFOFDM(1:6000)]);

plot(ccdf)
legend('QAM', 'OFDM', 'F-OFDM', 'location','best')

fprintf('\nPAPR for 64-QAM = %5.2f dB\nPAPR for OFDM = %5.2f dB\nPAPR for F-OFDM = %5.2f dB\n', PAPR(1), PAPR(2), PAPR(3))


