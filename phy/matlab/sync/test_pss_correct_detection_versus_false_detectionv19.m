clear all;close all;clc

phy_bw_idx = 3;

plot_figures = 0;

add_noise = 1;

NUM_ITER = 1e3;

SNR = -12:2:6; %-30:2.5:0;

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
enable_ofdm_with_pss_plus_data2       = false;
enable_ofdm_with_pss_plus_data3       = false;
enable_ofdm_with_pss_plus_data4       = false;
enable_ofdm_with_pss_plus_data5       = false;
enable_ofdm_with_pss_plus_data6       = false;
enable_ofdm_with_pss_plus_data7       = false;
enable_ofdm_with_pss_plus_data8       = false;
enable_ofdm_with_pss_plus_data9       = false;
enable_ofdm_with_pss_plus_data10      = false;
enable_ofdm_with_pss_plus_data11      = false;
enable_ofdm_with_pss_plus_data12      = false;
enable_ofdm_with_pss_plus_data13      = false;
enable_ofdm_with_pss_plus_data14      = false;
enable_ofdm_with_pss_plus_data15      = false;
enable_ofdm_with_pss_plus_data16      = true;


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
    
    % Calculate sampling rate.
    sampling_rate = NDFT*delta_f;
    
%     % Create FIR filter for improved PSS detection.
%     FIR_ORDER = 512;
%     %pss_filter = fir1(FIR_ORDER, ((PSS_GUARD_BAND+SRSLTE_PSS_LEN)*delta_f)/sampling_rate).';
%     pss_filter = fir1(FIR_ORDER, (1.4e6)/sampling_rate).';
%     %freqz(fir_coef,1,512)
    
    if(1)
        toneOffset = 2.5;        % Tone offset or excess bandwidth (in subcarriers)
        FILTER_ORDER = 512;
        L = FILTER_ORDER + 1;                 % Filter length (=filterOrder+1), odd

        numDataCarriers = 72;    % number of data subcarriers in sub-band
        halfFilt = floor(L/2);
        n = -halfFilt:halfFilt;
        
        % Sinc function prototype filter
        pb = sinc((numDataCarriers+2*toneOffset).*n./NDFT);
        
        % Sinc truncation window
        w = (0.5*(1+cos(2*pi.*n/(L-1)))).^0.6;
        
        % Normalized lowpass filter coefficients
        fnum = (pb.*w)/sum(pb.*w);
        
%         % Filter impulse response
%         h = fvtool(fnum, 'Analysis', 'freq', 'NormalizedFrequency', 'off', 'Fs', NDFT*deltaF);
%         h.CurrentAxes.XLabel.String = 'Time (\mus)';
%         h.FigureToolbar = 'off';
        
        % Use dsp filter objects for filtering
        pss_filter_obj = dsp.FIRFilter('Structure', 'Direct form symmetric', 'Numerator', fnum);
        
        pss_filter = pss_filter_obj.Numerator;
        
        pss_filter = pss_filter.';
    end
    
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
    
    local_conj_time_domain_pss = [local_conj_time_domain_pss; zeros(FRAME_SIZE,1)];
    
    local_conj_pss_fft = fft(local_conj_time_domain_pss,NDFT2);
    
    correct_detection_with_data2 = zeros(1,length(SNR));
    miss_detection_with_data2 = zeros(1,length(SNR));
    false_detection_with_data2 = zeros(1,length(SNR));
    rng(12041988);
    for snr_idx = 1:1:length(SNR)
        
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
            
            num_int = 0; %randi((FRAME_SIZE/2)-NDFT);
            
            rx_signal = [zeros(num_int,1); subframe];
            
            rx_signal_buffer = rx_signal(1:FRAME_SIZE);
            
            if(add_noise == 1)
                noisy_signal = awgn(rx_signal_buffer, SNR(snr_idx), 'measured');
            else
                noisy_signal = rx_signal_buffer;
            end
            
            if(0)
                start_idx   = cpLen_1st_symb(phy_bw_idx) + 6*cpLen_other_symbs(phy_bw_idx) + 6*NDFT + 1;
                end_idx     = cpLen_1st_symb(phy_bw_idx) + 6*cpLen_other_symbs(phy_bw_idx) + 7*NDFT;
                test_signal = noisy_signal(start_idx:end_idx);
                figure;
                plot(0:1:NDFT-1, abs(fft(test_signal)).^2);
                title('Before filtering');
            end
            
            % Filter the received signal.
            filtered_noisy_signal = filter(pss_filter, 1, noisy_signal);
            
            if(0)
                start_idx   = cpLen_1st_symb(phy_bw_idx) + 6*cpLen_other_symbs(phy_bw_idx) + 6*NDFT + 1;
                end_idx     = cpLen_1st_symb(phy_bw_idx) + 6*cpLen_other_symbs(phy_bw_idx) + 7*NDFT;
                test_signal = filtered_noisy_signal(start_idx:end_idx);
                figure;
                plot(0:1:NDFT-1, abs(fft(test_signal)).^2);
                title('After filtering');
            end
            
            if(0)
                figure;
                plot(abs(fft(noisy_signal)).^2);
                figure;
                plot(abs(fft(filtered_noisy_signal)).^2);
            end
            
            % Detection of PSS.
            input_signal = [filtered_noisy_signal; zeros(NDFT2-FRAME_SIZE,1)];
            
            received_signal_fft = fft(input_signal, NDFT2);
            
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
            
            side_lobe_values_vector = [power_conv(sl_right), power_conv(sl_left)];
            
            psr_max = peak_value/max(side_lobe_values_vector);
            
            psr_min = peak_value/min(side_lobe_values_vector);
            
            psr = (psr_max + psr_min)/2;
            
            if(psr_max >= threshold)
                if(peak_pos-(L/2)-1 >= (SUBFRAME_LENGTH(phy_bw_idx)/2)-3 && peak_pos-(L/2)-1 <= (SUBFRAME_LENGTH(phy_bw_idx)/2)+3)
                    correct_detection_with_data2(snr_idx) = correct_detection_with_data2(snr_idx) + 1;
                else
                    false_detection_with_data2(snr_idx) = false_detection_with_data2(snr_idx) + 1;
                end
            else
                miss_detection_with_data2(snr_idx) = miss_detection_with_data2(snr_idx) + 1;
            end
            
        end
        
    end
    
    correct_detection_with_data2 = correct_detection_with_data2./NUM_ITER;
    miss_detection_with_data2 = miss_detection_with_data2./NUM_ITER;
    false_detection_with_data2 = false_detection_with_data2./NUM_ITER;
end

%% --------------------------------------------------------------------------
if(enable_ofdm_with_pss_plus_data3)
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
    
    correct_detection_with_data3 = zeros(1,length(SNR));
    miss_detection_with_data3 = zeros(1,length(SNR));
    false_detection_with_data3 = zeros(1,length(SNR));
    psr_avg3 = zeros(length(SNR));
    psr_cnt3 = zeros(length(SNR));    
    rng(12041988);
    for snr_idx = 1:1:length(SNR)
        
        for n=1:1:NUM_ITER
            
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
            
            received_signal_fft = fft(input_signal,NDFT2);
            
            local_conj_pss_fft = fft(local_conj_time_domain_pss,NDFT2);
            
            prod = received_signal_fft.*local_conj_pss_fft;
            
            convolution = ifft(prod,NDFT2);
            
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
            
            side_lobe_values_vector = [power_conv(sl_right), power_conv(sl_left)];
            
            psr_max = peak_value/max(side_lobe_values_vector);
            
            psr_min = peak_value/min(side_lobe_values_vector);
            
            psr = (psr_max + psr_min)/2;

            if(psr >= threshold)
                if(peak_pos-1 >= (SUBFRAME_LENGTH(phy_bw_idx)/2)-3 && peak_pos-1 <= (SUBFRAME_LENGTH(phy_bw_idx)/2)+3)
                    correct_detection_with_data3(snr_idx) = correct_detection_with_data3(snr_idx) + 1;
                else
                    false_detection_with_data3(snr_idx) = false_detection_with_data3(snr_idx) + 1;
                end
                
                psr_avg3(snr_idx) = psr_avg3(snr_idx) + psr;
                psr_cnt3(snr_idx) = psr_cnt3(snr_idx) + 1;                  
            else
                miss_detection_with_data3(snr_idx) = miss_detection_with_data3(snr_idx) + 1;
            end
            
        end
        
    end
    
    correct_detection_with_data3 = correct_detection_with_data3./NUM_ITER;
    miss_detection_with_data3 = miss_detection_with_data3./NUM_ITER;
    false_detection_with_data3 = false_detection_with_data3./NUM_ITER;
    
    psr_avg3(snr_idx) = psr_avg3(snr_idx)./psr_cnt3(snr_idx);    
end

