clear all;close all;clc

phy_bw_idx = 3;

plot_figures = 0;

add_noise = 1;

NUM_ITER = 1e4;

SNR = -20:2:6; %-30:2.5:0;

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

enable_ofdm_with_pss_plus_data1       = true;
enable_ofdm_with_pss_plus_data16      = true;
enable_ofdm_with_pss_plus_data17      = true;


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




if(enable_ofdm_with_pss_plus_data16)
    % Generate PSS signal.
    pss = lte_pss_zc(cell_id);
    
    % Create OFDM symbol number 6, the one which carries PSS.
    pss_zero_pad = [0;pss((SRSLTE_PSS_LEN/2)+1:end);zeros(NDFT-(SRSLTE_PSS_LEN+1),1);pss(1:SRSLTE_PSS_LEN/2)];
    local_time_domain_pss = (sqrt(NDFT)/sqrt(SRSLTE_PSS_LEN))*ifft(pss_zero_pad, NDFT);
    
    % Conjugated local version of PSS in time domain.
    local_conj_time_domain_pss = conj(local_time_domain_pss);
    
    local_conj_time_domain_pss = [local_conj_time_domain_pss; zeros(NDFT2-length(local_conj_time_domain_pss),1)];
    
    smaller_local_time_domain_pss = conj(local_time_domain_pss); 
    
    correct_detection_with_data16 = zeros(1,length(SNR));
    miss_detection_with_data16 = zeros(1,length(SNR));
    false_detection_with_data16 = zeros(1,length(SNR));
    false_detection_peak_pos16 = zeros(length(SNR),NUM_ITER);
    miss_detection_psr16 = zeros(length(SNR), NUM_ITER);
    miss_detection_psr_cnt16 = ones(length(SNR));
    psr_avg16 = zeros(length(SNR));
    psr_cnt16 = zeros(length(SNR));     
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
            
            %% ------------------------------------------------------------
            % 1st Stage PSS Detection.
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
            %% ------------------------------------------------------------
            
            
            
            %% ------------------------------------------------------------
            first_stage_threshold = 2.0;%2.5;
            second_stage_threshold = 3.5;
            offset = 0;
            if(psr_max >= first_stage_threshold && psr_max < second_stage_threshold && (peak_pos-offset-NDFT) > 0)
                
                peak_pos_1st_stage = peak_pos - offset;
                
                smaller_input_signal = input_signal(peak_pos_1st_stage-NDFT:peak_pos_1st_stage-1);
                
                smaller_received_signal_fft = (sqrt(SRSLTE_PSS_LEN)/sqrt(NDFT))*fft([smaller_input_signal; zeros(NDFT, 1)], 2*NDFT);
                
                smaller_local_conj_pss_fft = (sqrt(SRSLTE_PSS_LEN)/sqrt(NDFT))*fft([smaller_local_time_domain_pss; zeros(NDFT, 1)], 2*NDFT);
                
                smaller_prod = smaller_received_signal_fft.*smaller_local_conj_pss_fft;
                
                smaller_convolution = (ifft(smaller_prod, 2*NDFT));
                
                smaller_power_conv = abs(smaller_convolution).^2;
                
                [peak_value_2nd_stage, peak_pos_2nd_stage] = max(smaller_power_conv);
                
                if(peak_pos_2nd_stage == (NDFT + offset + 1) && peak_value_2nd_stage < 3.0)

                    pl_ub = peak_pos_2nd_stage + 1;
                    while(smaller_power_conv(pl_ub + 1) <= smaller_power_conv(pl_ub))
                        pl_ub = pl_ub + 1;
                    end

                    pl_lb = peak_pos_2nd_stage - 1;
                    while(smaller_power_conv(pl_lb - 1) <= smaller_power_conv(pl_lb))
                        pl_lb = pl_lb - 1;
                    end

                    sl_distance_right = length(smaller_power_conv)-1-pl_ub;
                    if(sl_distance_right < 0)
                        sl_distance_right = 0;
                    end
                    sl_distance_left = pl_lb;

                    [value_right, sl_right] = max(smaller_power_conv(pl_ub:end));
                    sl_right = sl_right + pl_ub - 1;
                    [value_left, sl_left] = max(smaller_power_conv(1:sl_distance_left));

                    [side_lobe_value, side_lobe_pos] = max([smaller_power_conv(sl_right), smaller_power_conv(sl_left)]);

                    psr_max_second_stage = peak_value_2nd_stage/side_lobe_value;
                
                else
                    
                    psr_max_second_stage = 0;
                    
                end
                
            else
                
                psr_max_second_stage = 0;
                
            end
            %% ------------------------------------------------------------

            if(psr_max >= first_stage_threshold && (psr_max_second_stage >= second_stage_threshold || psr_max >= second_stage_threshold))
                if(peak_pos-1 >= (SUBFRAME_LENGTH(phy_bw_idx)/2)-3 && peak_pos-1 <= (SUBFRAME_LENGTH(phy_bw_idx)/2)+3)                    
                    correct_detection_with_data16(snr_idx) = correct_detection_with_data16(snr_idx) + 1;
                else
                    false_detection_with_data16(snr_idx) = false_detection_with_data16(snr_idx) + 1;
                    
                    false_detection_peak_pos16(snr_idx, false_detection_peak_pos_cnt) = peak_pos-1;
                    false_detection_peak_pos_cnt = false_detection_peak_pos_cnt + 1;
                end
                
                psr_avg16(snr_idx) = psr_avg16(snr_idx) + psr_max;
                psr_cnt16(snr_idx) = psr_cnt16(snr_idx) + 1;                 
            else
                miss_detection_with_data16(snr_idx) = miss_detection_with_data16(snr_idx) + 1;

                miss_detection_psr16(snr_idx, miss_detection_psr_cnt16(snr_idx)) = psr_max;
                miss_detection_psr_cnt16(snr_idx) = miss_detection_psr_cnt16(snr_idx) + 1;
            end
            
        end
        
    end
    
    correct_detection_with_data16 = correct_detection_with_data16./NUM_ITER;
    miss_detection_with_data16 = miss_detection_with_data16./NUM_ITER;
    false_detection_with_data16 = false_detection_with_data16./NUM_ITER;
    
    psr_avg16(snr_idx) = psr_avg16(snr_idx)./psr_cnt16(snr_idx);    
