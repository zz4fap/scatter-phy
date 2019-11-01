clear all;close all;clc;

rng(12041988);

phy_bw_idx = 3;

plot_figures = 0;

add_noise = 1;

NUM_ITER = 1e3;

SNR = 0; %-30:2.5:0;

noise_variance = 0;

NOF_SLOTS = 2;
NOF_SYMBS_PER_SLOT = 7;
SUBFRAME_LENGTH = [1920 3840 5760 11520 15360 23040];
numFFT = [128 256 384 768 1024 1536];       % Number of FFT points
deltaF = 15000;                             % Subcarrier spacing
numRBs = [6 15 25 50 75 100];               % Number of resource blocks
rbSize = 12;                                % Number of subcarriers per resource block
cpLen_1st_symb  = [10 20 30 60 80 120];     % Cyclic prefix length in samples only for very 1st symbol.
cpLen_other_symbs = [9 18 27 54 72 108];

cell_id = 0;
NDFT = numFFT(phy_bw_idx); % For 5 MHz PHY BW.
delta_f = 15e3; % subcarrier spacing in Hz.
USEFUL_SUBCARRIERS = 300;
PSS_GUARD_BAND = 10; % in number of subcarriers.
SRSLTE_PSS_LEN = 62;
FRAME_SIZE = SUBFRAME_LENGTH(phy_bw_idx); % For 5 MHz PHY BW.
NDFT2 = FRAME_SIZE + NDFT;

%% --------------------------------------------------------------------------
% Generate PSS signal.
pss = lte_pss_zc(cell_id);

% Create OFDM symbol number 6, the one which carries PSS.
pss_zero_pad = [0;pss((SRSLTE_PSS_LEN/2)+1:end);zeros(NDFT-(SRSLTE_PSS_LEN+1),1);pss(1:SRSLTE_PSS_LEN/2)];
local_time_domain_pss = (sqrt(NDFT)/sqrt(SRSLTE_PSS_LEN))*ifft(pss_zero_pad, NDFT);

%if(1)
if(plot_figures == 1)
    ofdm_symbol = (1/sqrt(NDFT))*ifftshift(fft(local_time_domain_pss,NDFT));
    figure;
    plot(0:1:NDFT-1,10*log10(abs(ofdm_symbol)),'b-')
end

% Conjugated local version of PSS in time domain.
local_conj_time_domain_pss = conj(local_time_domain_pss);

local_conj_time_domain_pss = [local_conj_time_domain_pss; zeros(NDFT2-length(local_conj_time_domain_pss),1)];

