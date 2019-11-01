clear all;close all;clc

rng(1)

cell_id = 0;
NDFT = 384; % For 5 MHz PHY BW.
SRSLTE_PSS_LEN = 62;
FRAME_SIZE = 5760; % For 5 MHz PHY BW.
delta_f = 15000;

% Load signal.
filename = '/home/zz4fap/work/dcf_tdma/scatter/build/transmitted_subframe_1_cnt_0.dat';
fileID = fopen(filename);
ofdm_subframe = complex(zeros(1,FRAME_SIZE),zeros(1,FRAME_SIZE));
idx = 1;
value = fread(fileID, 2, 'float');
while ~feof(fileID)
    ofdm_subframe(idx) = complex(value(1),value(2));
    value = fread(fileID, 2, 'float');
    idx = idx + 1;
end
fclose(fileID);

% retrive received PSS.
rx_pss = ofdm_subframe((FRAME_SIZE/2)-NDFT+1:FRAME_SIZE/2).';

RX_PSS = fft(rx_pss,NDFT);
%figure;plot(abs(RX_PSS))

RX_PSS_filt = zeros(1,NDFT);
RX_PSS_filt([1:32 354:384]) = RX_PSS([1:32 354:384]);
RX_PSS_filt = RX_PSS_filt.';
%figure;plot(abs(RX_PSS_filt))
rx_pss_filt = ifft(RX_PSS_filt,NDFT).';
rx_pss_filt = rx_pss_filt;

% Generate PSS signal.
local_pss = lte_pss_zc(cell_id);

% Create OFDM symbol number 6, the one which carries PSS.
pss_zero_pad = [0;local_pss((SRSLTE_PSS_LEN/2)+1:end);zeros(NDFT-(SRSLTE_PSS_LEN+1),1);local_pss(1:SRSLTE_PSS_LEN/2)];
%figure;plot(abs(pss_zero_pad))
%pss_zero_pad = [0;pss(1:SRSLTE_PSS_LEN/2);zeros(NDFT-(SRSLTE_PSS_LEN+1),1);pss((SRSLTE_PSS_LEN/2)+1:end)];

local_time_domain_pss = (ifft(pss_zero_pad, NDFT)*(1/SRSLTE_PSS_LEN));

local_time_domain_pss_power = sum(abs(local_time_domain_pss(:)).^2)/length(local_time_domain_pss(:));

%% ------------------------------------------------------------------------
% Generate frequency offset.
fs = NDFT*delta_f;
N = 384*100000;

n = 0:1:NDFT-1;

f0 = fs/N;

max_freq = (delta_f/2)/f0;

numIter = 10000;

SNR = 0:5:30; %1000;

SNR_linear = 10.^(SNR./10);

% Baseband FIR for 1.4MHz Rx,
fir_coef = fir1(512, (64*delta_f)/(NDFT*delta_f)).';
len_fir_half = (length(fir_coef)-1)/2;
%freqz(fir_coef,1,512)

RX_PSS_filt = zeros(1,NDFT);

error_simple_avg = zeros(1,length(SNR));
error_filtered_avg = zeros(1,length(SNR));
for snrIdx = 1:1:length(SNR)
    
    for iter=1:1:numIter
        
        k0 = randi(max_freq + 1) - 1;
        
        cfo_freq = f0*k0;
        
        freq_offset = exp(1i.*2.*pi.*k0.*n./N);
        
        %% ------------------------------------------------------------------------
        % Pass signal through channel.
        
        cfo_signal = (rx_pss.*(freq_offset(1:1:NDFT).'));
        
        rx_signal = my_awgn(cfo_signal, SNR(snrIdx), 'measured'); %cfo_signal
        
        %% ------------------------------------------------------------------------
        % Execute dot-product.
        
        y = rx_signal.*conj(local_time_domain_pss);
        
        %% ------------------------------------------------------------------------
        % PSS-based CFO estimation.
        % OBS.: Maximum CFO frequency estimation is 15 KHz.
        
        ycorr0 = sum(y(1:1:NDFT/2));
        
        ycorr1 = sum(y((NDFT/2)+1:1:NDFT));
        
        cfo_est = angle(conj(ycorr0).*ycorr1)/pi;
        
        cfo_freq_est = cfo_est*15000;
        
        error_simple = abs(cfo_freq - cfo_freq_est).^2;
        
        error_simple_avg(snrIdx) = error_simple_avg(snrIdx) + error_simple;
        
        %% ------------------------------------------------------------------------
        % Filtered-PSS-based CFO estimation.    
        
        %filtered_rx_signal = conv(rx_signal, fir_coef);
        rx_signal_fft = fft(rx_signal, NDFT);
        RX_PSS_filt([1:32 354:384]) = rx_signal_fft([1:32 354:384]);
        %figure;plot(abs(RX_PSS_filt))
        filtered_rx_signal = ifft(RX_PSS_filt,NDFT).';
        %figure;plot(real(filtered_rx_signal))
        
        y = filtered_rx_signal.*conj(local_time_domain_pss);
        
        filt_ycorr0 = sum(y(1:1:NDFT/2));
        
        filt_ycorr1 = sum(y((NDFT/2)+1:1:NDFT));
        
        cfo_est_filt = angle(conj(filt_ycorr0).*filt_ycorr1)/pi;
        
        filt_cfo_freq_est = cfo_est_filt*15000;
        
        error_filt = abs(cfo_freq - filt_cfo_freq_est).^2;
        
        error_filtered_avg(snrIdx) = error_filtered_avg(snrIdx) + error_filt;        
        
        
    end
    
    error_simple_avg(snrIdx) = error_simple_avg(snrIdx)/numIter;
    error_filtered_avg(snrIdx) = error_filtered_avg(snrIdx)/numIter;
    
    fprintf(1,'SNR: %1.2f\n', SNR(snrIdx));
    fprintf(1,'PSS-based CFO Estimate Error: %1.2e\n', error_simple_avg(snrIdx));
    fprintf(1,'Filtered-PSS-based CFO Estimate Error: %1.2e\n', error_filtered_avg(snrIdx));
    fprintf(1,'-----------------------------------------------\n');
    
end

figure;
semilogy(SNR, error_simple_avg);
hold on;
semilogy(SNR, error_filtered_avg);
hold off;
grid on;
xlabel('MSE')
title('CFO estimation')
legend('Traditional','Filtered')
