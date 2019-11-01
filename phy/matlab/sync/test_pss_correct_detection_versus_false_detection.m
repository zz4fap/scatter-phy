clear all;close all;clc

plot_figures = 0;

add_noise = 1;

NUM_ITER = 1e6;

SNR = -30:2.5:0;

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

rng(12041988);
correct_detection_lte = zeros(1,length(SNR));
false_detection_lte = zeros(1,length(SNR));
for snr_idx = 1:1:length(SNR)
    
    for n=1:1:NUM_ITER
        
        num_int = randi(FRAME_SIZE-NDFT);
        
        rx_signal = [zeros(num_int,1); local_time_domain_pss; zeros(FRAME_SIZE-NDFT,1)];
        
        rx_signal_buffer = rx_signal(1:FRAME_SIZE);
        
        if(add_noise == 1)
            noisy_signal = awgn(rx_signal_buffer, SNR(snr_idx), 'measured');
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
        
        [value, peak_pos] = max(power_conv);
        
        if(abs(num_int+NDFT) == (peak_pos-1))
            correct_detection_lte(snr_idx) = correct_detection_lte(snr_idx) + 1;
        else
            false_detection_lte(snr_idx) = false_detection_lte(snr_idx) + 1;
        end
        
    end
    
end

correct_detection_lte = correct_detection_lte./NUM_ITER;
false_detection_lte = false_detection_lte./NUM_ITER;


%% --------------------------------------------------------------------------
% Generate PSS signal.
u = 17;
pss = customized_pss_zc(u);

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

correct_detection_customized = zeros(1,length(SNR));
false_detection_customized = zeros(1,length(SNR));
rng(12041988);
for snr_idx = 1:1:length(SNR)
    
    for n=1:1:NUM_ITER
        
        num_int = randi(FRAME_SIZE-NDFT);
        
        rx_signal = [zeros(num_int,1); local_time_domain_pss; zeros(FRAME_SIZE-NDFT,1)];
        
        rx_signal_buffer = rx_signal(1:FRAME_SIZE);
        
        if(add_noise == 1)
            noisy_signal = awgn(rx_signal_buffer, SNR(snr_idx), 'measured');
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
        
        [value, peak_pos] = max(power_conv);
        
        if(abs(num_int+NDFT) == (peak_pos-1))
            correct_detection_customized(snr_idx) = correct_detection_customized(snr_idx) + 1;
        else
            false_detection_customized(snr_idx) = false_detection_customized(snr_idx) + 1;
        end
        
    end
    
end

correct_detection_customized = correct_detection_customized./NUM_ITER;
false_detection_customized = false_detection_customized./NUM_ITER;

figure;
subplot(2,1,1);
yyaxis left
plot(SNR,correct_detection_lte);
ylabel('Correct Detection')
hold on;
yyaxis right
plot(SNR,false_detection_lte);
ylabel('False Detection')
grid on
xlabel('SNR [dB]')

subplot(2,1,2);
yyaxis left
plot(SNR,correct_detection_customized);
ylabel('Correct Detection')
hold on;
yyaxis right
plot(SNR,false_detection_customized);
ylabel('False Detection')
grid on
xlabel('SNR [dB]')
