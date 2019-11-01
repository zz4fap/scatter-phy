clear all;close all;clc

% Set RNG state for repeatability
s = rng(211);

%% System Parameters
%
% Define system parameters for the example. These parameters can be
% modified to explore their impact on the system.

phy_bw_to_be_used = 3;

numFFT = [128 256 384 1024];            % Number of FFT points
deltaF = 15000;          % Subcarrier spacing
numRBs = [6 15 25 50];             % Number of resource blocks
rbSize = 12;             % Number of subcarriers per resource block
cpLen  = [9 18 27 32];             % Cyclic prefix length in samples

bitsPerSubCarrier = 6;   % 2: QPSK, 4: 16QAM, 6: 64QAM, 8: 256QAM

toneOffset = 1;          % Tone offset or excess bandwidth (in subcarriers)
L = 256+1;                 % Filter length (=filterOrder+1), odd

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

%error_tx_rx_filter = calculateError(FirTxFilter,FirRxFilter);

% Prepend cyclic prefix
N = 1000;
txSigOFDM = complex(randn(N,1),randn(N,1));


%load('/home/zz4fap/backup/rx_sync_thread/scatter/build/phy/srslte/examples/ofdm_assessment_two_columns_format.dat');
%re = ofdm_assessment_two_columns_format(:,1);
%im = ofdm_assessment_two_columns_format(:,2);
%txSigOFDM = complex(re,im);

% Zero-padding to flush tail.;
packet1 = [txSigOFDM; zeros(L-1,1);];
txSigOFDMPadded = [packet1];

% Filter signal. Get the transmit signal
txSigFOFDM = filtTx(txSigOFDMPadded);

% Apply implemented filter.
txSigFOFDM2 = myFilterv2(FirTxFilter, txSigOFDMPadded);


%for i=1:1:length(txSigFOFDM2)
%    fprintf(1,'packet %d: %f,%f\n',i-1,real(txSigFOFDM2(i)),imag(txSigFOFDM2(i)));
%end

error_filters = calculateError(txSigFOFDM,txSigFOFDM2);
if(error_filters > 1e-14)
   fprintf(1,'Average error too big!\n'); 
end

txSigFOFDM_part1 = myFilterv2(FirTxFilter, packet1);

%packet1_packet2 = [packet1(end-L+1:end); packet2];
% for i=1:1:length(packet1_packet2)
%     fprintf(1,'packet %d: %f,%f\n',i-1,real(packet1_packet2(i)),imag(packet1_packet2(i)));
% end

%txSigFOFDM_part2 = myFilterv2(FirTxFilter, packet1_packet2);
%for i=1:1:length(packet1_packet2)
    %fprintf(1,'txSigFOFDM_part2 %d: %e,%e\n',i-1,real(txSigFOFDM_part2(i)),imag(txSigFOFDM_part2(i)));
%end

% txSigFOFDM_join_part2 = txSigFOFDM_part2(L+1:end);
% txSigFOFDM_join = [txSigFOFDM_part1; txSigFOFDM_join_part2];
% 
% error_filters = calculateError(txSigFOFDM_join,txSigFOFDM2);
% if(error_filters > 1e-14)
%    fprintf(1,'Average error too big!\n'); 
% end

%% ------------------------------------------------------
if(1)
signalToPrint = txSigFOFDM2;
is_complex = 1;
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
        if(mod(i,20)==0)
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
        if(mod(i,20)==0)
            fprintf(1,'\n');
        end
    end
end
end