correct_detection_with_data = zeros(1,length(SNR));
miss_detection_with_data = zeros(1,length(SNR));
false_detection_with_data = zeros(1,length(SNR));
correct_miss_detection_with_data = zeros(1,length(SNR));
for snr_idx = 1:1:length(SNR)
    
    convolution_vector = [];
    for n = 1:1:NUM_ITER
        
        send_data = randi([0 1], 1);
        
        if(send_data == 1)
            subframe = [];
            for symb_idx = 1:1:(NOF_SLOTS*NOF_SYMBS_PER_SLOT)
                if(symb_idx == 7)
                    % Generate random QPSK data.
                    data = randi([0 3], USEFUL_SUBCARRIERS-SRSLTE_PSS_LEN-PSS_GUARD_BAND, 1);
                    % Apply QPSK modulation.
                    data_symbols = qammod(data, 4, 'UnitAveragePower', true);
                    tx_pss_zero_pad = [0; pss((SRSLTE_PSS_LEN/2)+1:end); zeros(5,1); data_symbols(1:114,1); zeros(83,1); data_symbols(114+1:end,1); zeros(5,1); pss(1:SRSLTE_PSS_LEN/2)];
                    tx_time_domain_pss_plus_data = (sqrt(NDFT)/sqrt(SRSLTE_PSS_LEN))*ifft(tx_pss_zero_pad, NDFT);
                    tx_time_domain_pss_plus_data_plus_cp = [tx_time_domain_pss_plus_data(end-cpLen_other_symbs(phy_bw_idx)+1:end); tx_time_domain_pss_plus_data];
                    
                    subframe = [subframe; tx_time_domain_pss_plus_data_plus_cp];
                else
                    % Generate random QPSK data.
                    data = randi([0 3], USEFUL_SUBCARRIERS, 1);
                    % Apply QPSK modulation.
                    data_symbols = qammod(data, 4, 'UnitAveragePower', true);
                    
                    tx_data_zero_pad = [0; data_symbols(1:USEFUL_SUBCARRIERS/2,1); zeros(83,1); data_symbols(USEFUL_SUBCARRIERS/2+1:end,1)];
                    tx_time_domain_data = (sqrt(NDFT)/sqrt(SRSLTE_PSS_LEN))*ifft(tx_data_zero_pad, NDFT);
                    
                    if(symb_idx == 1 || symb_idx == 8)
                        tx_time_domain_data_plus_cp = [tx_time_domain_data(end-cpLen_1st_symb(phy_bw_idx)+1:end); tx_time_domain_data];
                    else
                        tx_time_domain_data_plus_cp = [tx_time_domain_data(end-cpLen_other_symbs(phy_bw_idx)+1:end); tx_time_domain_data];
                    end
                    
                    subframe = [subframe; tx_time_domain_data_plus_cp];
                end
            end
            
        else
            subframe = complex(zeros(SUBFRAME_LENGTH(phy_bw_idx),1),zeros(SUBFRAME_LENGTH(phy_bw_idx),1));
        end
        
        num_int = 0;%randi((FRAME_SIZE/2)-NDFT);
        
        rx_signal = [zeros(num_int,1); subframe];
        
        rx_signal_buffer = rx_signal(1:FRAME_SIZE);
        
        if(add_noise == 1)
            noisy_signal = rx_signal_buffer + (sqrt(noise_variance))*(1/sqrt(2))*complex(randn(FRAME_SIZE,1), randn(FRAME_SIZE,1));                
        else
            noisy_signal = rx_signal_buffer;
        end
        
        if(0)
            figure;
            plot(abs(fft(noisy_signal)).^2);
            figure;
            plot(abs(fft(local_conj_time_domain_pss)).^2);
        end
        
        % Detection of PSS.
        input_signal = [noisy_signal; zeros(NDFT2-FRAME_SIZE,1)];
        
        received_signal_fft = fft(input_signal, NDFT2);
        
        local_conj_pss_fft = fft(local_conj_time_domain_pss, NDFT2);
        
        prod = received_signal_fft.*local_conj_pss_fft;
        
        convolution = ifft(prod, NDFT2);
        
        %convolution_vector = [convolution_vector; convolution];
        
        power_conv = abs(convolution).^2;
        
        power_conv_avg = sum(power_conv)/length(power_conv);
        
        [peak_value, peak_pos] = max(power_conv);
        
        if(send_data == 1)
            
            test = peak_value/power_conv_avg;
            
            if(abs((FRAME_SIZE/2) + num_int) == (peak_pos-1))
                correct_detection_with_data(snr_idx) = correct_detection_with_data(snr_idx) + 1;
            else
                miss_detection_with_data(snr_idx) = miss_detection_with_data(snr_idx) + 1;
            end
        else
            if(abs((FRAME_SIZE/2) + num_int) == (peak_pos-1))
                false_detection_with_data(snr_idx) = false_detection_with_data(snr_idx) + 1;
            else
                correct_miss_detection_with_data(snr_idx) = correct_miss_detection_with_data(snr_idx) + 1;
            end
        end
        
    end
    
end

correct_detection_with_data = correct_detection_with_data./NUM_ITER;
miss_detection_with_data = miss_detection_with_data./NUM_ITER;

false_detection_with_data = false_detection_with_data./NUM_ITER;
correct_miss_detection_with_data = correct_miss_detection_with_data./NUM_ITER;

if(0)
    gaussian_control = (sqrt(var(convolution_vector)))*(1/sqrt(2))*complex(randn(1,length(convolution_vector)), randn(1,length(convolution_vector)));
    h1 = histogram(real(convolution_vector));
    hold on
    h2 = histogram(real(gaussian_control));
    h1.Normalization = 'probability';
    h1.BinWidth = 0.005;
    h2.Normalization = 'probability';
    h2.BinWidth = 0.005;
    legend('conv', 'control');
    hold off
end

figure;
subplot(2,1,1)
yyaxis left
plot(SNR,correct_detection_with_data);
ylabel('Correct Detection')
hold on;
yyaxis right
plot(SNR,miss_detection_with_data);
ylabel('False Detection')
grid on
xlabel('SNR [dB]')

subplot(2,1,2)
yyaxis left
plot(SNR,false_detection_with_data);
ylabel('False Detection')
hold on;
yyaxis right
plot(SNR,correct_miss_detection_with_data);
ylabel('Correct Miss')
grid on
xlabel('SNR [dB]')

