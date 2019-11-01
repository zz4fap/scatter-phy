clear all;close all;clc

% Set RNG state for repeatability
s = rng(211);

%% System Parameters
%
% Define system parameters for the example. These parameters can be
% modified to explore their impact on the system.

phy_bw_to_be_used = 3;

numFFT = [128 256 384];            % Number of FFT points
deltaF = 15000;          % Subcarrier spacing
numRBs = [6 15 25];             % Number of resource blocks
rbSize = 12;             % Number of subcarriers per resource block
cpLen  = [9 18 27];             % Cyclic prefix length in samples

%-load('/home/zz4fap/backup/rx_sync_thread/scatter/build/phy/srslte/examples/ofdm_assessment_two_columns_format.dat');
%load('rx_filter_25rb.mat');
load('/home/zz4fap/work/fofdm/scatter/build/cp_ofdm_rx_side_assessment_5MHz_v0.dat')
re = ofdm_assessment_two_columns_format(:,1);
im = ofdm_assessment_two_columns_format(:,2);

ofdm = complex(re,im);

% Plot power spectral density (PSD)
signal2plot = ofdm;
%hFig = figure('Position', figposition([1.5*46 1.5*50 1.5*30 1.5*30]), 'MenuBar', 'none');
hFig = figure;
axis([-0.5 0.5 -50 0]);
hold on; 
grid on
xlabel('Normalized frequency');
ylabel('PSD (dBW/Hz)')
title(['OFDM, ' num2str(numRBs(phy_bw_to_be_used)) ' Resource blocks, ' num2str(rbSize) ' Subcarriers each, '])
[psd_ofdm,f_ofdm] = periodogram(signal2plot, rectwin(length(signal2plot)), numFFT(phy_bw_to_be_used)*2, 1, 'centered'); 
plot(f_ofdm,10*log10(psd_ofdm));

% Compute peak-to-average-power ratio (PAPR)
PAPR = comm.CCDF('PAPROutputPort', true, 'PowerUnits', 'dBW');
[~,~,paprOFDM] = PAPR(signal2plot);
disp(['Peak-to-Average-Power-Ratio for OFDM = ' num2str(paprOFDM) ' dB']);

%% -------------------------------------------------------------

load('/home/zz4fap/backup/rx_sync_thread/scatter/build/phy/srslte/examples/f_ofdm_assessment_two_columns_format.dat');
re = f_ofdm_assessment_two_columns_format(:,1);
im = f_ofdm_assessment_two_columns_format(:,2);

fofdm = complex(re,im);

% Plot power spectral density (PSD)
signal2plot = fofdm;
%hFig = figure('Position', figposition([1.5*46 1.5*50 1.5*30 1.5*30]), 'MenuBar', 'none');
hFig = figure;
axis([-0.5 0.5 -150 0]);
hold on; 
grid on
xlabel('Normalized frequency');
ylabel('PSD (dBW/Hz)')
title(['F-OFDM, ' num2str(numRBs(phy_bw_to_be_used)) ' Resource blocks, ' num2str(rbSize) ' Subcarriers each, '])
[psd_fofdm,f_fofdm] = periodogram(signal2plot, rectwin(length(signal2plot)), numFFT(phy_bw_to_be_used)*2, 1, 'centered'); 
plot(f_fofdm,10*log10(psd_fofdm));

% Compute peak-to-average-power ratio (PAPR)
PAPR = comm.CCDF('PAPROutputPort', true, 'PowerUnits', 'dBW');
[~,~,paprFOFDM] = PAPR(signal2plot);
disp(['Peak-to-Average-Power-Ratio for F-OFDM = ' num2str(paprFOFDM) ' dB']);

%% -------------------------------------

figure;
axis([-0.5 0.5 -150 0]);
plot(f_ofdm,10*log10(psd_ofdm),'b');
hold on
plot(f_fofdm,10*log10(psd_fofdm),'r');
xlabel('Normalized frequency');
ylabel('PSD (dBW/Hz)')
title(['OFDM vs. F-OFDM, ' num2str(numRBs(phy_bw_to_be_used)) ' Resource blocks'])
legend('OFDM','f-OFDM')
grid on
hold off
