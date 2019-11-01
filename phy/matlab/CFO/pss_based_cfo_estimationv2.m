clear all;close all;clc

cell_id = 0;
NDFT = 384; % For 5 MHz PHY BW.
SRSLTE_PSS_LEN = 62;
FRAME_SIZE = 5760; % For 5 MHz PHY BW.

delta_f = 15000;

% Generate PSS signal.
pss = lte_pss_zc(cell_id);

% Create OFDM symbol number 6, the one which carries PSS.
pss_zero_pad = [0;pss((SRSLTE_PSS_LEN/2)+1:end);zeros(NDFT-(SRSLTE_PSS_LEN+1),1);pss(1:SRSLTE_PSS_LEN/2)];

%plot(0:1:length(pss_zero_pad)-1, abs(pss_zero_pad).^2)

%local_time_domain_pss = (sqrt(NDFT)/sqrt(SRSLTE_PSS_LEN))*ifft(pss_zero_pad, NDFT);
local_time_domain_pss = (sqrt(SRSLTE_PSS_LEN))*(sqrt(NDFT)/sqrt(SRSLTE_PSS_LEN))*ifft(pss_zero_pad, NDFT);

%% ------------------------------------------------------------------------
% Generate frequency offset.
fs = NDFT*delta_f;
N = 384*100000;
n = 0:1:N-1;

f0 = fs/N;

numIter = 1000;

error_simple_avg = 0;
error_extended_avg = 0;
for iter=1:1:numIter
    
    k0 = randi(100000+1) -1;
    
    cfo_freq = f0*k0;
    
    freq_offset = exp(1i.*2.*pi.*k0.*n./N);
    
    %figure; plot(real(freq_offset))
    %figure; plot(real(pss),imag(pss),'*')
    
    %% ------------------------------------------------------------------------
    % Pass signal through channel.
    
    rx_signal = local_time_domain_pss.*(freq_offset(1:1:NDFT).');
    
    %RX_signal = (1/sqrt(NDFT))*fft(rx_signal, NDFT);
    %figure; plot(real(RX_signal),imag(RX_signal),'*')
    
    %% ------------------------------------------------------------------------
    % Execute dot-product.
    
    y = rx_signal.*conj(local_time_domain_pss);
    
    %% ------------------------------------------------------------------------
    % PSS-based CFO estimation.
    
    ycorr0 = sum(y(1:1:NDFT/2));
    
    ycorr1 = sum(y((NDFT/2)+1:1:NDFT));
    
    cfo_est = angle(conj(ycorr0).*ycorr1)/pi;
    
    cfo_freq_est = cfo_est*15000;
    
    error_simple = abs(cfo_freq - cfo_freq_est).^2;
    
    error_simple_avg = error_simple_avg + error_simple;
    
    fprintf(1,'CFO: %1.2f [Hz] - PSS-based CFO estimate: %1.2f [Hz] - Error: %1.2f\n', f0*k0, cfo_est*15000, error_simple);
    
    %rx_xcorr = xcorr(rx_signal,local_time_domain_pss);
    %figure; plot(abs(rx_xcorr).^2)
    
    %% ------------------------------------------------------------------------
    % Extended PSS-based CFO estimation.
    
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
    
    % fprintf(1,'CFO: %1.2f [Hz] - Extended PSS-based CFO estimate: %1.2f [Hz]\n', f0*k0, cfo_est0*15000);
    % fprintf(1,'CFO: %1.2f [Hz] - Extended PSS-based CFO estimate: %1.2f [Hz]\n', f0*k0, cfo_est1*15000);
    % fprintf(1,'CFO: %1.2f [Hz] - Extended PSS-based CFO estimate: %1.2f [Hz]\n', f0*k0, cfo_est2*15000);
    % fprintf(1,'CFO: %1.2f [Hz] - Extended PSS-based CFO estimate: %1.2f [Hz]\n', f0*k0, cfo_est3*15000);
    % fprintf(1,'CFO: %1.2f [Hz] - Extended PSS-based CFO estimate: %1.2f [Hz]\n', f0*k0, cfo_est4*15000);
    % fprintf(1,'CFO: %1.2f [Hz] - Extended PSS-based CFO estimate: %1.2f [Hz]\n', f0*k0, cfo_est5*15000);
    
    ext_cfo_est = (cfo_est0+cfo_est1+cfo_est2+cfo_est3+cfo_est4+cfo_est5)/6;
    
    ext_cfo_freq_est = ext_cfo_est*15000;
    
    error_extended = abs(cfo_freq - ext_cfo_freq_est).^2;
    
    error_extended_avg = error_extended_avg + error_extended;
    
    fprintf(1,'CFO: %1.2f [Hz] - Extended PSS-based CFO estimate: %1.2f [Hz] - Error: %1.2f\n', f0*k0, ext_cfo_est*15000, error_extended);
    
    
end

%rx_xcorr = xcorr(rx_signal,local_time_domain_pss);
%figure; plot(abs(rx_xcorr).^2)