%% --------------------------------------------------------------------------
if(enable_ofdm_with_pss_plus_data4)
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
    
    correct_detection_with_data4 = zeros(1,length(SNR));
    miss_detection_with_data4 = zeros(1,length(SNR));
    false_detection_with_data4 = zeros(1,length(SNR));
    rng(12041988);
    for snr_idx = 1:1:length(SNR)
        
        for n=1:1:NUM_ITER
            
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
            
            received_signal_fft = fft(input_signal,NDFT2);
            
            local_conj_pss_fft = fft(local_conj_time_domain_pss,NDFT2);
            
            prod = received_signal_fft.*local_conj_pss_fft;
            
            convolution = ifft(prod,NDFT2);
            
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
            
            side_lobe_values_vector = [power_conv(sl_right), power_conv(sl_left)];
            
            psr_max = peak_value/max(side_lobe_values_vector);
            
            psr_min = peak_value/min(side_lobe_values_vector);

            if(psr_min >= threshold)
                if(peak_pos-1 >= (SUBFRAME_LENGTH(phy_bw_idx)/2)-3 && peak_pos-1 <= (SUBFRAME_LENGTH(phy_bw_idx)/2)+3)
                    correct_detection_with_data4(snr_idx) = correct_detection_with_data4(snr_idx) + 1;
                else
                    false_detection_with_data4(snr_idx) = false_detection_with_data4(snr_idx) + 1;
                end
            else
                miss_detection_with_data4(snr_idx) = miss_detection_with_data4(snr_idx) + 1;
            end
            
        end
        
    end
    
    correct_detection_with_data4 = correct_detection_with_data4./NUM_ITER;
    miss_detection_with_data4 = miss_detection_with_data4./NUM_ITER;
    false_detection_with_data4 = false_detection_with_data4./NUM_ITER;
end


%% --------------------------------------------------------------------------
if(enable_ofdm_with_pss_plus_data5)
    % Generate PSS signal.
    %pss = lte_pss_zc(cell_id);
    
    SCATTER_PSS_LEN5 = 62;
    
    pss = scatter_pss_zcv1(cell_id, SCATTER_PSS_LEN5);

    SCATTER_PSS_GUARD_BAND = ((SRSLTE_PSS_LEN + PSS_GUARD_BAND) - SCATTER_PSS_LEN5);
    
    nof_pss_nulls = SCATTER_PSS_GUARD_BAND/2;
    
    % Create OFDM symbol number 6, the one which carries PSS.
    pss_zero_pad = [0;pss((SCATTER_PSS_LEN5/2)+1:end);zeros(NDFT-(SCATTER_PSS_LEN5+1),1);pss(1:SCATTER_PSS_LEN5/2)];
    local_time_domain_pss = (sqrt(NDFT)/sqrt(SCATTER_PSS_LEN5))*ifft(pss_zero_pad, NDFT);
    
    %if(1)
    if(plot_figures == 1)
        ofdm_symbol = (1/sqrt(NDFT))*ifftshift(fft(local_time_domain_pss,NDFT));
        figure;
        plot(0:1:NDFT-1,10*log10(abs(ofdm_symbol)),'b-')
    end
    
    % Conjugated local version of PSS in time domain.
    local_conj_time_domain_pss = conj(local_time_domain_pss);
    
    local_conj_time_domain_pss = [local_conj_time_domain_pss; zeros(NDFT2-length(local_conj_time_domain_pss),1)];
    
    correct_detection_with_data5 = zeros(1,length(SNR));
    miss_detection_with_data5 = zeros(1,length(SNR));
    false_detection_with_data5 = zeros(1,length(SNR));
    psr_avg5 = zeros(length(SNR));
    psr_cnt5 = zeros(length(SNR));
    rng(12041988);
    for snr_idx = 1:1:length(SNR)
        
        for n=1:1:NUM_ITER
            
            subframe = [];
            for symb_idx = 1:1:14
                if(symb_idx == 7)
                    % Generate random QPSK data.
                    data = randi([0 3], USEFUL_SUBCARRIERS-SCATTER_PSS_LEN5-SCATTER_PSS_GUARD_BAND, 1);
                    % Apply QPSK modulation.
                    data_symbols = qammod(data, 4, 'UnitAveragePower', true);
                    tx_pss_zero_pad = [0; pss((SCATTER_PSS_LEN5/2)+1:end); zeros(nof_pss_nulls,1); data_symbols(1:114,1); zeros(83,1); data_symbols(114+1:end,1); zeros(nof_pss_nulls,1); pss(1:SCATTER_PSS_LEN5/2)];
                    tx_time_domain_pss_plus_data = (sqrt(NDFT)/sqrt(SCATTER_PSS_LEN5))*ifft(tx_pss_zero_pad, NDFT);
                    tx_time_domain_pss_plus_data_plus_cp = [tx_time_domain_pss_plus_data(end-cpLen_other_symbs(phy_bw_idx)+1:end); tx_time_domain_pss_plus_data];
                    
                    subframe = [subframe; tx_time_domain_pss_plus_data_plus_cp];
                else
                    % Generate random QPSK data.
                    data = randi([0 3], USEFUL_SUBCARRIERS, 1);
                    % Apply QPSK modulation.
                    data_symbols = qammod(data, 4, 'UnitAveragePower', true);
                    
                    tx_data_zero_pad = [0; data_symbols(1:USEFUL_SUBCARRIERS/2,1); zeros(83,1); data_symbols(USEFUL_SUBCARRIERS/2+1:end,1)];
                    tx_time_domain_data = (sqrt(NDFT)/sqrt(SCATTER_PSS_LEN5))*ifft(tx_data_zero_pad, NDFT);
                    
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
            
            received_signal_fft = fft(input_signal,NDFT2);
            
            local_conj_pss_fft = fft(local_conj_time_domain_pss,NDFT2);
            
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
            
            side_lobe_values_vector = [power_conv(sl_right), power_conv(sl_left)];
            
            psr_max = peak_value/max(side_lobe_values_vector);
            
            psr_min = peak_value/min(side_lobe_values_vector);
            
            psr = (psr_max + psr_min)/2;

            if(psr >= threshold)
                if(peak_pos-1 >= (SUBFRAME_LENGTH(phy_bw_idx)/2)-3 && peak_pos-1 <= (SUBFRAME_LENGTH(phy_bw_idx)/2)+3)
                    correct_detection_with_data5(snr_idx) = correct_detection_with_data5(snr_idx) + 1;
                else
                    false_detection_with_data5(snr_idx) = false_detection_with_data5(snr_idx) + 1;
                end
                
                psr_avg5(snr_idx) = psr_avg5(snr_idx) + psr;
                psr_cnt5(snr_idx) = psr_cnt5(snr_idx) + 1;
            else
                miss_detection_with_data5(snr_idx) = miss_detection_with_data5(snr_idx) + 1;
            end
            
        end
        
    end
    
    correct_detection_with_data5 = correct_detection_with_data5./NUM_ITER;
    miss_detection_with_data5 = miss_detection_with_data5./NUM_ITER;
    false_detection_with_data5 = false_detection_with_data5./NUM_ITER;
    
    psr_avg5(snr_idx) = psr_avg5(snr_idx)./psr_cnt5(snr_idx);
end

%% --------------------------------------------------------------------------
if(enable_ofdm_with_pss_plus_data6)
    % Generate PSS signal.
    %pss = lte_pss_zc(cell_id);
    
    SCATTER_PSS_LEN6 = 64;
    
    pss = scatter_pss_zcv1(cell_id, SCATTER_PSS_LEN6);

    SCATTER_PSS_GUARD_BAND = ((SRSLTE_PSS_LEN + PSS_GUARD_BAND) - SCATTER_PSS_LEN6);
    
    nof_pss_nulls = SCATTER_PSS_GUARD_BAND/2;
    
    % Create OFDM symbol number 6, the one which carries PSS.
    pss_zero_pad = [0;pss((SCATTER_PSS_LEN6/2)+1:end);zeros(NDFT-(SCATTER_PSS_LEN6+1),1);pss(1:SCATTER_PSS_LEN6/2)];
    local_time_domain_pss = (sqrt(NDFT)/sqrt(SCATTER_PSS_LEN6))*ifft(pss_zero_pad, NDFT);
    
    %if(1)
    if(plot_figures == 1)
        ofdm_symbol = (1/sqrt(NDFT))*ifftshift(fft(local_time_domain_pss,NDFT));
        figure;
        plot(0:1:NDFT-1,10*log10(abs(ofdm_symbol)),'b-')
    end
    
    % Conjugated local version of PSS in time domain.
    local_conj_time_domain_pss = conj(local_time_domain_pss);
    
    local_conj_time_domain_pss = [local_conj_time_domain_pss; zeros(NDFT2-length(local_conj_time_domain_pss),1)];
    
    correct_detection_with_data6 = zeros(1,length(SNR));
    miss_detection_with_data6 = zeros(1,length(SNR));
    false_detection_with_data6 = zeros(1,length(SNR));
    psr_avg6 = zeros(length(SNR));
    psr_cnt6 = zeros(length(SNR));    
    rng(12041988);
    for snr_idx = 1:1:length(SNR)
        
        for n=1:1:NUM_ITER
            
            subframe = [];
            for symb_idx = 1:1:14
                if(symb_idx == 7)
                    % Generate random QPSK data.
                    data = randi([0 3], USEFUL_SUBCARRIERS-SCATTER_PSS_LEN6-SCATTER_PSS_GUARD_BAND, 1);
                    % Apply QPSK modulation.
                    data_symbols = qammod(data, 4, 'UnitAveragePower', true);
                    tx_pss_zero_pad = [0; pss((SCATTER_PSS_LEN6/2)+1:end); zeros(nof_pss_nulls,1); data_symbols(1:114,1); zeros(83,1); data_symbols(114+1:end,1); zeros(nof_pss_nulls,1); pss(1:SCATTER_PSS_LEN6/2)];
                    tx_time_domain_pss_plus_data = (sqrt(NDFT)/sqrt(SCATTER_PSS_LEN6))*ifft(tx_pss_zero_pad, NDFT);
                    tx_time_domain_pss_plus_data_plus_cp = [tx_time_domain_pss_plus_data(end-cpLen_other_symbs(phy_bw_idx)+1:end); tx_time_domain_pss_plus_data];
                    
                    subframe = [subframe; tx_time_domain_pss_plus_data_plus_cp];
                else
                    % Generate random QPSK data.
                    data = randi([0 3], USEFUL_SUBCARRIERS, 1);
                    % Apply QPSK modulation.
                    data_symbols = qammod(data, 4, 'UnitAveragePower', true);
                    
                    tx_data_zero_pad = [0; data_symbols(1:USEFUL_SUBCARRIERS/2,1); zeros(83,1); data_symbols(USEFUL_SUBCARRIERS/2+1:end,1)];
                    tx_time_domain_data = (sqrt(NDFT)/sqrt(SCATTER_PSS_LEN6))*ifft(tx_data_zero_pad, NDFT);
                    
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
            
            received_signal_fft = fft(input_signal,NDFT2);
            
            local_conj_pss_fft = fft(local_conj_time_domain_pss,NDFT2);
            
            prod = received_signal_fft.*local_conj_pss_fft;
            
            convolution = ifft(prod,NDFT2);
            
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
            
            side_lobe_values_vector = [power_conv(sl_right), power_conv(sl_left)];
            
            psr_max = peak_value/max(side_lobe_values_vector);
            
            psr_min = peak_value/min(side_lobe_values_vector);
            
            psr = (psr_max + psr_min)/2;

            if(psr >= threshold)
                if(peak_pos-1 >= (SUBFRAME_LENGTH(phy_bw_idx)/2)-3 && peak_pos-1 <= (SUBFRAME_LENGTH(phy_bw_idx)/2)+3)
                    correct_detection_with_data6(snr_idx) = correct_detection_with_data6(snr_idx) + 1;
                else
                    false_detection_with_data6(snr_idx) = false_detection_with_data6(snr_idx) + 1;
                end
                
                psr_avg6(snr_idx) = psr_avg6(snr_idx) + psr;
                psr_cnt6(snr_idx) = psr_cnt6(snr_idx) + 1;                
            else
                miss_detection_with_data6(snr_idx) = miss_detection_with_data6(snr_idx) + 1;
            end
            
        end
        
    end
    
    correct_detection_with_data6 = correct_detection_with_data6./NUM_ITER;
    miss_detection_with_data6 = miss_detection_with_data6./NUM_ITER;
    false_detection_with_data6 = false_detection_with_data6./NUM_ITER;
    
    psr_avg6(snr_idx) = psr_avg6(snr_idx)./psr_cnt6(snr_idx);    