end








if(enable_ofdm_with_pss_plus_data17)
    
    Pfa = 0.0001;
    Pfd = 0.001;
    
    M = 1;

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
    
    smaller_local_time_domain_pss = conj(local_time_domain_pss); 
    
    correct_detection_with_data17 = zeros(1,length(SNR));
    miss_detection_with_data17 = zeros(1,length(SNR));
    false_detection_with_data17 = zeros(1,length(SNR));
    false_detection_peak_pos17 = zeros(length(SNR),NUM_ITER);
    miss_detection_psr17 = zeros(length(SNR), NUM_ITER);
    miss_detection_psr_cnt17 = ones(length(SNR));
    psr_avg17 = zeros(length(SNR));
    psr_cnt17 = zeros(length(SNR));     
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
            
            %% ------------------------------------------------------------
            % 1st Stage PSS Detection.
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
            %% ------------------------------------------------------------
            
            
            
            %% ------------------------------------------------------------            

            first_stage_threshold = 2.0;%2.5;
            second_stage_threshold = 3.0;  
            if(psr_max >= first_stage_threshold && psr_max < second_stage_threshold && (peak_pos-NDFT) > 0)
                smaller_input_signal = input_signal(peak_pos-NDFT:peak_pos-1);                
                
                smaller_received_signal_fft = (sqrt(SRSLTE_PSS_LEN)/sqrt(NDFT))*fft(smaller_input_signal, NDFT);
                
                poss = [2:1:2+(SRSLTE_PSS_LEN/2)-1, NDFT-(SRSLTE_PSS_LEN/2)+1:1:NDFT];
                
                smaller_received_signal_fft_pss = smaller_received_signal_fft(poss);                
                
                poxx = [(SRSLTE_PSS_LEN/2)+1:SRSLTE_PSS_LEN  1:(SRSLTE_PSS_LEN/2)];
                
                smaller_received_signal_fft_pss = smaller_received_signal_fft_pss(poxx);
                
                
                
                smaller_local_conj_pss_fft = (sqrt(SRSLTE_PSS_LEN)/sqrt(NDFT))*fft(smaller_local_time_domain_pss, NDFT);
                
                smaller_local_conj_pss_fft_pss = smaller_local_conj_pss_fft(poss);
                
                smaller_local_conj_pss_fft_pss = smaller_local_conj_pss_fft_pss(poxx);
                
                
                
                smaller_prod = smaller_received_signal_fft_pss.*smaller_local_conj_pss_fft_pss;
                
                smaller_convolution = ifftshift(ifft(smaller_prod, SRSLTE_PSS_LEN));
                
                smaller_power_conv = abs(smaller_convolution).^2;
                
                
    
                
                smaller_power_conv_aux = smaller_power_conv;
                
                %stem(0:1:Nzc-1,pdp_detection)
                
                % This is the implemenatation of the algorithm proposed in [3]
                % Sort Segmented PDP vector in increasing order of energy.
                for i=1:1:length(smaller_power_conv)
                    for k=1:1:length(smaller_power_conv)-1
                        if(smaller_power_conv(k)>smaller_power_conv(k+1))
                            temp = smaller_power_conv(k+1);
                            smaller_power_conv(k+1) = smaller_power_conv(k);
                            smaller_power_conv(k) = temp;
                        end
                    end
                end
                
                % Algorithm used to discard samples. Indeed, it is used to find a
                % speration between noisy samples and signal+noise samples.
                for I = 10:1:SRSLTE_PSS_LEN-1
                    limiar = -log(Pfd)/I;
                    if(smaller_power_conv(I+1) > limiar*sum(smaller_power_conv(1:I)))
                        break;
                    end
                end
                
                % Output of the disposal module. It's used as a noise reference.
                Zref = sum(smaller_power_conv(1:I));
                
                % Calculate the scale factor.
                alpha = finv((1-Pfa),(2*M),(2*I))/(I/M);
                
                peak_value_2nd_stage = smaller_power_conv_aux((SRSLTE_PSS_LEN/2) + 1);
                
                ratio_2nd_stage = peak_value_2nd_stage/Zref;
                if(ratio_2nd_stage >= alpha && peak_value_2nd_stage < 2.0)
                    psr_max_second_stage = second_stage_threshold;
                else
                    psr_max_second_stage = 0;
                end
                
            else
                
                psr_max_second_stage = 0;
                
            end
            %% ------------------------------------------------------------
            
            if(psr_max >= first_stage_threshold && (psr_max_second_stage >= second_stage_threshold || psr_max >= second_stage_threshold))
                if(peak_pos-1 >= (SUBFRAME_LENGTH(phy_bw_idx)/2)-3 && peak_pos-1 <= (SUBFRAME_LENGTH(phy_bw_idx)/2)+3)                    
                    correct_detection_with_data17(snr_idx) = correct_detection_with_data17(snr_idx) + 1;
                else
                    false_detection_with_data17(snr_idx) = false_detection_with_data17(snr_idx) + 1;
                    
                    false_detection_peak_pos17(snr_idx, false_detection_peak_pos_cnt) = peak_pos-1;
                    false_detection_peak_pos_cnt = false_detection_peak_pos_cnt + 1;
                end
                
                psr_avg17(snr_idx) = psr_avg17(snr_idx) + psr_max;
                psr_cnt17(snr_idx) = psr_cnt17(snr_idx) + 1;                 
            else
                miss_detection_with_data17(snr_idx) = miss_detection_with_data17(snr_idx) + 1;

                miss_detection_psr17(snr_idx, miss_detection_psr_cnt17(snr_idx)) = psr_max;
                miss_detection_psr_cnt17(snr_idx) = miss_detection_psr_cnt17(snr_idx) + 1;
            end
            
        end
        
    end
    
    correct_detection_with_data17 = correct_detection_with_data17./NUM_ITER;
    miss_detection_with_data17 = miss_detection_with_data17./NUM_ITER;
    false_detection_with_data17 = false_detection_with_data17./NUM_ITER;
    
    psr_avg17(snr_idx) = psr_avg17(snr_idx)./psr_cnt17(snr_idx);    
