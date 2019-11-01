clear all;close all;clc

% Set RNG state for repeatability
s = rng(211);

%% System Parameters
%
% Define system parameters for the example. These parameters can be
% modified to explore their impact on the system.

subframe_size = 5760;

phy_bw_to_be_used = 3;

numFFT = [128 256 384];            % Number of FFT points
deltaF = 15000;          % Subcarrier spacing
numRBs = [6 15 25];             % Number of resource blocks
rbSize = 12;             % Number of subcarriers per resource block
cpLen  = [9 18 27];             % Cyclic prefix length in samples

%% -------------------------------------------------------------
filename = "/home/zz4fap/work/fofdm/scatter/build/cp_ofdm_rx_side_assessment_5MHz_v4.dat";
fileID = fopen(filename);
ofdm_subframe = complex(zeros(1,subframe_size),zeros(1,subframe_size));
idx = 1;
value = fread(fileID, 2, 'float');
while ~feof(fileID)
    ofdm_subframe(idx) = complex(value(1),value(2));
    value = fread(fileID, 2, 'float');
    idx = idx + 1;
end
fclose(fileID);

% Plot power spectral density (PSD)
signal2plot = ofdm_subframe;
%hFig = figure('Position', figposition([1.5*46 1.5*50 1.5*30 1.5*30]), 'MenuBar', 'none');
hFig = figure;
%axis([-0.5 0.5 -50 0]);
hold on; 
grid on
xlabel('Normalized frequency');
ylabel('PSD (dBW/Hz)')
title(['OFDM, ' num2str(numRBs(phy_bw_to_be_used)) ' - Resource Blocks' num2str(rbSize) ' - No Filter'])
[psd_ofdm,f_ofdm] = periodogram(signal2plot, rectwin(length(signal2plot)), numFFT(phy_bw_to_be_used)*2, 1, 'centered'); 
plot(f_ofdm,10*log10(psd_ofdm));