end

%% --------------------------------------------------------------------------
if(enable_ofdm_with_pss_plus_data7)
    % Generate PSS signal.
    %pss = lte_pss_zc(cell_id);
    
    SCATTER_PSS_LEN7 = 66;
    
    pss = scatter_pss_zcv1(cell_id, SCATTER_PSS_LEN7);

    SCATTER_PSS_GUARD_BAND = ((SRSLTE_PSS_LEN + PSS_GUARD_BAND) - SCATTER_PSS_LEN7);
    
    nof_pss_nulls = SCATTER_PSS_GUARD_BAND/2;
    
    % Create OFDM symbol number 6, the one which carries PSS.
    pss_zero_pad = [0;pss((SCATTER_PSS_LEN7/2)+1:end);zeros(NDFT-(SCATTER_PSS_LEN7+1),1);pss(1:SCATTER_PSS_LEN7/2)];
    local_time_domain_pss = (sqrt(NDFT)/sqrt(SCATTER_PSS_LEN7))*ifft(pss_zero_pad, NDFT);
    
    %if(1)
    if(plot_figures == 1)
        ofdm_symbol = (1/sqrt(NDFT))*ifftshift(fft(local_time_domain_pss,NDFT));
        figure;
        plot(0:1:NDFT-1,10*log10(abs(ofdm_symbol)),'b-')
    end
    
    % Conjugated local version of PSS in time domain.
    local_conj_time_domain_pss = conj(local_time_domain_pss);
    
    local_conj_time_domain_pss = [local_conj_time_domain_pss; zeros(NDFT2-length(local_conj_time_domain_pss),1)];
    
    correct_detection_with_data7 = zeros(1,length(SNR));
    miss_detection_with_data7 = zeros(1,length(SNR));
    false_detection_with_data7 = zeros(1,length(SNR));
    psr_avg7 = zeros(length(SNR));
    psr_cnt7 = zeros(length(SNR));    
    rng(12041988);
    for snr_idx = 1:1:length(SNR)
        
        for n=1:1:NUM_ITER
            
            subframe = [];
            for symb_idx = 1:1:14
                if(symb_idx == 7)
                    % Generate random QPSK data.
                    data = randi([0 3], USEFUL_SUBCARRIERS-SCATTER_PSS_LEN7-SCATTER_PSS_GUARD_BAND, 1);
                    % Apply QPSK modulation.
                    data_symbols = qammod(data, 4, 'UnitAveragePower', true);
                    tx_pss_zero_pad = [0; pss((SCATTER_PSS_LEN7/2)+1:end); zeros(nof_pss_nulls,1); data_symbols(1:114,1); zeros(83,1); data_symbols(114+1:end,1); zeros(nof_pss_nulls,1); pss(1:SCATTER_PSS_LEN7/2)];
                    tx_time_domain_pss_plus_data = (sqrt(NDFT)/sqrt(SCATTER_PSS_LEN7))*ifft(tx_pss_zero_pad, NDFT);
                    tx_time_domain_pss_plus_data_plus_cp = [tx_time_domain_pss_plus_data(end-cpLen_other_symbs(phy_bw_idx)+1:end); tx_time_domain_pss_plus_data];
                    
                    subframe = [subframe; tx_time_domain_pss_plus_data_plus_cp];
                else
                    % Generate random QPSK data.
                    data = randi([0 3], USEFUL_SUBCARRIERS, 1);
                    % Apply QPSK modulation.
                    data_symbols = qammod(data, 4, 'UnitAveragePower', true);
                    
                    tx_data_zero_pad = [0; data_symbols(1:USEFUL_SUBCARRIERS/2,1); zeros(83,1); data_symbols(USEFUL_SUBCARRIERS/2+1:end,1)];
                    tx_time_domain_data = (sqrt(NDFT)/sqrt(SCATTER_PSS_LEN7))*ifft(tx_data_zero_pad, NDFT);
                    
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
            
            received_signal_fft = fft(input_signal,NDFT2);
            
            local_conj_pss_fft = fft(local_conj_time_domain_pss,NDFT2);
            
            prod = received_signal_fft.*local_conj_pss_fft;
            
            convolution = ifft(prod,NDFT2);
            
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
            
            side_lobe_values_vector = [power_conv(sl_right), power_conv(sl_left)];
            
            psr_max = peak_value/max(side_lobe_values_vector);
            
            psr_min = peak_value/min(side_lobe_values_vector);
            
            psr = (psr_max + psr_min)/2;

            if(psr >= threshold)
                if(peak_pos-1 >= (SUBFRAME_LENGTH(phy_bw_idx)/2)-3 && peak_pos-1 <= (SUBFRAME_LENGTH(phy_bw_idx)/2)+3)
                    correct_detection_with_data7(snr_idx) = correct_detection_with_data7(snr_idx) + 1;
                else
                    false_detection_with_data7(snr_idx) = false_detection_with_data7(snr_idx) + 1;
                end
                
                psr_avg7(snr_idx) = psr_avg7(snr_idx) + psr;
                psr_cnt7(snr_idx) = psr_cnt7(snr_idx) + 1;                
            else
                miss_detection_with_data7(snr_idx) = miss_detection_with_data7(snr_idx) + 1;
            end
            
        end
        
    end
    
    correct_detection_with_data7 = correct_detection_with_data7./NUM_ITER;
    miss_detection_with_data7 = miss_detection_with_data7./NUM_ITER;
    false_detection_with_data7 = false_detection_with_data7./NUM_ITER;
    
    psr_avg7(snr_idx) = psr_avg7(snr_idx)./psr_cnt7(snr_idx);
end

