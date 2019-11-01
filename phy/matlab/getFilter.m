function [filtTx, FirTxFilter, filtRx, FirRxFilter, L] = getFilter(phy_bw_to_be_used, filterOrder, toneOffset)

numFFT = [128 256 384 768 1536];            % Number of FFT points
numRBs = [6 15 25 50 100];             % Number of resource blocks
rbSize = 12;             % Number of subcarriers per resource block

L = filterOrder + 1;                 % Filter length (=filterOrder+1), odd

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
w = (0.5*(1+cos(2*pi.*n/(L-1)))).^0.6;

% Normalized lowpass filter coefficients
fnum = (pb.*w)/sum(pb.*w);

% Use dsp filter objects for filtering
filtTx = dsp.FIRFilter('Structure', 'Direct form', 'Numerator', fnum);

FirTxFilter = filtTx.Numerator;

filtRx = clone(filtTx); % Matched filter for the Rx

FirRxFilter = filtRx.Numerator;
