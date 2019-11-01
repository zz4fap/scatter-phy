clear all;close all;clc

plot_figures = 0;

add_noise = 0;

NUM_ITER = 1000;

SNR = 20;

cell_id = 0;
NDFT = 384; % For 5 MHz PHY BW.
SRSLTE_PSS_LEN = 62;
FRAME_SIZE = 5760; % For 5 MHz PHY BW.
NDFT2 = FRAME_SIZE + NDFT;

% Generate PSS signal.
pss = lte_pss_zc(cell_id);

% Create OFDM symbol number 6, the one which carries PSS.
padding = (NDFT-SRSLTE_PSS_LEN)/2;
pss_zero_pad = [0;pss((SRSLTE_PSS_LEN/2)+1:end);zeros(NDFT-(SRSLTE_PSS_LEN+1),1);pss(1:SRSLTE_PSS_LEN/2)];
local_time_domain_pss = (sqrt(NDFT)/sqrt(SRSLTE_PSS_LEN))*ifft(pss_zero_pad, NDFT);

if(plot_figures == 1)
    ofdm_symbol = (1/sqrt(NDFT))*fftshift(fft(local_time_domain_pss,NDFT));
    figure;
    plot(0:1:NDFT-1,10*log10(abs(ofdm_symbol)),'b-')
end

% Conjugated local version of PSS in time domain.
local_conj_time_domain_pss = conj(local_time_domain_pss);

local_conj_time_domain_pss = [local_conj_time_domain_pss; zeros(FRAME_SIZE,1)];

for n=1:1:NUM_ITER
    
    num_int = 0; %randi(FRAME_SIZE); %5477
    
    rx_signal = [zeros(num_int,1); local_time_domain_pss; zeros(FRAME_SIZE-NDFT,1)];
    
    rx_signal_buffer = rx_signal(1:FRAME_SIZE);
    
    if(add_noise == 1)
        noisy_signal = awgn(rx_signal_buffer,SNR,'measured');
    else
        noisy_signal = rx_signal_buffer;
    end
    
    % Detection of PSS.
    input_signal = [noisy_signal; zeros(NDFT2-FRAME_SIZE,1)];
    
    received_signal_fft = fft(input_signal,NDFT2);
    
    local_conj_pss_fft = fft(local_conj_time_domain_pss,NDFT2);
    
    prod = received_signal_fft.*local_conj_pss_fft;
    
    convolution = ifft(prod,NDFT2);
    
    power_conv = abs(convolution).^2;
    
    [value peak_pos] = max(power_conv);
    
    %fprintf(1,'%d\n',abs(num_int-peak_pos));
    fprintf(1,'%d\t%d\t%d\t%d\n',abs(num_int+NDFT), peak_pos-(num_int+NDFT), peak_pos, num_int);
    
    %plot(0:1:length(power_conv)-1,(power_conv));
    
end





