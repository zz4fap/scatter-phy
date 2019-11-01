clear all;close all;clc

NUM_ITER = 1000;

snr = 20;

cell_id = 0;
NDFT = 384;
SRSLTE_PSS_LEN = 62;
FRAME_SIZE = 5760;
NDFT2 = FRAME_SIZE + NDFT;

% Generate PSS signal.
pss = lte_pss_zc(cell_id);

padding = (NDFT-SRSLTE_PSS_LEN)/2;
pss_zero_pad = [zeros(padding,1); pss; zeros(padding,1)];
time_domain_pss = ifft(pss_zero_pad, NDFT);

%time_domain_pss = conj(time_domain_pss).*(1.0/SRSLTE_PSS_LEN);
sync_time_domain_pss = conj(time_domain_pss);

sync_time_domain_pss = [sync_time_domain_pss; zeros(FRAME_SIZE,1)];


for n=1:1:NUM_ITER
    
    num_int = randi(FRAME_SIZE-NDFT);
    
    % This is a preamble.
    rx_signal = [zeros(num_int,1); time_domain_pss; zeros(FRAME_SIZE-num_int-NDFT,1)];
    
    noisy_signal = rx_signal; %awgn(rx_signal,snr,'measured');
    
    % Detection of PSS.
    input_signal = [noisy_signal; zeros(NDFT2-FRAME_SIZE,1)];
    
    input_signal_fft = fft(input_signal,NDFT2);
    
    sync_pss_fft = fft(sync_time_domain_pss,NDFT2);
    
    prod = input_signal_fft.*sync_pss_fft;
    
    convolution = ifft(prod,NDFT2);
    
    power_conv = abs(convolution).^2;
    
    [value peak_pos] = max(power_conv);
    
    %fprintf(1,'%d\n',abs(num_int-peak_pos));
    fprintf(1,'%d - %d - %d\n',abs(num_int+NDFT), peak_pos-(num_int+NDFT), peak_pos );
end
    
plot(0:1:length(power_conv)-1,power_conv);
    
    a = 1;



