clear all;close all;clc

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
figure;plot(abs(RX_PSS_filt))
rx_pss_filt = ifft(RX_PSS_filt,NDFT).';
rx_pss_filt = rx_pss_filt;

% Generate PSS signal.
local_pss = lte_pss_zc(cell_id);

% Create OFDM symbol number 6, the one which carries PSS.
%pss_zero_pad = [0;pss((SRSLTE_PSS_LEN/2)+1:end);zeros(NDFT-(SRSLTE_PSS_LEN+1),1);pss(1:SRSLTE_PSS_LEN/2)];
%figure;plot(abs(pss_zero_pad))
%pss_zero_pad = [0;pss(1:SRSLTE_PSS_LEN/2);zeros(NDFT-(SRSLTE_PSS_LEN+1),1);pss((SRSLTE_PSS_LEN/2)+1:end)];

pss_zero_pad(162:161+SRSLTE_PSS_LEN) = local_pss;

load('a.mat')

local_time_domain_pss = a.'; %(ifft(pss_zero_pad, NDFT)*(1/SRSLTE_PSS_LEN)).';

local_time_domain_pss_power = sum(abs(local_time_domain_pss(:)).^2)/length(local_time_domain_pss(:));

%% ------------------------------------------------------------------------
% Generate frequency offset.
fs = NDFT*delta_f;
N = 384*100000;

n = 0:1:NDFT-1;

f0 = fs/N;

max_freq = (delta_f/2)/f0;

numIter = 10000;

SNR = 1000;

SNR_linear = 10.^(SNR./10);

% Baseband FIR for 1.4MHz Rx,
fir_coef = fir1(512, (64*delta_f)/(NDFT*delta_f)).';
len_fir_half = (length(fir_coef)-1)/2;
%freqz(fir_coef,1,512)