%% --------------------------------------------------------------------------
if(enable_ofdm_with_pss_plus_data8)
    % Generate PSS signal.
    %pss = lte_pss_zc(cell_id);
    
    SCATTER_PSS_LEN8 = 68;
    
    pss = scatter_pss_zcv1(cell_id, SCATTER_PSS_LEN8);

    SCATTER_PSS_GUARD_BAND = ((SRSLTE_PSS_LEN + PSS_GUARD_BAND) - SCATTER_PSS_LEN8);
    
    nof_pss_nulls = SCATTER_PSS_GUARD_BAND/2;
    
    % Create OFDM symbol number 6, the one which carries PSS.
    pss_zero_pad = [0;pss((SCATTER_PSS_LEN8/2)+1:end);zeros(NDFT-(SCATTER_PSS_LEN8+1),1);pss(1:SCATTER_PSS_LEN8/2)];
    local_time_domain_pss = (sqrt(NDFT)/sqrt(SCATTER_PSS_LEN8))*ifft(pss_zero_pad, NDFT);
    
    %if(1)
    if(plot_figures == 1)
        ofdm_symbol = (1/sqrt(NDFT))*ifftshift(fft(local_time_domain_pss,NDFT));
        figure;
        plot(0:1:NDFT-1,10*log10(abs(ofdm_symbol)),'b-')
    end
    
    % Conjugated local version of PSS in time domain.
    local_conj_time_domain_pss = conj(local_time_domain_pss);
    
    local_conj_time_domain_pss = [local_conj_time_domain_pss; zeros(NDFT2-length(local_conj_time_domain_pss),1)];
    
    correct_detection_with_data8 = zeros(1,length(SNR));
    miss_detection_with_data8 = zeros(1,length(SNR));
    false_detection_with_data8 = zeros(1,length(SNR));
    psr_avg8 = zeros(length(SNR));
    psr_cnt8 = zeros(length(SNR));    
    rng(12041988);
    for snr_idx = 1:1:length(SNR)
        
        for n=1:1:NUM_ITER
            
            subframe = [];
            for symb_idx = 1:1:14
                if(symb_idx == 7)
                    % Generate random QPSK data.
                    data = randi([0 3], USEFUL_SUBCARRIERS-SCATTER_PSS_LEN8-SCATTER_PSS_GUARD_BAND, 1);
                    % Apply QPSK modulation.
                    data_symbols = qammod(data, 4, 'UnitAveragePower', true);
                    tx_pss_zero_pad = [0; pss((SCATTER_PSS_LEN8/2)+1:end); zeros(nof_pss_nulls,1); data_symbols(1:114,1); zeros(83,1); data_symbols(114+1:end,1); zeros(nof_pss_nulls,1); pss(1:SCATTER_PSS_LEN8/2)];
                    tx_time_domain_pss_plus_data = (sqrt(NDFT)/sqrt(SCATTER_PSS_LEN8))*ifft(tx_pss_zero_pad, NDFT);
                    tx_time_domain_pss_plus_data_plus_cp = [tx_time_domain_pss_plus_data(end-cpLen_other_symbs(phy_bw_idx)+1:end); tx_time_domain_pss_plus_data];
                    
                    subframe = [subframe; tx_time_domain_pss_plus_data_plus_cp];
                else
                    % Generate random QPSK data.
                    data = randi([0 3], USEFUL_SUBCARRIERS, 1);
                    % Apply QPSK modulation.
                    data_symbols = qammod(data, 4, 'UnitAveragePower', true);
                    
                    tx_data_zero_pad = [0; data_symbols(1:USEFUL_SUBCARRIERS/2,1); zeros(83,1); data_symbols(USEFUL_SUBCARRIERS/2+1:end,1)];
                    tx_time_domain_data = (sqrt(NDFT)/sqrt(SCATTER_PSS_LEN8))*ifft(tx_data_zero_pad, NDFT);
                    
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
            
            received_signal_fft = fft(input_signal,NDFT2);
            
            local_conj_pss_fft = fft(local_conj_time_domain_pss,NDFT2);
            
            prod = received_signal_fft.*local_conj_pss_fft;
            
            convolution = ifft(prod,NDFT2);
            
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
            
            side_lobe_values_vector = [power_conv(sl_right), power_conv(sl_left)];
            
            psr_max = peak_value/max(side_lobe_values_vector);
            
            psr_min = peak_value/min(side_lobe_values_vector);
            
            psr = (psr_max + psr_min)/2;

            if(psr >= threshold)
                if(peak_pos-1 >= (SUBFRAME_LENGTH(phy_bw_idx)/2)-3 && peak_pos-1 <= (SUBFRAME_LENGTH(phy_bw_idx)/2)+3)
                    correct_detection_with_data8(snr_idx) = correct_detection_with_data8(snr_idx) + 1;
                else
                    false_detection_with_data8(snr_idx) = false_detection_with_data8(snr_idx) + 1;
                end
                
                psr_avg8(snr_idx) = psr_avg8(snr_idx) + psr;
                psr_cnt8(snr_idx) = psr_cnt8(snr_idx) + 1;                
            else
                miss_detection_with_data8(snr_idx) = miss_detection_with_data8(snr_idx) + 1;
            end
            
        end
        
    end
    
    correct_detection_with_data8 = correct_detection_with_data8./NUM_ITER;
    miss_detection_with_data8 = miss_detection_with_data8./NUM_ITER;
    false_detection_with_data8 = false_detection_with_data8./NUM_ITER;
    
    psr_avg8(snr_idx) = psr_avg8(snr_idx)./psr_cnt8(snr_idx);
end

%% --------------------------------------------------------------------------
if(enable_ofdm_with_pss_plus_data9)
    % Generate PSS signal.
    %pss = lte_pss_zc(cell_id);
    
    SCATTER_PSS_LEN9 = 70;
    
    pss = scatter_pss_zcv1(cell_id, SCATTER_PSS_LEN9);

    SCATTER_PSS_GUARD_BAND = ((SRSLTE_PSS_LEN + PSS_GUARD_BAND) - SCATTER_PSS_LEN9);
    
    nof_pss_nulls = SCATTER_PSS_GUARD_BAND/2;
    
    % Create OFDM symbol number 6, the one which carries PSS.
    pss_zero_pad = [0;pss((SCATTER_PSS_LEN9/2)+1:end);zeros(NDFT-(SCATTER_PSS_LEN9+1),1);pss(1:SCATTER_PSS_LEN9/2)];
    local_time_domain_pss = (sqrt(NDFT)/sqrt(SCATTER_PSS_LEN9))*ifft(pss_zero_pad, NDFT);
    
    %if(1)
    if(plot_figures == 1)
        ofdm_symbol = (1/sqrt(NDFT))*ifftshift(fft(local_time_domain_pss,NDFT));
        figure;
        plot(0:1:NDFT-1,10*log10(abs(ofdm_symbol)),'b-')
    end
    
    % Conjugated local version of PSS in time domain.
    local_conj_time_domain_pss = conj(local_time_domain_pss);
    
    local_conj_time_domain_pss = [local_conj_time_domain_pss; zeros(NDFT2-length(local_conj_time_domain_pss),1)];
    
    correct_detection_with_data9 = zeros(1,length(SNR));
    miss_detection_with_data9 = zeros(1,length(SNR));
    false_detection_with_data9 = zeros(1,length(SNR));
    psr_avg9 = zeros(length(SNR));
    psr_cnt9 = zeros(length(SNR));    
    rng(12041988);
    for snr_idx = 1:1:length(SNR)
        
        for n=1:1:NUM_ITER
            
            subframe = [];
            for symb_idx = 1:1:14
                if(symb_idx == 7)
                    % Generate random QPSK data.
                    data = randi([0 3], USEFUL_SUBCARRIERS-SCATTER_PSS_LEN9-SCATTER_PSS_GUARD_BAND, 1);
                    % Apply QPSK modulation.
                    data_symbols = qammod(data, 4, 'UnitAveragePower', true);
                    tx_pss_zero_pad = [0; pss((SCATTER_PSS_LEN9/2)+1:end); zeros(nof_pss_nulls,1); data_symbols(1:114,1); zeros(83,1); data_symbols(114+1:end,1); zeros(nof_pss_nulls,1); pss(1:SCATTER_PSS_LEN9/2)];
                    tx_time_domain_pss_plus_data = (sqrt(NDFT)/sqrt(SCATTER_PSS_LEN9))*ifft(tx_pss_zero_pad, NDFT);
                    tx_time_domain_pss_plus_data_plus_cp = [tx_time_domain_pss_plus_data(end-cpLen_other_symbs(phy_bw_idx)+1:end); tx_time_domain_pss_plus_data];
                    
                    subframe = [subframe; tx_time_domain_pss_plus_data_plus_cp];
                else
                    % Generate random QPSK data.
                    data = randi([0 3], USEFUL_SUBCARRIERS, 1);
                    % Apply QPSK modulation.
                    data_symbols = qammod(data, 4, 'UnitAveragePower', true);
                    
                    tx_data_zero_pad = [0; data_symbols(1:USEFUL_SUBCARRIERS/2,1); zeros(83,1); data_symbols(USEFUL_SUBCARRIERS/2+1:end,1)];
                    tx_time_domain_data = (sqrt(NDFT)/sqrt(SCATTER_PSS_LEN9))*ifft(tx_data_zero_pad, NDFT);
                    
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
            
            received_signal_fft = fft(input_signal,NDFT2);
            
            local_conj_pss_fft = fft(local_conj_time_domain_pss,NDFT2);
            
            prod = received_signal_fft.*local_conj_pss_fft;
            
            convolution = ifft(prod,NDFT2);
            
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
            
            side_lobe_values_vector = [power_conv(sl_right), power_conv(sl_left)];
            
            psr_max = peak_value/max(side_lobe_values_vector);
            
            psr_min = peak_value/min(side_lobe_values_vector);
            
            psr = (psr_max + psr_min)/2;

            if(psr >= threshold)
                if(peak_pos-1 >= (SUBFRAME_LENGTH(phy_bw_idx)/2)-3 && peak_pos-1 <= (SUBFRAME_LENGTH(phy_bw_idx)/2)+3)
                    correct_detection_with_data9(snr_idx) = correct_detection_with_data9(snr_idx) + 1;
                else
                    false_detection_with_data9(snr_idx) = false_detection_with_data9(snr_idx) + 1;
                end
                
                psr_avg9(snr_idx) = psr_avg9(snr_idx) + psr;
                psr_cnt9(snr_idx) = psr_cnt9(snr_idx) + 1;                
            else
                miss_detection_with_data9(snr_idx) = miss_detection_with_data9(snr_idx) + 1;
            end
            
        end
        
    end
    
    correct_detection_with_data9 = correct_detection_with_data9./NUM_ITER;
    miss_detection_with_data9 = miss_detection_with_data9./NUM_ITER;
    false_detection_with_data9 = false_detection_with_data9./NUM_ITER;
    
    psr_avg9(snr_idx) = psr_avg9(snr_idx)./psr_cnt9(snr_idx);
