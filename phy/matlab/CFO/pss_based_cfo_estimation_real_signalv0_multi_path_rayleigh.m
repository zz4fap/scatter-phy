clear all;close all;clc

rng(1)

cell_id = 0;
NDFT = 384; % For 5 MHz PHY BW.
SRSLTE_PSS_LEN = 62;
FRAME_SIZE = 5760; % For 5 MHz PHY BW.
delta_f = 15000;

% Load signal.
filename = '/home/zz4fap/work/dcf_tdma/scatter/build/transmitted_subframe_1_cnt_0.dat';
%filename = '/home/zz4fap/work/dcf_tdma/scatter/build/received_subframe_1_cnt_0.dat';

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

% Generate PSS signal.
local_pss = lte_pss_zc(cell_id);

% Create OFDM symbol number 6, the one which carries PSS.
pss_zero_pad = [0;local_pss((SRSLTE_PSS_LEN/2)+1:end);zeros(NDFT-(SRSLTE_PSS_LEN+1),1);local_pss(1:SRSLTE_PSS_LEN/2)];

local_time_domain_pss = (ifft(pss_zero_pad, NDFT)*(1/SRSLTE_PSS_LEN));

local_time_domain_pss_power = sum(abs(local_time_domain_pss(:)).^2)/length(local_time_domain_pss(:));

%% ------------------------------------------------------------------------
% Generate frequency offset.
fs = NDFT*delta_f;
N = 384*100000;

n = 0:1:FRAME_SIZE-1;

f0 = fs/N;

max_freq = 6000/f0;  %(delta_f/2)/f0; 

numIter = 100000;

SNR = 0:5:30;

SNR_linear = 10.^(SNR./10);

% Baseband FIR for 1.4MHz Rx,
fir_coef = fir1(512, (66*delta_f)/(NDFT*delta_f)).';
len_fir_half = (length(fir_coef)-1)/2;
%freqz(fir_coef,1,512)

RX_PSS_filt = zeros(1,NDFT);

%% ------------------------------------------------------------------------
% Construct a frequency-flat (“single path”) Rayleigh fading channel object.

ts = 1/(NDFT*delta_f);
c = 3e8; % Light speed in m/s
fc = 1e9; % Center frequency in Hz.
lambda = c/fc; % Wavelenght in meters.
speed = 1; % in m/s
fd = (speed/lambda)*cos(0); % Maximum Doppler shift in Hz.

channel = rayleighchan(ts, fd);

% Parameter for the Extended Pedestrian A model (EPA)
channel.PathDelays = [0 30e-9 70e-9 90e-9 110e-9 190e-9 410e-9];
channel.AvgPathGaindB = [0 -1.0 -2.0 -3.0 -8.0 -17.2 -20.8];

%% ------------------------------------------------------------------------
% Monte Carlo simulation loop.