error_simple_avg = zeros(1,length(SNR));
error_extended_avg = zeros(1,length(SNR));
error_proposed_avg = zeros(1,length(SNR));
for snrIdx = 1:1:length(SNR)
    
    for iter=1:1:numIter
        
        k0 = 1000; %randi(max_freq + 1) - 1;
        
        cfo_freq = f0*k0;
        
        freq_offset = exp(1i.*2.*pi.*k0.*n./N);
        
        %% ------------------------------------------------------------------------
        % Pass signal through channel.
        
        cfo_signal = (rx_pss.*(freq_offset(1:1:NDFT).'));
        
        rx_signal = cfo_signal; %my_awgn(cfo_signal, SNR(snrIdx), 'measured');
        
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
        % Extended PSS-based CFO estimation.
        % OBS.: Maximum CFO frequency estimation is 15/2 = 7.5 KHz.
        
        ycorr0 = sum(y(0*(NDFT/4)+1:1:1*(NDFT/4)));
        
        ycorr1 = sum(y(1*(NDFT/4)+1:1:2*(NDFT/4)));
        
        ycorr2 = sum(y(2*(NDFT/4)+1:1:3*(NDFT/4)));
        
        ycorr3 = sum(y(3*(NDFT/4)+1:1:4*(NDFT/4)));
        
        cfo_est0 = angle(conj(ycorr0).*ycorr1)/(1*(pi/2));
        
        cfo_est1 = angle(conj(ycorr0).*ycorr2)/(2*(pi/2));
        
        cfo_est2 = angle(conj(ycorr0).*ycorr3)/(3*(pi/2));
        
        cfo_est3 = angle(conj(ycorr1).*ycorr2)/(1*(pi/2));
        
        cfo_est4 = angle(conj(ycorr1).*ycorr3)/(2*(pi/2));
        
        cfo_est5 = angle(conj(ycorr2).*ycorr3)/(1*(pi/2));
        
        ext_cfo_est = (cfo_est0+cfo_est1+cfo_est2+cfo_est3+cfo_est4+cfo_est5)/6;
        
        ext_cfo_freq_est = ext_cfo_est*15000;
        
        error_extended = abs(cfo_freq - ext_cfo_freq_est).^2;
        
        error_extended_avg(snrIdx) = error_extended_avg(snrIdx) + error_extended;
        
        %% ------------------------------------------------------------------------
        % Proposed PSS-based CFO estimation.
        % OBS.: Maximum CFO frequency estimation is 15/2 = 7.5 KHz.
        
        ycorr0 = sum(y(0*(NDFT/6)+1:1:1*(NDFT/6)));
        
        ycorr1 = sum(y(1*(NDFT/6)+1:1:2*(NDFT/6)));
        
        ycorr2 = sum(y(2*(NDFT/6)+1:1:3*(NDFT/6)));
        
        ycorr3 = sum(y(3*(NDFT/6)+1:1:4*(NDFT/6)));
        
        ycorr4 = sum(y(4*(NDFT/6)+1:1:5*(NDFT/6)));
        
        ycorr5 = sum(y(5*(NDFT/6)+1:1:6*(NDFT/6)));        
        
        
        
       
        
        cfo_est0 = angle(conj(ycorr0).*ycorr1)/(1*(pi/3));
        
        cfo_est1 = angle(conj(ycorr0).*ycorr2)/(2*(pi/3));
        
        cfo_est2 = angle(conj(ycorr0).*ycorr3)/(3*(pi/3));
        
        cfo_est3 = angle(conj(ycorr0).*ycorr4)/(4*(pi/3));
        
        cfo_est4 = angle(conj(ycorr0).*ycorr5)/(5*(pi/3));
        

        
        cfo_est5 = angle(conj(ycorr1).*ycorr2)/(1*(pi/3));
      
        cfo_est6 = angle(conj(ycorr1).*ycorr3)/(2*(pi/3));
        
        cfo_est7 = angle(conj(ycorr1).*ycorr4)/(3*(pi/3));
        
        cfo_est8 = angle(conj(ycorr1).*ycorr5)/(4*(pi/3));
        
        
        
        cfo_est9 = angle(conj(ycorr2).*ycorr3)/(1*(pi/3));
      
        cfo_est10 = angle(conj(ycorr2).*ycorr4)/(2*(pi/3));
        
        cfo_est11 = angle(conj(ycorr2).*ycorr5)/(3*(pi/3));        
        
        

        cfo_est12 = angle(conj(ycorr3).*ycorr4)/(1*(pi/3));
        
        cfo_est13 = angle(conj(ycorr3).*ycorr5)/(2*(pi/3)); 
        
        
        cfo_est14 = angle(conj(ycorr4).*ycorr5)/(1*(pi/3)); 
       
        
        
        
        prop_cfo_est = (cfo_est0 + cfo_est1 + cfo_est2 + cfo_est3 + cfo_est4 + cfo_est5 + cfo_est6 + cfo_est7 + cfo_est8 + cfo_est9 + cfo_est10 + cfo_est11 + cfo_est12 + cfo_est13 + cfo_est14)/15;
        
        prop_cfo_freq_est = prop_cfo_est*15000;
        
        error_proposed = abs(cfo_freq - prop_cfo_freq_est).^2;
        
        error_proposed_avg(snrIdx) = error_proposed_avg(snrIdx) + error_proposed;        
        
        
    end
    
    error_simple_avg(snrIdx) = error_simple_avg(snrIdx)/numIter;
    error_extended_avg(snrIdx) = error_extended_avg(snrIdx)/numIter;
    error_proposed_avg(snrIdx) = error_proposed_avg(snrIdx)/numIter;
    
    fprintf(1,'SNR: %1.2f\n', SNR(snrIdx));
    fprintf(1,'PSS-based CFO Estimate Error: %1.2e\n', error_simple_avg(snrIdx));
    fprintf(1,'Extended PSS-based CFO Estimate Error: %1.2e\n', error_extended_avg(snrIdx));
    fprintf(1,'Proposed PSS-based CFO Estimate Error: %1.2e\n', error_proposed_avg(snrIdx));
    fprintf(1,'-----------------------------------------------\n');
    
end

figure;
semilogy(SNR, error_simple_avg);
hold on;
semilogy(SNR, error_extended_avg);
semilogy(SNR, error_proposed_avg);
hold off;
grid on;
xlabel('MSE')
title('CFO estimation')
legend('Traditional','Extended', 'Proposed')