end

%% --------------------------------------------------------------------------
if(enable_ofdm_with_pss_plus_data10)
    % Generate PSS signal.
    %pss = lte_pss_zc(cell_id);
    
    SCATTER_PSS_LEN10 = 72;
    
    pss = scatter_pss_zcv1(cell_id, SCATTER_PSS_LEN10);

    SCATTER_PSS_GUARD_BAND = ((SRSLTE_PSS_LEN + PSS_GUARD_BAND) - SCATTER_PSS_LEN10);
    
    nof_pss_nulls = SCATTER_PSS_GUARD_BAND/2;
    
    % Create OFDM symbol number 6, the one which carries PSS.
    pss_zero_pad = [0;pss((SCATTER_PSS_LEN10/2)+1:end);zeros(NDFT-(SCATTER_PSS_LEN10+1),1);pss(1:SCATTER_PSS_LEN10/2)];
    local_time_domain_pss = (sqrt(NDFT)/sqrt(SCATTER_PSS_LEN10))*ifft(pss_zero_pad, NDFT);
    
    %if(1)
    if(plot_figures == 1)
        ofdm_symbol = (1/sqrt(NDFT))*ifftshift(fft(local_time_domain_pss,NDFT));
        figure;
        plot(0:1:NDFT-1,10*log10(abs(ofdm_symbol)),'b-')
    end
    
    % Conjugated local version of PSS in time domain.
    local_conj_time_domain_pss = conj(local_time_domain_pss);
    
    local_conj_time_domain_pss = [local_conj_time_domain_pss; zeros(NDFT2-length(local_conj_time_domain_pss),1)];
    
    correct_detection_with_data10 = zeros(1,length(SNR));
    miss_detection_with_data10 = zeros(1,length(SNR));
    false_detection_with_data10 = zeros(1,length(SNR));
    psr_avg10 = zeros(length(SNR));
    psr_cnt10 = zeros(length(SNR));    
    rng(12041988);
    for snr_idx = 1:1:length(SNR)
        
        for n=1:1:NUM_ITER
            
            subframe = [];
            for symb_idx = 1:1:14
                if(symb_idx == 7)
                    % Generate random QPSK data.
                    data = randi([0 3], USEFUL_SUBCARRIERS-SCATTER_PSS_LEN10-SCATTER_PSS_GUARD_BAND, 1);
                    % Apply QPSK modulation.
                    data_symbols = qammod(data, 4, 'UnitAveragePower', true);
                    tx_pss_zero_pad = [0; pss((SCATTER_PSS_LEN10/2)+1:end); zeros(nof_pss_nulls,1); data_symbols(1:114,1); zeros(83,1); data_symbols(114+1:end,1); zeros(nof_pss_nulls,1); pss(1:SCATTER_PSS_LEN10/2)];
                    tx_time_domain_pss_plus_data = (sqrt(NDFT)/sqrt(SCATTER_PSS_LEN10))*ifft(tx_pss_zero_pad, NDFT);
                    tx_time_domain_pss_plus_data_plus_cp = [tx_time_domain_pss_plus_data(end-cpLen_other_symbs(phy_bw_idx)+1:end); tx_time_domain_pss_plus_data];
                    
                    subframe = [subframe; tx_time_domain_pss_plus_data_plus_cp];
                else
                    % Generate random QPSK data.
                    data = randi([0 3], USEFUL_SUBCARRIERS, 1);
                    % Apply QPSK modulation.
                    data_symbols = qammod(data, 4, 'UnitAveragePower', true);
                    
                    tx_data_zero_pad = [0; data_symbols(1:USEFUL_SUBCARRIERS/2,1); zeros(83,1); data_symbols(USEFUL_SUBCARRIERS/2+1:end,1)];
                    tx_time_domain_data = (sqrt(NDFT)/sqrt(SCATTER_PSS_LEN10))*ifft(tx_data_zero_pad, NDFT);
                    
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
            
            received_signal_fft = fft(input_signal,NDFT2);
            
            local_conj_pss_fft = fft(local_conj_time_domain_pss,NDFT2);
            
            prod = received_signal_fft.*local_conj_pss_fft;
            
            convolution = ifft(prod,NDFT2);
            
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
            
            side_lobe_values_vector = [power_conv(sl_right), power_conv(sl_left)];
            
            psr_max = peak_value/max(side_lobe_values_vector);
            
            psr_min = peak_value/min(side_lobe_values_vector);
            
            psr = (psr_max + psr_min)/2;

            if(psr >= threshold)
                if(peak_pos-1 >= (SUBFRAME_LENGTH(phy_bw_idx)/2)-3 && peak_pos-1 <= (SUBFRAME_LENGTH(phy_bw_idx)/2)+3)
                    correct_detection_with_data10(snr_idx) = correct_detection_with_data10(snr_idx) + 1;
                else
                    false_detection_with_data10(snr_idx) = false_detection_with_data10(snr_idx) + 1;
                end
                
                psr_avg10(snr_idx) = psr_avg10(snr_idx) + psr;
                psr_cnt10(snr_idx) = psr_cnt10(snr_idx) + 1;                
            else
                miss_detection_with_data10(snr_idx) = miss_detection_with_data10(snr_idx) + 1;
            end
            
        end
        
    end
    
    correct_detection_with_data10 = correct_detection_with_data10./NUM_ITER;
    miss_detection_with_data10 = miss_detection_with_data10./NUM_ITER;
    false_detection_with_data10 = false_detection_with_data10./NUM_ITER;
    
    psr_avg10(snr_idx) = psr_avg10(snr_idx)./psr_cnt10(snr_idx);
end

if(enable_ofdm_with_pss_plus_data11)
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
    smaller_local_time_domain_pss = [smaller_local_time_domain_pss; zeros(NDFT, 1)];
    
    NDFT3 = 2*NDFT;
    
    correct_detection_with_data11 = zeros(1,length(SNR));
    miss_detection_with_data11 = zeros(1,length(SNR));
    false_detection_with_data11 = zeros(1,length(SNR));
    false_detection_peak_pos11 = zeros(length(SNR),NUM_ITER);
    miss_detection_psr11 = zeros(length(SNR), NUM_ITER);
    miss_detection_psr_cnt11 = ones(length(SNR));
    psr_avg11 = zeros(length(SNR));
    psr_cnt11 = zeros(length(SNR));     
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
            
            
            if(peak_pos-NDFT > 0)
                
                smaller_local_conj_pss_fft = fft(smaller_local_time_domain_pss, NDFT3);
                
                smaller_input_signal = [input_signal(peak_pos-NDFT:peak_pos-1); zeros(NDFT,1)];
                
                smaller_received_signal_fft = fft(smaller_input_signal, NDFT3);
                
                smaller_prod = smaller_received_signal_fft.*smaller_local_conj_pss_fft;
                
                smaller_convolution = ifft(smaller_prod, NDFT3);
                
                smaller_power_conv = abs(smaller_convolution).^2;
                
                [smaller_peak_value, smaller_peak_pos] = max(smaller_power_conv);
                
                
                
                
                
                pl_ub = smaller_peak_pos + 1;
                while(smaller_power_conv(pl_ub + 1) <= smaller_power_conv(pl_ub))
                    pl_ub = pl_ub + 1;
                end
                
                pl_lb = smaller_peak_pos - 1;
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
                
                psr_max = smaller_peak_value/side_lobe_value;
                
            else
                psr_max = 0;
                
            end
            
            if(psr_max >= threshold)
                if(peak_pos-1 >= (SUBFRAME_LENGTH(phy_bw_idx)/2)-3 && peak_pos-1 <= (SUBFRAME_LENGTH(phy_bw_idx)/2)+3)                    
                    correct_detection_with_data11(snr_idx) = correct_detection_with_data11(snr_idx) + 1;
                else
                    false_detection_with_data11(snr_idx) = false_detection_with_data11(snr_idx) + 1;
                    
                    false_detection_peak_pos11(snr_idx, false_detection_peak_pos_cnt) = peak_pos-1;
                    false_detection_peak_pos_cnt = false_detection_peak_pos_cnt + 1;
                end
                
                psr_avg11(snr_idx) = psr_avg11(snr_idx) + psr_max;
                psr_cnt11(snr_idx) = psr_cnt11(snr_idx) + 1;                 
            else
                miss_detection_with_data11(snr_idx) = miss_detection_with_data11(snr_idx) + 1;

                miss_detection_psr11(snr_idx, miss_detection_psr_cnt11(snr_idx)) = psr_max;
                miss_detection_psr_cnt11(snr_idx) = miss_detection_psr_cnt11(snr_idx) + 1;
            end
            
        end
        
    end
    
    correct_detection_with_data11 = correct_detection_with_data11./NUM_ITER;
    miss_detection_with_data11 = miss_detection_with_data11./NUM_ITER;
    false_detection_with_data11 = false_detection_with_data11./NUM_ITER;
    
    psr_avg11(snr_idx) = psr_avg11(snr_idx)./psr_cnt11(snr_idx);    
end

if(0)
    