error_simple_avg = zeros(1,length(SNR));
error_filtered_avg = zeros(1,length(SNR));
error_coherent_avg = zeros(1,length(SNR));
error_2cp_avg = zeros(1,length(SNR));
error_7cp_avg = zeros(1,length(SNR));
error_14cp_avg = zeros(1,length(SNR));
error_extended_avg = zeros(1,length(SNR));
for snrIdx = 1:1:length(SNR)
    
    for iter=1:1:numIter
        
        k0 = randi(max_freq + 1) - 1;
        
        signal = randi(2) - 1;
        if(signal == 0)
           k0 = -k0;
        end
        
        cfo_freq = f0*k0;
        
        freq_offset = exp(1i.*2.*pi.*k0.*n./N);
        
        %% ------------------------------------------------------------------------
        % Pass signal through channel: Rayleigh + Noise + CFO.
        
        rayleigh_signal = filter(channel, ofdm_subframe);
       
        noise_signal = my_awgn(rayleigh_signal, SNR(snrIdx), 'measured'); %cfo_signal
        
        cfo_signal = noise_signal.*freq_offset(1:1:FRAME_SIZE);
        
        rx_signal = cfo_signal;
        
        %% ------------------------------------------------------------------------
        % Retrive received PSS.
        rx_pss = rx_signal((FRAME_SIZE/2)-NDFT+1:FRAME_SIZE/2).';
        
        %% ------------------------------------------------------------------------
        % Execute dot-product.        
        
        y = rx_pss.*conj(local_time_domain_pss);
        
        %% ------------------------------------------------------------------------
        % PSS-based CFO estimation.
        % OBS.: Maximum CFO frequency estimation is 15 KHz.
        
        ycorr0 = sum(y(1:1:NDFT/2));
        
        ycorr1 = sum(y((NDFT/2)+1:1:NDFT));
        
        cfo_est = angle(conj(ycorr0).*ycorr1)/pi;
        
        cfo_freq_est = cfo_est*15000;
        
        error_simple = abs((cfo_freq - cfo_freq_est)).^2;
        
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
        
        error_extended = (abs(cfo_freq - ext_cfo_freq_est).^2);
        
        error_extended_avg(snrIdx) = error_extended_avg(snrIdx) + error_extended;        
        
        %% ------------------------------------------------------------------------
        % Filtered-PSS-based CFO estimation.    
        
        filt_rx_signal = conv(rx_pss, fir_coef);
        filtered_rx_signal = filt_rx_signal((len_fir_half+1):(end-len_fir_half));
        