% Compute peak-to-average-power ratio (PAPR)
PAPR = comm.CCDF('PAPROutputPort', true, 'PowerUnits', 'dBW');
[~,~,paprOFDM] = PAPR(signal2plot.');
disp(['Peak-to-Average-Power-Ratio for OFDM = ' num2str(paprOFDM) ' dB']);

%% -------------------------------------------------------------
filename = "/home/zz4fap/work/fofdm/scatter/build/f_ofdm_rx_side_assessment_5MHz_fir_65.dat";
fileID = fopen(filename);
fofdm_subframe = complex(zeros(1,subframe_size),zeros(1,subframe_size));
idx = 1;
value = fread(fileID, 2, 'float');
while ~feof(fileID)
    fofdm_subframe(idx) = complex(value(1),value(2));
    value = fread(fileID, 2, 'float');
    idx = idx + 1;
end
fclose(fileID);

% Plot power spectral density (PSD)
signal2plot = fofdm_subframe;
%hFig = figure('Position', figposition([1.5*46 1.5*50 1.5*30 1.5*30]), 'MenuBar', 'none');
hFig = figure;
%axis([-0.5 0.5 -50 0]);
hold on; 
grid on
xlabel('Normalized frequency');
ylabel('PSD (dBW/Hz)')
title(['F-OFDM - ' num2str(numRBs(phy_bw_to_be_used)) ' Resource Blocks', ' - FIR order: 64'])
[psd_fofdm_65,f_fofdm_65] = periodogram(signal2plot, rectwin(length(signal2plot)), numFFT(phy_bw_to_be_used)*2, 1, 'centered'); 
plot(f_fofdm_65,10*log10(psd_fofdm_65));

% Compute peak-to-average-power ratio (PAPR)
PAPR = comm.CCDF('PAPROutputPort', true, 'PowerUnits', 'dBW');
[~,~,paprFOFDM] = PAPR(signal2plot.');
disp(['Peak-to-Average-Power-Ratio for F-OFDM = ' num2str(paprFOFDM) ' dB']);

%% -------------------------------------------------------------
%filename = "/home/zz4fap/work/fofdm/scatter/build/f_ofdm_rx_side_assessment_5MHz_fir_129.dat";
filename = "/home/zz4fap/work/fofdm/scatter/build/f_ofdm_rx_side_assessment_5MHz_fir_129_v1.dat";
fileID = fopen(filename);
fofdm_subframe = complex(zeros(1,subframe_size),zeros(1,subframe_size));
idx = 1;
value = fread(fileID, 2, 'float');
while ~feof(fileID)
    fofdm_subframe(idx) = complex(value(1),value(2));
    value = fread(fileID, 2, 'float');
    idx = idx + 1;
end
fclose(fileID);

% Plot power spectral density (PSD)
signal2plot = fofdm_subframe;
%hFig = figure('Position', figposition([1.5*46 1.5*50 1.5*30 1.5*30]), 'MenuBar', 'none');
hFig = figure;
%axis([-0.5 0.5 -50 0]);
hold on; 
grid on
xlabel('Normalized frequency');
ylabel('PSD (dBW/Hz)')
title(['F-OFDM - ' num2str(numRBs(phy_bw_to_be_used)) ' Resource Blocks', ' - FIR order: 128'])
[psd_fofdm_129,f_fofdm_129] = periodogram(signal2plot, rectwin(length(signal2plot)), numFFT(phy_bw_to_be_used)*2, 1, 'centered'); 
plot(f_fofdm_129,10*log10(psd_fofdm_129));

% Compute peak-to-average-power ratio (PAPR)
PAPR = comm.CCDF('PAPROutputPort', true, 'PowerUnits', 'dBW');
[~,~,paprFOFDM] = PAPR(signal2plot.');
disp(['Peak-to-Average-Power-Ratio for F-OFDM = ' num2str(paprFOFDM) ' dB']);

%% -------------------------------------------------------------
filename = "/home/zz4fap/work/fofdm/scatter/build/f_ofdm_rx_side_assessment_5MHz_fir_257.dat";
fileID = fopen(filename);
fofdm_subframe = complex(zeros(1,subframe_size),zeros(1,subframe_size));
idx = 1;
value = fread(fileID, 2, 'float');
while ~feof(fileID)
    fofdm_subframe(idx) = complex(value(1),value(2));
    value = fread(fileID, 2, 'float');
    idx = idx + 1;
end
fclose(fileID);

% Plot power spectral density (PSD)
signal2plot = fofdm_subframe;
%hFig = figure('Position', figposition([1.5*46 1.5*50 1.5*30 1.5*30]), 'MenuBar', 'none');
hFig = figure;
%axis([-0.5 0.5 -50 0]);
hold on; 
grid on
xlabel('Normalized frequency');
ylabel('PSD (dBW/Hz)')
title(['F-OFDM - ' num2str(numRBs(phy_bw_to_be_used)) ' Resource Blocks', '- FIR order: 256'])
[psd_fofdm_257,f_fofdm_257] = periodogram(signal2plot, rectwin(length(signal2plot)), numFFT(phy_bw_to_be_used)*2, 1, 'centered'); 
plot(f_fofdm_257,10*log10(psd_fofdm_257));

% Compute peak-to-average-power ratio (PAPR)
PAPR = comm.CCDF('PAPROutputPort', true, 'PowerUnits', 'dBW');
[~,~,paprFOFDM] = PAPR(signal2plot.');
disp(['Peak-to-Average-Power-Ratio for F-OFDM = ' num2str(paprFOFDM) ' dB']);


%% -------------------------------------------------------------
%filename = "/home/zz4fap/work/fofdm/scatter/build/f_ofdm_rx_side_assessment_5MHz_fir_513.dat";
filename = "/home/zz4fap/work/fofdm/scatter/build/f_ofdm_rx_side_assessment_5MHz_fir_513_v1.dat";
fileID = fopen(filename);
fofdm_subframe = complex(zeros(1,subframe_size),zeros(1,subframe_size));
idx = 1;
value = fread(fileID, 2, 'float');
while ~feof(fileID)
    fofdm_subframe(idx) = complex(value(1),value(2));
    value = fread(fileID, 2, 'float');
    idx = idx + 1;
end
fclose(fileID);

% Plot power spectral density (PSD)
signal2plot = fofdm_subframe;
%hFig = figure('Position', figposition([1.5*46 1.5*50 1.5*30 1.5*30]), 'MenuBar', 'none');
hFig = figure;
%axis([-0.5 0.5 -50 0]);
hold on; 
grid on
xlabel('Normalized frequency');
ylabel('PSD (dBW/Hz)')
title(['F-OFDM - ' num2str(numRBs(phy_bw_to_be_used)) ' Resource Blocks', ' - FIR order: 512'])
[psd_fofdm_513,f_fofdm_513] = periodogram(signal2plot, rectwin(length(signal2plot)), numFFT(phy_bw_to_be_used)*2, 1, 'centered'); 
plot(f_fofdm_513,10*log10(psd_fofdm_513));

% Compute peak-to-average-power ratio (PAPR)
PAPR = comm.CCDF('PAPROutputPort', true, 'PowerUnits', 'dBW');
[~,~,paprFOFDM] = PAPR(signal2plot.');
disp(['Peak-to-Average-Power-Ratio for F-OFDM = ' num2str(paprFOFDM) ' dB']);

%% ------------------ Figure with all 4 FIR Filters ------------------------
figure;
plot(f_ofdm,10*log10(psd_ofdm),'b');
hold on
plot(f_fofdm_65,10*log10(psd_fofdm_65),'c');
plot(f_fofdm_129,10*log10(psd_fofdm_129),'r');
plot(f_fofdm_257,10*log10(psd_fofdm_257),'g');
plot(f_fofdm_513,10*log10(psd_fofdm_513),'k');
axis([-0.5 0.5 -80 -10]);
xlabel('Normalized frequency');
ylabel('PSD (dBW/Hz)')
title(['OFDM vs. F-OFDM - ' num2str(numRBs(phy_bw_to_be_used)) ' Resource Blocks - Rx Side'])
legend('OFDM (no filter)','f-OFDM - FIR order: 64','f-OFDM - FIR order: 128', 'f-OFDM - FIR order: 256', 'f-OFDM - FIR order: 512')
grid on
hold off

%% ------------------ Figure with only 1 FIR Filter ------------------------
figure;
plot(f_ofdm,10*log10(psd_ofdm),'b');
hold on
plot(f_fofdm_257,10*log10(psd_fofdm_257),'k');
axis([-0.5 0.5 -80 -10]);
xlabel('Normalized frequency');
ylabel('PSD (dBW/Hz)')
title(['OFDM vs. F-OFDM - ' num2str(numRBs(phy_bw_to_be_used)) ' Resource Blocks - Rx Side'])
legend('OFDM (no filter)', 'f-OFDM - FIR order: 512')
grid on
hold off


%% ------------------ Figure with some FIR Filters ------------------------
figure;
plot(f_ofdm,10*log10(psd_ofdm),'b');
hold on
plot(f_fofdm_65,10*log10(psd_fofdm_65),'r');
%plot(f_fofdm_129,10*log10(psd_fofdm_129),'r');
plot(f_fofdm_257,10*log10(psd_fofdm_257),'k');
%plot(f_fofdm_513,10*log10(psd_fofdm_513),'k');
axis([-0.5 0.5 -80 -10]);
xlabel('Normalized frequency');
ylabel('PSD (dBW/Hz)')
title(['OFDM vs. F-OFDM - ' num2str(numRBs(phy_bw_to_be_used)) ' Resource Blocks - Rx Side'])
legend('OFDM (no filter)','f-OFDM - FIR order: 64','f-OFDM - FIR order: 128')
grid on
hold off