if(enable_ofdm_with_pss_plus_data12)
    
    smaller_threshold = 2.0;
    
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
    
    
    NDFT3 = 3*NDFT;
    
    smaller_local_time_domain_pss = conj(local_time_domain_pss);
    smaller_local_time_domain_pss = [smaller_local_time_domain_pss; zeros(NDFT3 - length(smaller_local_time_domain_pss), 1)];  
    
    correct_detection_with_data12 = zeros(1,length(SNR));
    miss_detection_with_data12 = zeros(1,length(SNR));
    false_detection_with_data12 = zeros(1,length(SNR));
    false_detection_peak_pos12 = zeros(length(SNR),NUM_ITER);
    miss_detection_psr12 = zeros(length(SNR), NUM_ITER);
    miss_detection_psr_cnt12 = ones(length(SNR));
    psr_avg12 = zeros(length(SNR));
    psr_cnt12 = zeros(length(SNR));     
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
            
           
            
            
            
            
            
            
            if(peak_pos-2*NDFT > 0)
                
                
                
                
                smaller_local_conj_pss_fft = fft(smaller_local_time_domain_pss, NDFT3);
                
                %smaller_input_signal = [input_signal(peak_pos-NDFT:peak_pos-1); zeros(NDFT,1)];
                smaller_input_signal = [input_signal(peak_pos-2*NDFT:peak_pos-1); zeros(NDFT,1)];
                
                aux = input_signal(peak_pos-NDFT:peak_pos-1);
                r = xcorr((local_time_domain_pss), aux);
                
                smaller_received_signal_fft = fft(smaller_input_signal, NDFT3);
                
                smaller_prod = smaller_received_signal_fft.*smaller_local_conj_pss_fft;
                
                smaller_convolution = ifft(smaller_prod, NDFT3);
                
                smaller_power_conv = abs(smaller_convolution).^2;
                
                [smaller_peak_value, smaller_peak_pos] = max(smaller_power_conv);
                
                pl_ub = smaller_peak_pos + 1;
                while(smaller_power_conv(pl_ub + 1) <= smaller_power_conv(pl_ub))
                    pl_ub = pl_ub + 1;
                end
                
                pl_lb = smaller_peak_pos - 1;
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
                
                smaller_psr_max = smaller_peak_value/side_lobe_value;
                
            else
                
                smaller_psr_max = 0;
                
            end
            
            
            
            threshold = 3.5;
            
            smaller_threshold = 2.4;
            
            if(psr_max >= smaller_threshold && smaller_psr_max >= threshold)
                if(peak_pos-1 >= (SUBFRAME_LENGTH(phy_bw_idx)/2)-3 && peak_pos-1 <= (SUBFRAME_LENGTH(phy_bw_idx)/2)+3)                    
                    correct_detection_with_data12(snr_idx) = correct_detection_with_data12(snr_idx) + 1;
                else
                    false_detection_with_data12(snr_idx) = false_detection_with_data12(snr_idx) + 1;
                    
                    false_detection_peak_pos12(snr_idx, false_detection_peak_pos_cnt) = peak_pos-1;
                    false_detection_peak_pos_cnt = false_detection_peak_pos_cnt + 1;
                end
                
                psr_avg12(snr_idx) = psr_avg12(snr_idx) + smaller_psr_max;
                psr_cnt12(snr_idx) = psr_cnt12(snr_idx) + 1;                 
            else
                miss_detection_with_data12(snr_idx) = miss_detection_with_data12(snr_idx) + 1;

                miss_detection_psr12(snr_idx, miss_detection_psr_cnt12(snr_idx)) = smaller_psr_max;
                miss_detection_psr_cnt12(snr_idx) = miss_detection_psr_cnt12(snr_idx) + 1;
            end
            
        end
        
    end
    
    correct_detection_with_data12 = correct_detection_with_data12./NUM_ITER;
    miss_detection_with_data12 = miss_detection_with_data12./NUM_ITER;
    false_detection_with_data12 = false_detection_with_data12./NUM_ITER;
    
    psr_avg12(snr_idx) = psr_avg12(snr_idx)./psr_cnt12(snr_idx);    
end

end

if(enable_ofdm_with_pss_plus_data13)
    
    smaller_threshold = 2.0;
    
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
    
    
    
    NDFT3 = NDFT;
    
    smaller_local_time_domain_pss = conj(local_time_domain_pss);
    
    false_alarm_counter = 0;
                    
    correct_rejection_counter = 0;
    
    
    correct_detection_with_data13 = zeros(1,length(SNR));
    miss_detection_with_data13 = zeros(1,length(SNR));
    false_detection_with_data13 = zeros(1,length(SNR));
    false_detection_peak_pos13 = zeros(length(SNR),NUM_ITER);
    miss_detection_psr13 = zeros(length(SNR), NUM_ITER);
    miss_detection_psr_cnt13 = ones(length(SNR));
    psr_avg13 = zeros(length(SNR));
    psr_cnt13 = zeros(length(SNR));     
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
            
           
            
            
            
            
            
            
            if(peak_pos-NDFT > 0 && psr_max >= smaller_threshold)
                
                if(1)
                    smaller_local_conj_pss_fft = fft(smaller_local_time_domain_pss, NDFT3);
                    
                    poss = [2:2+(SRSLTE_PSS_LEN/2)-1, NDFT3-(SRSLTE_PSS_LEN/2)+1:NDFT3];
                    smaller_local_conj_pss_fft = smaller_local_conj_pss_fft(poss);

                    smaller_input_signal = input_signal(peak_pos-NDFT:peak_pos-1);

                    smaller_received_signal_fft = (fft(smaller_input_signal, NDFT3));
                    
                    poss = [2:2+(SRSLTE_PSS_LEN/2)-1, NDFT3-(SRSLTE_PSS_LEN/2)+1:NDFT3];
                    smaller_received_signal_fft = smaller_received_signal_fft(poss);

                    smaller_prod = smaller_received_signal_fft.*smaller_local_conj_pss_fft;

                    smaller_convolution = ifftshift(ifft(smaller_prod, SRSLTE_PSS_LEN));

                    smaller_power_conv = abs(smaller_convolution).^2;
                    
                    
                    %% ----------------------------------------------------
                    sorted_smaller_power_conv = sort(smaller_power_conv);
                    
                    Tcme = 3.5;%4;%6.9078; % for 1 sample.
                    % Algorithm used to discard samples. Indeed, it is used to find a
                    % speration between noisy samples and signal+noise samples.
                    for I = 10:1:SRSLTE_PSS_LEN-1
                        limiar = Tcme/I;
                        if(sorted_smaller_power_conv(I+1,1) > limiar*sum(sorted_smaller_power_conv(1:I,1)))
                            break;
                        end
                    end
                    
                    % Output of the disposal module. It's used as a noise reference.
                    Zref = sum(sorted_smaller_power_conv(1:I,1));
                    
                    % Calculate the scale factor.
                    Pfa = 0.0001;
                    alpha = finv(1-Pfa,(1*2),(I*1*2))/I;  
                    
                    
                    [smaller_peak_value, smaller_peak_pos] = max(smaller_power_conv);
                    
                    smaller_ratio = smaller_peak_value/Zref;
                    
                    
                    
                    if(smaller_ratio > alpha)
                        false_alarm_counter = false_alarm_counter + 1;
                    else
                        correct_rejection_counter = correct_rejection_counter + 1;
                    end

                    %% ----------------------------------------------------
                    


                    
                end                

                if(0)
                    smaller_local_conj_pss_fft = fft(smaller_local_time_domain_pss, NDFT3);
                    
                    poss = [2:2+(SRSLTE_PSS_LEN/2)-1, NDFT3-(SRSLTE_PSS_LEN/2)+1:NDFT3];
                    smaller_local_conj_pss_fft = smaller_local_conj_pss_fft(poss);

                    smaller_input_signal = input_signal(peak_pos-NDFT:peak_pos-1);

                    smaller_received_signal_fft = (fft(smaller_input_signal, NDFT3));
                    
                    poss = [2:2+(SRSLTE_PSS_LEN/2)-1, NDFT3-(SRSLTE_PSS_LEN/2)+1:NDFT3];
                    smaller_received_signal_fft = smaller_received_signal_fft(poss);

                    smaller_prod = smaller_received_signal_fft.*smaller_local_conj_pss_fft;

                    smaller_convolution = ifftshift(ifft(smaller_prod, SRSLTE_PSS_LEN));

                    smaller_power_conv = abs(smaller_convolution).^2;

                    [smaller_peak_value, smaller_peak_pos] = max(smaller_power_conv);
                end
                
                if(0)
                    smaller_local_conj_pss_fft = smaller_local_time_domain_pss;
                    
                    smaller_input_signal = input_signal(peak_pos-NDFT:peak_pos-1);
                    
                    smaller_received_signal_fft = smaller_input_signal;
                    
                    smaller_prod = smaller_received_signal_fft.*smaller_local_conj_pss_fft;
                    
                    smaller_convolution = ifftshift(ifft(smaller_prod, NDFT3));
                    
                    %smaller_convolution = abs(xcorr(smaller_received_signal_fft,smaller_local_conj_pss_fft)).^2;
                    
                    smaller_power_conv = abs(smaller_convolution).^2;
                    
                    [smaller_peak_value, smaller_peak_pos] = max(smaller_power_conv);
                end                
                
                if(smaller_peak_pos == (SRSLTE_PSS_LEN/2) + 1)
                    
                    pl_ub = smaller_peak_pos + 1;
                    while(smaller_power_conv(pl_ub + 1) <= smaller_power_conv(pl_ub))
                        pl_ub = pl_ub + 1;
                    end
                    
                    pl_lb = smaller_peak_pos - 1;
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
                    
                    smaller_psr_max = smaller_peak_value/side_lobe_value;
                    
                else
                    smaller_psr_max = 0;
                end
                
            else
                
                smaller_psr_max = 0;
                
            end
            
            
            

            
            if(psr_max >= smaller_threshold && smaller_psr_max >= threshold)
                if(peak_pos-1 >= (SUBFRAME_LENGTH(phy_bw_idx)/2)-3 && peak_pos-1 <= (SUBFRAME_LENGTH(phy_bw_idx)/2)+3)                    
                    correct_detection_with_data13(snr_idx) = correct_detection_with_data13(snr_idx) + 1;
                else
                    false_detection_with_data13(snr_idx) = false_detection_with_data13(snr_idx) + 1;
                    
                    false_detection_peak_pos13(snr_idx, false_detection_peak_pos_cnt) = peak_pos-1;
                    false_detection_peak_pos_cnt = false_detection_peak_pos_cnt + 1;
                end
                
                psr_avg13(snr_idx) = psr_avg13(snr_idx) + smaller_psr_max;
                psr_cnt13(snr_idx) = psr_cnt13(snr_idx) + 1;                 
            else
                miss_detection_with_data13(snr_idx) = miss_detection_with_data13(snr_idx) + 1;

                miss_detection_psr13(snr_idx, miss_detection_psr_cnt13(snr_idx)) = smaller_psr_max;
                miss_detection_psr_cnt13(snr_idx) = miss_detection_psr_cnt13(snr_idx) + 1;
            end
            
        end
        
    end
    
    correct_detection_with_data13 = correct_detection_with_data13./NUM_ITER;
    miss_detection_with_data13 = miss_detection_with_data13./NUM_ITER;
    false_detection_with_data13 = false_detection_with_data13./NUM_ITER;
    
    psr_avg13(snr_idx) = psr_avg13(snr_idx)./psr_cnt13(snr_idx);    