end


fdee_figure1 = figure('rend','painters','pos',[10 10 1000 900]);

nof_plots = 3;%9;
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

if(enable_ofdm_with_pss_plus_data16)
    plot_cnt = plot_cnt + 1;
    subplot(nof_plots, 1, plot_cnt);
    plot(SNR, correct_detection_with_data16);
    hold on;
    plot(SNR, miss_detection_with_data16);
    plot(SNR, false_detection_with_data16);
    grid on;
    xlabel('SNR [dB]');
    legend('Correct detection', 'Miss detection', 'False detection');
    titleStr = sprintf('OFDM Symbol with PSS + Data - 2-stage (PSR)');
    title(titleStr);
    hold off;
end

if(enable_ofdm_with_pss_plus_data17)
    plot_cnt = plot_cnt + 1;
    subplot(nof_plots, 1, plot_cnt);
    plot(SNR, correct_detection_with_data17);
    hold on;
    plot(SNR, miss_detection_with_data17);
    plot(SNR, false_detection_with_data17);
    grid on;
    xlabel('SNR [dB]');
    legend('Correct detection', 'Miss detection', 'False detection');
    titleStr = sprintf('OFDM Symbol with PSS + Data - 2-stage (CA-CFAR)');
    title(titleStr);
    hold off;
end
