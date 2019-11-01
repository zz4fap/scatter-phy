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

% clear all;
% close all;
% clc

phy_bw_idx = 1;

numFFT = [128 256 384 768 1024 1536];       % Number of FFT points
deltaF = 15000;                             % Subcarrier spacing
% numRBs = [6 15 25 50 75 100];               % Legacy physical resource blocks
numRBs = [7 15 25 50 75 100];               % Guard band harvested physical resource blocks
rbSize = 12;                                % Number of subcarriers per resource block

toneOffset = 5;                             % Tone offset or excess bandwidth (in subcarriers)
L = 129;                                    % Filter length (=filterOrder+1), odd

%% Filtered-OFDM Filter Design
numDataCarriers = numRBs(phy_bw_idx)*rbSize;    % number of data subcarriers in subband
halfFilt = floor(L/2);
n = -halfFilt:halfFilt;

% Sinc function prototype filter
pb = sinc((numDataCarriers+2*toneOffset).*n./numFFT(phy_bw_idx));

% Sinc truncation window
w = (0.5*(1+cos(2*pi.*n/(L-1)))).^1.2;

% Normalized lowpass filter coefficients
fnum = (pb.*w)/sum(pb.*w);

% Filter impulse response
if(1)
    h = fvtool(fnum, 'Analysis', 'impulse', 'NormalizedFrequency', 'off', 'Fs', numFFT(phy_bw_idx)*deltaF);
    h.CurrentAxes.XLabel.String = 'Time (\mus)';
    h.FigureToolbar = 'off';
end

% Convert filter coefficients to fixed point values
fnum_fix = fi(fnum,1,16,15);
fnum_int = fnum_fix.int(1:L/2+1);

% disp(fnum_fix.int(1:L/2+1));