end

%% --------------------------------------------------------------------------
if(enable_ofdm_with_pss_plus_data14)
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
    
    correct_detection_with_data14 = zeros(1,length(SNR));
    miss_detection_with_data14 = zeros(1,length(SNR));
    false_detection_with_data14 = zeros(1,length(SNR));
    false_detection_peak_pos14 = zeros(length(SNR),NUM_ITER);
    miss_detection_psr14 = zeros(length(SNR), NUM_ITER);
    miss_detection_psr_cnt14 = ones(length(SNR));
    psr_avg14 = zeros(length(SNR));
    psr_cnt14 = zeros(length(SNR));     
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
            NDFT3 = NDFT;
            first_stage_threshold = 2.5; %2.2;
            second_stage_threshold = 3.5;
            if(psr_max >= first_stage_threshold && psr_max < second_stage_threshold && (peak_pos-NDFT) > 0)
                smaller_input_signal = input_signal(peak_pos-NDFT:peak_pos-1);
                
                smaller_received_signal_fft = fft(smaller_input_signal);
                
                pos_null = [1, (SRSLTE_PSS_LEN/2)+2:NDFT3-(SRSLTE_PSS_LEN/2) ];
                
                smaller_received_signal_fft(pos_null) = zeros(length(pos_null), 1);
                
                
                
                smaller_local_conj_pss_fft = fft(smaller_local_time_domain_pss, NDFT3);
                
                
                
                smaller_prod = smaller_received_signal_fft.*smaller_local_conj_pss_fft;
                
                smaller_convolution = ifftshift(ifft(smaller_prod, NDFT3));
                
                smaller_power_conv = abs(smaller_convolution).^2;
                
                
                
                
                
                [peak_value_2nd_stage, peak_pos_2nd_stage] = max(smaller_power_conv);
                
                if(peak_pos_2nd_stage == (NDFT3/2) + 1)

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
                    correct_detection_with_data14(snr_idx) = correct_detection_with_data14(snr_idx) + 1;
                else
                    false_detection_with_data14(snr_idx) = false_detection_with_data14(snr_idx) + 1;
                    
                    false_detection_peak_pos14(snr_idx, false_detection_peak_pos_cnt) = peak_pos-1;
                    false_detection_peak_pos_cnt = false_detection_peak_pos_cnt + 1;
                end
                
                psr_avg14(snr_idx) = psr_avg14(snr_idx) + psr_max;
                psr_cnt14(snr_idx) = psr_cnt14(snr_idx) + 1;                 
            else
                miss_detection_with_data14(snr_idx) = miss_detection_with_data14(snr_idx) + 1;

                miss_detection_psr14(snr_idx, miss_detection_psr_cnt14(snr_idx)) = psr_max;
                miss_detection_psr_cnt14(snr_idx) = miss_detection_psr_cnt14(snr_idx) + 1;
            end
            
        end
        
    end
    
    correct_detection_with_data14 = correct_detection_with_data14./NUM_ITER;
    miss_detection_with_data14 = miss_detection_with_data14./NUM_ITER;
    false_detection_with_data14 = false_detection_with_data14./NUM_ITER;
    
    psr_avg14(snr_idx) = psr_avg14(snr_idx)./psr_cnt14(snr_idx);    
end





    
if(enable_ofdm_with_pss_plus_data15)
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
    
    correct_detection_with_data15 = zeros(1,length(SNR));
    miss_detection_with_data15 = zeros(1,length(SNR));
    false_detection_with_data15 = zeros(1,length(SNR));
    false_detection_peak_pos15 = zeros(length(SNR),NUM_ITER);
    miss_detection_psr15 = zeros(length(SNR), NUM_ITER);
    miss_detection_psr_cnt15 = ones(length(SNR));
    psr_avg15 = zeros(length(SNR));
    psr_cnt15 = zeros(length(SNR));     
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
            NDFT3 = NDFT;
            NDFT4 = NDFT;
            first_stage_threshold = 2.5;
            second_stage_threshold = 3.5;
            if(psr_max >= first_stage_threshold && psr_max < second_stage_threshold && (peak_pos-NDFT) > 0)
                smaller_input_signal = input_signal(peak_pos-NDFT:peak_pos-1);
                
                smaller_received_signal_fft = fft(smaller_input_signal, NDFT4);
                
                %smaller_received_signal_fft = fft([smaller_input_signal; zeros(NDFT,1)], NDFT4);
                
                %pos_null = [1, (SRSLTE_PSS_LEN/2)+2:NDFT3-(SRSLTE_PSS_LEN/2) ];
                
                %smaller_received_signal_fft(pos_null) = zeros(length(pos_null), 1);

                %smaller_local_conj_pss_fft = fft([smaller_local_time_domain_pss; zeros(2*NDFT,1)], NDFT4);
                
                
                smaller_local_conj_pss_fft = fft(smaller_local_time_domain_pss, NDFT4);
                
                smaller_prod = smaller_received_signal_fft.*smaller_local_conj_pss_fft;
                
                smaller_convolution = ifftshift(ifft(smaller_prod, NDFT4));
                
                smaller_power_conv = abs(smaller_convolution).^2;
                
                
                
                
                
                [peak_value_2nd_stage, peak_pos_2nd_stage] = max(smaller_power_conv);
                
                if(peak_pos_2nd_stage == (NDFT3/2) + 1)

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
                    correct_detection_with_data15(snr_idx) = correct_detection_with_data15(snr_idx) + 1;
                else
                    false_detection_with_data15(snr_idx) = false_detection_with_data15(snr_idx) + 1;
                    
                    false_detection_peak_pos15(snr_idx, false_detection_peak_pos_cnt) = peak_pos-1;
                    false_detection_peak_pos_cnt = false_detection_peak_pos_cnt + 1;
                end
                
                psr_avg15(snr_idx) = psr_avg15(snr_idx) + psr_max;
                psr_cnt15(snr_idx) = psr_cnt15(snr_idx) + 1;                 
            else
                miss_detection_with_data15(snr_idx) = miss_detection_with_data15(snr_idx) + 1;

                miss_detection_psr15(snr_idx, miss_detection_psr_cnt15(snr_idx)) = psr_max;
                miss_detection_psr_cnt15(snr_idx) = miss_detection_psr_cnt15(snr_idx) + 1;
            end
            
        end
        
    end
    
    correct_detection_with_data15 = correct_detection_with_data15./NUM_ITER;
    miss_detection_with_data15 = miss_detection_with_data15./NUM_ITER;
    false_detection_with_data15 = false_detection_with_data15./NUM_ITER;
    
    psr_avg15(snr_idx) = psr_avg15(snr_idx)./psr_cnt15(snr_idx);    
end

if(enable_ofdm_with_pss_plus_data16)
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
            
            if(0)
                figure;
                plot(abs(fft(noisy_signal)).^2);
                figure;
                plot(abs(fft(local_conj_time_domain_pss)).^2);
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
            NDFT3 = NDFT;
            NDFT4 = 2*NDFT;
            first_stage_threshold = 2.3;%2.5;
            second_stage_threshold = 3.5;
            if(psr_max >= first_stage_threshold && psr_max < second_stage_threshold && (peak_pos-NDFT) > 0)
                smaller_input_signal = input_signal(peak_pos-NDFT:peak_pos-1);
                
                smaller_received_signal_fft = fft([smaller_input_signal; zeros(NDFT, 1)], 2*NDFT);
                
                smaller_local_conj_pss_fft = fft([smaller_local_time_domain_pss; zeros(NDFT, 1)], 2*NDFT);
                
                smaller_prod = smaller_received_signal_fft.*smaller_local_conj_pss_fft;
                
                smaller_convolution = (ifft(smaller_prod, 2*NDFT));
                
                smaller_power_conv = abs(smaller_convolution).^2;
                
                [peak_value_2nd_stage, peak_pos_2nd_stage] = max(smaller_power_conv);
                
                if(peak_pos_2nd_stage == (NDFT3 + 1) && peak_value_2nd_stage < 2.0)

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