%         RX_PSS = zeros(NDFT,1);
%         OFDM_SYMBOL = fft(rx_pss,NDFT);
%         RX_PSS([2:32 354:384]) = OFDM_SYMBOL([2:32 354:384]);
%         filtered_rx_signal = ifft(RX_PSS,NDFT);
        
        
        %figure;plot(real(filtered_rx_signal))
        %figure;plot(abs(fft(filtered_rx_signal)))
        
        y = filtered_rx_signal.*conj(local_time_domain_pss);
        
        filt_ycorr0 = sum(y(1:1:NDFT/2));
        
        filt_ycorr1 = sum(y((NDFT/2)+1:1:NDFT));
        
        cfo_est_filt = angle(conj(filt_ycorr0).*filt_ycorr1)/pi;
        
        filt_cfo_freq_est = cfo_est_filt*15000;
        
        error_filt = abs((cfo_freq - filt_cfo_freq_est)).^2;
        
        error_filtered_avg(snrIdx) = error_filtered_avg(snrIdx) + error_filt;      
        
        %% ------------------------------------------------------------------------
        % CP-based CFO estimation: 2
        
        idxStart = 0;
        numSymb = 2;
        cp_corr = 0;
        for symIdx=0:1:numSymb-1
        
            if(mod(symIdx,7)==0)
                cp_len = 30;
            else
                cp_len = 27;
            end
                       
            corr_local = 0;
            for sampleIdx=0:1:cp_len-1
                corr_local = corr_local + rx_signal(idxStart+sampleIdx+1).*conj(rx_signal(idxStart+NDFT+sampleIdx+1));
            end
            cp_corr = cp_corr + corr_local;
            
            idxStart = idxStart + (NDFT+cp_len);
    
        end
        
        cp_corr = cp_corr/numSymb;      
        
        cfo_cp_est = -angle(cp_corr) / pi / 2;
       
        cfo_cp_freq_est = cfo_cp_est*15000;
        
        error_cp = abs((cfo_freq - cfo_cp_freq_est)).^2;
        
        error_2cp_avg(snrIdx) = error_2cp_avg(snrIdx) + error_cp;
                
        %% ------------------------------------------------------------------------
        % CP-based CFO estimation: 7
        
        idxStart = 0;
        numSymb = 7;
        cp_corr = 0;
        for symIdx=0:1:numSymb-1
        
            if(mod(symIdx,7)==0)
                cp_len = 30;
            else
                cp_len = 27;
            end
                       
            corr_local = 0;
            for sampleIdx=0:1:cp_len-1
                corr_local = corr_local + rx_signal(idxStart+sampleIdx+1).*conj(rx_signal(idxStart+NDFT+sampleIdx+1));
            end
            cp_corr = cp_corr + corr_local;
            
            idxStart = idxStart + (NDFT+cp_len);
    
        end
        
        cp_corr = cp_corr/numSymb;      
        
        cfo_cp_est = -angle(cp_corr) / pi / 2;
       
        cfo_cp_freq_est = cfo_cp_est*15000;
        
        error_cp = abs((cfo_freq - cfo_cp_freq_est)).^2;
        
        error_7cp_avg(snrIdx) = error_7cp_avg(snrIdx) + error_cp;
        
        
        %% ------------------------------------------------------------------------
        % CP-based CFO estimation: 14
        
        idxStart = 0;
        numSymb = 14;
        cp_corr = 0;
        for symIdx=0:1:numSymb-1
        
            if(mod(symIdx,7)==0)
                cp_len = 30;
            else
                cp_len = 27;
            end
                       
            corr_local = 0;
            for sampleIdx=0:1:cp_len-1
                corr_local = corr_local + rx_signal(idxStart+sampleIdx+1).*conj(rx_signal(idxStart+NDFT+sampleIdx+1));
            end
            cp_corr = cp_corr + corr_local;
            
            idxStart = idxStart + (NDFT+cp_len);
    
        end
        
        cp_corr = cp_corr/numSymb;      
        
        cfo_cp_est = -angle(cp_corr) / pi / 2;
       
        cfo_cp_freq_est = cfo_cp_est*15000;
        
        error_cp = abs((cfo_freq - cfo_cp_freq_est)).^2;
        
        error_14cp_avg(snrIdx) = error_14cp_avg(snrIdx) + error_cp;
        
    end
    
    error_simple_avg(snrIdx) = error_simple_avg(snrIdx)/numIter;
    error_filtered_avg(snrIdx) = error_filtered_avg(snrIdx)/numIter;
    error_extended_avg(snrIdx) = error_extended_avg(snrIdx)/numIter;
    error_2cp_avg(snrIdx) = error_2cp_avg(snrIdx)/numIter;
    error_7cp_avg(snrIdx) = error_7cp_avg(snrIdx)/numIter;
    error_14cp_avg(snrIdx) = error_14cp_avg(snrIdx)/numIter;
    
    fprintf(1,'SNR: %1.2f\n', SNR(snrIdx));
    fprintf(1,'PSS-based CFO Estimate Error: %1.2e\n', error_simple_avg(snrIdx));
    fprintf(1,'Filtered-PSS-based CFO Estimate Error: %1.2e\n', error_filtered_avg(snrIdx));  
    fprintf(1,'Extended PSS-based CFO Estimate Error: %1.2e\n', error_extended_avg(snrIdx));  
    fprintf(1,'2 CP-based CFO Estimate Error: %1.2e\n', error_2cp_avg(snrIdx));
    fprintf(1,'7 CP-based CFO Estimate Error: %1.2e\n', error_7cp_avg(snrIdx));
    fprintf(1,'14 CP-based CFO Estimate Error: %1.2e\n', error_14cp_avg(snrIdx));
    fprintf(1,'-----------------------------------------------\n');
    
end

figure;
semilogy(SNR, error_simple_avg);
hold on;
semilogy(SNR, error_filtered_avg);
semilogy(SNR, error_extended_avg);
semilogy(SNR, error_2cp_avg);
semilogy(SNR, error_7cp_avg);
semilogy(SNR, error_14cp_avg);
hold off;
grid on;
xlabel('MSE')
title('CFO estimation in multi-path Rayleigh fading channel')
legend('PSS-based','Filtered PSS-based','Extended PSS-based','CP-based: 2 OFDM symbols','CP-based: 7 OFDM symbols','CP-based: 14 OFDM symbols')
