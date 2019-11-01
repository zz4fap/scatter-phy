clear all;close all;clc

phy_bw_idx = 3;

plot_figures = 0;

add_noise = 1;

NUM_ITER = 1e3;

SNR = -6:2:4; %-30:2.5:0;

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
SRSLTE_PSS_LEN = 62;
PSS_GUARD_BAND = 10; % in number of subcarriers.
FRAME_SIZE = SUBFRAME_LENGTH(phy_bw_idx); % For 5 MHz PHY BW.
NDFT2 = FRAME_SIZE + NDFT;

enable_ofdm_with_pss_plus_data1 = true;
enable_ofdm_with_pss_plus_data2 = true;

threshold = 3.0;

%% --------------------------------------------------------------------------
if(enable_ofdm_with_pss_plus_data1)
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
    
    correct_detection_with_data1 = zeros(1,length(SNR));
    miss_detection_with_data1 = zeros(1,length(SNR));
    false_detection_with_data1 = zeros(1,length(SNR));
    false_detection_peak_pos1 = zeros(length(SNR),NUM_ITER);
    miss_detection_psr1 = zeros(length(SNR), NUM_ITER);
    miss_detection_psr_cnt1 = ones(length(SNR));
    psr_avg1 = zeros(length(SNR));
    psr_cnt1 = zeros(length(SNR));     
    rng(12041988);
    for snr_idx = 1:1:length(SNR)
        
        false_detection_peak_pos_cnt = 1;
        for n = 1:1:NUM_ITER
            
            subframe = [];
            for symb_idx = 1:1:14
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
            
            num_int = 0;%randi((FRAME_SIZE/2)-NDFT);
            
            rx_signal = [zeros(num_int,1); subframe];
            
            rx_signal_buffer = rx_signal(1:FRAME_SIZE);
            
            if(add_noise == 1)
                noisy_signal = awgn(rx_signal_buffer, SNR(snr_idx), 'measured');
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
            
            power_conv = abs(convolution).^2;
            
            [peak_value, peak_pos] = max(power_conv);
            
            pl_ub = peak_pos + 1;
            while(power_conv(pl_ub + 1) <= power_conv(pl_ub))
                pl_ub = pl_ub + 1;
            end
            
            pl_lb = peak_pos - 1;
            while(power_conv(pl_lb - 1) <= power_conv(pl_lb))
                pl_lb = pl_lb - 1;
            end
            
            sl_distance_right = length(power_conv)-1-pl_ub;
            if(sl_distance_right < 0)
                sl_distance_right = 0;
            end
            sl_distance_left = pl_lb;
            
            [value_right, sl_right] = max(power_conv(pl_ub:end));
            sl_right = sl_right + pl_ub - 1;
            [value_left, sl_left] = max(power_conv(1:sl_distance_left));
            
            [side_lobe_value, side_lobe_pos] = max([power_conv(sl_right), power_conv(sl_left)]);
            
            psr_max = peak_value/side_lobe_value;
            
            if(psr_max >= threshold)
                if(peak_pos-1 >= (SUBFRAME_LENGTH(phy_bw_idx)/2)-3 && peak_pos-1 <= (SUBFRAME_LENGTH(phy_bw_idx)/2)+3)                    
                    correct_detection_with_data1(snr_idx) = correct_detection_with_data1(snr_idx) + 1;
                else
                    false_detection_with_data1(snr_idx) = false_detection_with_data1(snr_idx) + 1;
                    
                    false_detection_peak_pos1(snr_idx, false_detection_peak_pos_cnt) = peak_pos-1;
                    false_detection_peak_pos_cnt = false_detection_peak_pos_cnt + 1;
                end
                
                psr_avg1(snr_idx) = psr_avg1(snr_idx) + psr_max;
                psr_cnt1(snr_idx) = psr_cnt1(snr_idx) + 1;                 
            else
                miss_detection_with_data1(snr_idx) = miss_detection_with_data1(snr_idx) + 1;

                miss_detection_psr1(snr_idx, miss_detection_psr_cnt1(snr_idx)) = psr_max;
                miss_detection_psr_cnt1(snr_idx) = miss_detection_psr_cnt1(snr_idx) + 1;
            end
            
        end
        
    end
    
    correct_detection_with_data1 = correct_detection_with_data1./NUM_ITER;
    miss_detection_with_data1 = miss_detection_with_data1./NUM_ITER;
    false_detection_with_data1 = false_detection_with_data1./NUM_ITER;
    
    psr_avg1(snr_idx) = psr_avg1(snr_idx)./psr_cnt1(snr_idx);    
end