if(0)
    % Get timestamp for saving files.
    timeStamp = datestr(now,30);
    fileName = sprintf('test_pss_correct_detection_versus_false_detectionv1_%s.mat',timeStamp);
    save(fileName);
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

if(enable_ofdm_with_pss_plus_data2)
    plot_cnt = plot_cnt + 1;
    subplot(nof_plots, 1, plot_cnt);
    plot(SNR, correct_detection_with_data2);
    hold on;
    plot(SNR, miss_detection_with_data2);
    plot(SNR, false_detection_with_data2);
    grid on;
    xlabel('SNR [dB]')
    legend('Correct detection', 'Miss detection', 'False detection');
    titleStr = sprintf('OFDM Symbol with PSS + Data + Filter(%d) - Averaged-PSR-based Threshold', L);
    title(titleStr);
    hold off;
end

if(enable_ofdm_with_pss_plus_data3)
    plot_cnt = plot_cnt + 1;
    subplot(nof_plots, 1, plot_cnt);
    plot(SNR, correct_detection_with_data3);
    hold on;
    plot(SNR, miss_detection_with_data3);
    plot(SNR, false_detection_with_data3);
    grid on;
    xlabel('SNR [dB]');
    legend('Correct detection', 'Miss detection', 'False detection');
    title('OFDM Symbol with PSS + Data - Averaged-PSR-based Threshold');
    hold off;
end

if(enable_ofdm_with_pss_plus_data4)
    plot_cnt = plot_cnt + 1;
    subplot(nof_plots, 1, plot_cnt);
    plot(SNR, correct_detection_with_data4);
    hold on;
    plot(SNR, miss_detection_with_data4);
    plot(SNR, false_detection_with_data4);
    grid on;
    xlabel('SNR [dB]');
    legend('Correct detection', 'Miss detection', 'False detection');
    title('OFDM Symbol with PSS + Data - Min-value-PSR-based Threshold');
    hold off;
end

if(enable_ofdm_with_pss_plus_data5)
    plot_cnt = plot_cnt + 1;
    subplot(nof_plots, 1, plot_cnt);
    plot(SNR, correct_detection_with_data5);
    hold on;
    plot(SNR, miss_detection_with_data5);
    plot(SNR, false_detection_with_data5);
    grid on;
    xlabel('SNR [dB]');
    legend('Correct detection', 'Miss detection', 'False detection');
    titleStr = sprintf('OFDM Symbol with PSS + Data - Averaged-PSR-based Threshold & Longer PSS (%d)', SCATTER_PSS_LEN5);
    title(titleStr);
    hold off;
end

if(enable_ofdm_with_pss_plus_data6)
    plot_cnt = plot_cnt + 1;
    subplot(nof_plots, 1, plot_cnt);
    plot(SNR, correct_detection_with_data6);
    hold on;
    plot(SNR, miss_detection_with_data6);
    plot(SNR, false_detection_with_data6);
    grid on;
    xlabel('SNR [dB]');
    legend('Correct detection', 'Miss detection', 'False detection');
    titleStr = sprintf('OFDM Symbol with PSS + Data - Averaged-PSR-based Threshold & Longer PSS (%d)', SCATTER_PSS_LEN6);
    title(titleStr);
    hold off;
end

if(enable_ofdm_with_pss_plus_data7)
    plot_cnt = plot_cnt + 1;
    subplot(nof_plots, 1, plot_cnt);
    plot(SNR, correct_detection_with_data7);
    hold on;
    plot(SNR, miss_detection_with_data7);
    plot(SNR, false_detection_with_data7);
    grid on;
    xlabel('SNR [dB]');
    legend('Correct detection', 'Miss detection', 'False detection');
    titleStr = sprintf('OFDM Symbol with PSS + Data - Averaged-PSR-based Threshold & Longer PSS (%d)', SCATTER_PSS_LEN7);
    title(titleStr);
    hold off;
end

if(enable_ofdm_with_pss_plus_data8)
    plot_cnt = plot_cnt + 1;
    subplot(nof_plots, 1, plot_cnt);
    plot(SNR, correct_detection_with_data8);
    hold on;
    plot(SNR, miss_detection_with_data8);
    plot(SNR, false_detection_with_data8);
    grid on;
    xlabel('SNR [dB]');
    legend('Correct detection', 'Miss detection', 'False detection');
    titleStr = sprintf('OFDM Symbol with PSS + Data - Averaged-PSR-based Threshold & Longer PSS (%d)', SCATTER_PSS_LEN8);
    title(titleStr);
    hold off;
end

if(enable_ofdm_with_pss_plus_data9)
    plot_cnt = plot_cnt + 1;
    subplot(nof_plots, 1, plot_cnt);
    plot(SNR, correct_detection_with_data9);
    hold on;
    plot(SNR, miss_detection_with_data9);
    plot(SNR, false_detection_with_data9);
    grid on;
    xlabel('SNR [dB]');
    legend('Correct detection', 'Miss detection', 'False detection');
    titleStr = sprintf('OFDM Symbol with PSS + Data - Averaged-PSR-based Threshold & Longer PSS (%d)', SCATTER_PSS_LEN9);
    title(titleStr);
    hold off;
end

if(enable_ofdm_with_pss_plus_data10)
    plot_cnt = plot_cnt + 1;
    subplot(nof_plots, 1, plot_cnt);
    plot(SNR, correct_detection_with_data10);
    hold on;
    plot(SNR, miss_detection_with_data10);
    plot(SNR, false_detection_with_data10);
    grid on;
    xlabel('SNR [dB]');
    legend('Correct detection', 'Miss detection', 'False detection');
    titleStr = sprintf('OFDM Symbol with PSS + Data - Averaged-PSR-based Threshold & Longer PSS (%d)', SCATTER_PSS_LEN10);
    title(titleStr);
    hold off;
end

if(enable_ofdm_with_pss_plus_data11)
    plot_cnt = plot_cnt + 1;
    subplot(nof_plots, 1, plot_cnt);
    plot(SNR, correct_detection_with_data11);
    hold on;
    plot(SNR, miss_detection_with_data11);
    plot(SNR, false_detection_with_data11);
    grid on;
    xlabel('SNR [dB]');
    legend('Correct detection', 'Miss detection', 'False detection');
    titleStr = sprintf('OFDM Symbol with PSS + Data - finer-grained corr');
    title(titleStr);
    hold off;
end

if(enable_ofdm_with_pss_plus_data12)
    plot_cnt = plot_cnt + 1;
    subplot(nof_plots, 1, plot_cnt);
    plot(SNR, correct_detection_with_data12);
    hold on;
    plot(SNR, miss_detection_with_data12);
    plot(SNR, false_detection_with_data12);
    grid on;
    xlabel('SNR [dB]');
    legend('Correct detection', 'Miss detection', 'False detection');
    titleStr = sprintf('OFDM Symbol with PSS + Data - finer-grained corr and smaller threshold');
    title(titleStr);
    hold off;
end

if(enable_ofdm_with_pss_plus_data13)
    plot_cnt = plot_cnt + 1;
    subplot(nof_plots, 1, plot_cnt);
    plot(SNR, correct_detection_with_data13);
    hold on;
    plot(SNR, miss_detection_with_data13);
    plot(SNR, false_detection_with_data13);
    grid on;
    xlabel('SNR [dB]');
    legend('Correct detection', 'Miss detection', 'False detection');
    titleStr = sprintf('OFDM Symbol with PSS + Data - finer-grained corr and smaller threshold');
    title(titleStr);
    hold off;
end

if(enable_ofdm_with_pss_plus_data14)
    plot_cnt = plot_cnt + 1;
    subplot(nof_plots, 1, plot_cnt);
    plot(SNR, correct_detection_with_data14);
    hold on;
    plot(SNR, miss_detection_with_data14);
    plot(SNR, false_detection_with_data14);
    grid on;
    xlabel('SNR [dB]');
    legend('Correct detection', 'Miss detection', 'False detection');
    titleStr = sprintf('OFDM Symbol with PSS + Data - finer-grained corr and smaller threshold');
    title(titleStr);
    hold off;
end

if(enable_ofdm_with_pss_plus_data15)
    plot_cnt = plot_cnt + 1;
    subplot(nof_plots, 1, plot_cnt);
    plot(SNR, correct_detection_with_data15);
    hold on;
    plot(SNR, miss_detection_with_data15);
    plot(SNR, false_detection_with_data15);
    grid on;
    xlabel('SNR [dB]');
    legend('Correct detection', 'Miss detection', 'False detection');
    titleStr = sprintf('OFDM Symbol with PSS + Data - finer-grained corr and smaller threshold');
    title(titleStr);
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
    titleStr = sprintf('OFDM Symbol with PSS + Data - finer-grained corr and smaller threshold');
    title(titleStr);
    hold off;
end

% for snr_idx = 1:1:length(SNR)
%     fprintf(1, 'SNR: %1.2f - 1: %1.4f - 3: %1.4f - 5: %1.4f - 6: %1.4f - 7: %1.4f - 8: %1.4f - 9: %1.4f - 10: %1.4f\n', SNR(snr_idx), psr_avg1(snr_idx), psr_avg3(snr_idx), psr_avg5(snr_idx), psr_avg6(snr_idx), psr_avg7(snr_idx), psr_avg8(snr_idx), psr_avg9(snr_idx), psr_avg10(snr_idx));
% end