%% --------------------------------------------------------------------------
if(enable_ofdm_with_pss_plus_data2)
    
    HALF_FRAME_SIZE = FRAME_SIZE/2;
    
    FRAME_SIZE_PLUS_HALF = FRAME_SIZE + HALF_FRAME_SIZE;
    
    NDFT3 = FRAME_SIZE + HALF_FRAME_SIZE + NDFT;
    
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
    
    local_conj_time_domain_pss = [local_conj_time_domain_pss; zeros(NDFT3-length(local_conj_time_domain_pss),1)];
    
    local_conj_pss_fft = fft(local_conj_time_domain_pss, NDFT3);
    
    correct_detection_with_data2 = zeros(1,length(SNR));
    miss_detection_with_data2 = zeros(1,length(SNR));
    false_detection_with_data2 = zeros(1,length(SNR));
    false_detection_peak_pos2 = zeros(length(SNR),NUM_ITER);
    miss_detection_psr2 = zeros(length(SNR), NUM_ITER);
    miss_detection_psr_cnt2 = ones(length(SNR));
    psr_avg2 = zeros(length(SNR));
    psr_cnt2 = zeros(length(SNR));     
    rng(12041988);
    for snr_idx = 1:1:length(SNR)
        
        false_detection_peak_pos_cnt = 1;
        for n = 1:1:NUM_ITER
            
            subframe = [];
            for symb_idx = 1:1:14
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
            
            num_int = 0;%randi((FRAME_SIZE/2)-NDFT);
            
            rx_signal = [zeros(num_int,1); subframe; subframe(HALF_FRAME_SIZE+1:end)];
            
            rx_signal_buffer = rx_signal(1:FRAME_SIZE_PLUS_HALF);
            
            if(add_noise == 1)
                noisy_signal = awgn(rx_signal_buffer, SNR(snr_idx), 'measured');
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
            input_signal = [noisy_signal; zeros(NDFT3-FRAME_SIZE_PLUS_HALF,1)];
            
            received_signal_fft = fft(input_signal, NDFT3);
            
            prod = received_signal_fft.*local_conj_pss_fft;
            
            convolution = ifft(prod, NDFT3);
            
            power_conv = abs(convolution).^2;
            
            [peak_value, peak_pos] = max(power_conv);
            
            pl_ub = peak_pos + 1;
            while(power_conv(pl_ub + 1) <= power_conv(pl_ub))
                pl_ub = pl_ub + 1;
            end
            
            pl_lb = peak_pos - 1;
            while(power_conv(pl_lb - 1) <= power_conv(pl_lb))
                pl_lb = pl_lb - 1;
            end
            
            sl_distance_right = length(power_conv)-1-pl_ub;
            if(sl_distance_right < 0)
                sl_distance_right = 0;
            end
            sl_distance_left = pl_lb;
            
            [value_right, sl_right] = max(power_conv(pl_ub:end));
            sl_right = sl_right + pl_ub - 1;
            [value_left, sl_left] = max(power_conv(1:sl_distance_left));
            
            [side_lobe_value, side_lobe_pos] = max([power_conv(sl_right), power_conv(sl_left)]);
            
            psr_max = peak_value/side_lobe_value;
            
            if(psr_max >= threshold)
                if(peak_pos-1 >= (SUBFRAME_LENGTH(phy_bw_idx)/2)-3 && peak_pos-1 <= (SUBFRAME_LENGTH(phy_bw_idx)/2)+3)                    
                    correct_detection_with_data2(snr_idx) = correct_detection_with_data2(snr_idx) + 1;
                else
                    false_detection_with_data2(snr_idx) = false_detection_with_data2(snr_idx) + 1;
                    
                    false_detection_peak_pos2(snr_idx, false_detection_peak_pos_cnt) = peak_pos-1;
                    false_detection_peak_pos_cnt = false_detection_peak_pos_cnt + 1;
                end
                
                psr_avg2(snr_idx) = psr_avg2(snr_idx) + psr_max;
                psr_cnt2(snr_idx) = psr_cnt2(snr_idx) + 1;                 
            else
                miss_detection_with_data2(snr_idx) = miss_detection_with_data2(snr_idx) + 1;

                miss_detection_psr2(snr_idx, miss_detection_psr_cnt2(snr_idx)) = psr_max;
                miss_detection_psr_cnt2(snr_idx) = miss_detection_psr_cnt2(snr_idx) + 1;
            end
            
        end
        
    end
    
    correct_detection_with_data2 = correct_detection_with_data2./NUM_ITER;
    miss_detection_with_data2 = miss_detection_with_data2./NUM_ITER;
    false_detection_with_data2 = false_detection_with_data2./NUM_ITER;
    
    psr_avg2(snr_idx) = psr_avg2(snr_idx)./psr_cnt2(snr_idx);    
end


if(0)
    % Get timestamp for saving files.
    timeStamp = datestr(now,30);
    fileName = sprintf('test_pss_correct_detection_versus_false_detectionv1_%s.mat',timeStamp);
    save(fileName);
end

fdee_figure1 = figure('rend','painters','pos',[10 10 1000 900]);

nof_plots = 2;%9;
plot_cnt = 0;
if(enable_ofdm_with_pss_plus_data1)
    plot_cnt = plot_cnt + 1;
    subplot(nof_plots, 1, plot_cnt);
    plot(SNR, correct_detection_with_data1);
    hold on;
    plot(SNR, miss_detection_with_data1);
    plot(SNR, false_detection_with_data1);
    grid on;
    xlabel('SNR [dB]');
    legend('Correct detection', 'Miss detection', 'False detection');
    title('OFDM Symbol with PSS + Data - Legacy PSR-based Threshold');
    hold off;
end

if(enable_ofdm_with_pss_plus_data2)
    plot_cnt = plot_cnt + 1;
    subplot(nof_plots, 1, plot_cnt);
    plot(SNR, correct_detection_with_data2);
    hold on;
    plot(SNR, miss_detection_with_data2);
    plot(SNR, false_detection_with_data2);
    grid on;
    xlabel('SNR [dB]');
    legend('Correct detection', 'Miss detection', 'False detection');
    title('OFDM Symbol with PSS + Data - Longer frame - PSR-based Threshold');
    hold off;
end

for snr_idx = 1:1:length(SNR) 
    fprintf(1, 'SNR: %1.2f - 1: %1.4f - 2: %1.4f\n', SNR(snr_idx), psr_avg1(snr_idx), psr_avg2(snr_idx));
end