clear all;close all;clc

phy_bw_idx = 3;

plot_figures = 0;

add_noise = 1;

NUM_ITER = 1e4;

noise_variance = 0.001:0.1:20; %linspace(0.001,10,100);

SUBFRAME_LENGTH = [1920 3840 5760 11520 15360 23040];
numFFT = [128 256 384 768 1024 1536];       % Number of FFT points
deltaF = 15000;                             % Subcarrier spacing
numRBs = [6 15 25 50 75 100];               % Number of resource blocks
rbSize = 12;                                % Number of subcarriers per resource block
cpLen_1st_symb  =   [10 20 30 60 80 120];   % Cyclic prefix length in samples only for very 1st symbol.
cpLen_other_symbs = [9 18 27 54 72 108];

cell_id = 0;
NDFT = numFFT(phy_bw_idx); % For 5 MHz PHY BW.
delta_f = 15e3; % subcarrier spacing in Hz.
USEFUL_SUBCARRIERS = 300;
PSS_GUARD_BAND = 10; % in number of subcarriers.
SRSLTE_PSS_LEN = 62;
FRAME_SIZE = SUBFRAME_LENGTH(phy_bw_idx); % For 5 MHz PHY BW.
NDFT2 = FRAME_SIZE + NDFT;

enable_ofdm_with_pss_plus_data        = true;
enable_ofdm_with_pss_plus_data_filter = false;
enable_ofdm_with_pss_plus_data2       = false;
enable_ofdm_with_pss_plus_data3       = false;
enable_ofdm_with_pss_plus_data4       = false;
enable_ofdm_with_pss_plus_data5       = false;
enable_ofdm_with_pss_plus_data6       = false;
enable_ofdm_with_pss_plus_data7       = false;
enable_ofdm_with_pss_plus_data8       = false;
enable_ofdm_with_pss_plus_data9       = true;

threshold = 3.0;

%% --------------------------------------------------------------------------
if(enable_ofdm_with_pss_plus_data)
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
    
    correct_detection_with_data = zeros(1,length(noise_variance));
    correct_miss_with_data = zeros(1,length(noise_variance));
    false_detection_with_data = zeros(1,length(noise_variance));
    false_detection_peak_pos = zeros(length(noise_variance),NUM_ITER);
    rng(12041988);
    for snr_idx = 1:1:length(noise_variance)
        
        false_detection_peak_pos_cnt = 1;
        for n = 1:1:NUM_ITER
            
            if(add_noise == 1)
                noisy_signal = sqrt(noise_variance(snr_idx))*(1/sqrt(2))*complex(randn(SUBFRAME_LENGTH(phy_bw_idx),1),randn(SUBFRAME_LENGTH(phy_bw_idx),1));
            else
                noisy_signal = complex(zeros(SUBFRAME_LENGTH(phy_bw_idx),1),zeros(SUBFRAME_LENGTH(phy_bw_idx),1));
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
                false_detection_with_data(snr_idx) = false_detection_with_data(snr_idx) + 1;
            else
                correct_miss_with_data(snr_idx) = correct_miss_with_data(snr_idx) + 1;
            end
            
        end
        
    end
    
    false_detection_with_data = false_detection_with_data./NUM_ITER;
    correct_miss_with_data = correct_miss_with_data./NUM_ITER;
end

%% --------------------------------------------------------------------------
if(enable_ofdm_with_pss_plus_data_filter)
    
    % Calculate sampling rate.
    sampling_rate = NDFT*delta_f;
    
%     % Create FIR filter for improved PSS detection.
%     FIR_ORDER = 128;
%     pss_filter = fir1(FIR_ORDER, ((PSS_GUARD_BAND+SRSLTE_PSS_LEN)*delta_f)/sampling_rate).';
%     %pss_filter = fir1(FIR_ORDER, (1.4e6)/sampling_rate).';
%     %freqz(fir_coef,1,512)

    if(1)
        toneOffset = 2.5;        % Tone offset or excess bandwidth (in subcarriers)
        FIR_ORDER = 32;                 % Filter length (=filterOrder+1), odd

        numDataCarriers = 72;    % number of data subcarriers in sub-band
        halfFilt = floor(FIR_ORDER/2);
        n = -halfFilt:halfFilt;

        % Sinc function prototype filter
        pb = sinc((numDataCarriers+2*toneOffset).*n./NDFT);

        % Sinc truncation window
        w = (0.5*(1+cos(2*pi.*n/(FIR_ORDER-1)))).^0.6;

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
    
    correct_miss_with_data_filter = zeros(1,length(noise_variance));
    false_detection_with_data_filter = zeros(1,length(noise_variance));
    rng(12041988);
    for snr_idx = 1:1:length(noise_variance)
        
        for n = 1:1:NUM_ITER

            if(add_noise == 1)
                noisy_signal = sqrt(noise_variance(snr_idx))*(1/sqrt(2))*complex(randn(SUBFRAME_LENGTH(phy_bw_idx),1),randn(SUBFRAME_LENGTH(phy_bw_idx),1));
            else
                noisy_signal = complex(zeros(SUBFRAME_LENGTH(phy_bw_idx),1),zeros(SUBFRAME_LENGTH(phy_bw_idx),1));
            end
            
            % Filter the received signal.
            filtered_noisy_signal = filter(pss_filter, 1, noisy_signal);
            
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

            if(psr >= threshold)
                false_detection_with_data_filter(snr_idx) = false_detection_with_data_filter(snr_idx) + 1;
            else
                correct_miss_with_data_filter(snr_idx) = correct_miss_with_data_filter(snr_idx) + 1;
            end
            
        end
        
    end
    
    correct_miss_with_data_filter = correct_miss_with_data_filter./NUM_ITER;
    false_detection_with_data_filter = false_detection_with_data_filter./NUM_ITER;
end

%% --------------------------------------------------------------------------
if(enable_ofdm_with_pss_plus_data2)
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
    
    correct_miss_with_data2 = zeros(1,length(noise_variance));
    false_detection_with_data2 = zeros(1,length(noise_variance));
    rng(12041988);
    for snr_idx = 1:1:length(noise_variance)
        
        for n = 1:1:NUM_ITER
            
            if(add_noise == 1)
                noisy_signal = sqrt(noise_variance(snr_idx))*(1/sqrt(2))*complex(randn(SUBFRAME_LENGTH(phy_bw_idx),1), randn(SUBFRAME_LENGTH(phy_bw_idx),1));
            else
                noisy_signal = complex(zeros(SUBFRAME_LENGTH(phy_bw_idx),1), zeros(SUBFRAME_LENGTH(phy_bw_idx),1));
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
                false_detection_with_data2(snr_idx) = false_detection_with_data2(snr_idx) + 1;
            else
                correct_miss_with_data2(snr_idx) = correct_miss_with_data2(snr_idx) + 1;
            end

        end
        
    end
    
    false_detection_with_data2 = false_detection_with_data2./NUM_ITER;
    correct_miss_with_data2 = correct_miss_with_data2./NUM_ITER;
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
    
    correct_miss_with_data3 = zeros(1,length(noise_variance));
    false_detection_with_data3 = zeros(1,length(noise_variance));
    rng(12041988);
    for snr_idx = 1:1:length(noise_variance)
        
        for n = 1:1:NUM_ITER
            
            if(add_noise == 1)
                noisy_signal = sqrt(noise_variance(snr_idx))*(1/sqrt(2))*complex(randn(SUBFRAME_LENGTH(phy_bw_idx),1), randn(SUBFRAME_LENGTH(phy_bw_idx),1));
            else
                noisy_signal = complex(zeros(SUBFRAME_LENGTH(phy_bw_idx),1), zeros(SUBFRAME_LENGTH(phy_bw_idx),1));
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
            
            if(psr_min >= threshold)
                false_detection_with_data3(snr_idx) = false_detection_with_data3(snr_idx) + 1;
            else
                correct_miss_with_data3(snr_idx) = correct_miss_with_data3(snr_idx) + 1;
            end

        end
        
    end
    
    false_detection_with_data3 = false_detection_with_data3./NUM_ITER;
    correct_miss_with_data3 = correct_miss_with_data3./NUM_ITER;
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
    
    correct_miss_with_data4 = zeros(1,length(noise_variance));
    false_detection_with_data4 = zeros(1,length(noise_variance));
    rng(12041988);
    for snr_idx = 1:1:length(noise_variance)
        
        for n = 1:1:NUM_ITER
            
            if(add_noise == 1)
                noisy_signal = sqrt(noise_variance(snr_idx))*(1/sqrt(2))*complex(randn(SUBFRAME_LENGTH(phy_bw_idx),1), randn(SUBFRAME_LENGTH(phy_bw_idx),1));
            else
                noisy_signal = complex(zeros(SUBFRAME_LENGTH(phy_bw_idx),1), zeros(SUBFRAME_LENGTH(phy_bw_idx),1));
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
            
            abs_average = sum(power_conv)/length(power_conv);
            
            variable_threshold = peak_value/abs_average;

            
            if(peak_value >= variable_threshold)
                false_detection_with_data4(snr_idx) = false_detection_with_data4(snr_idx) + 1;
            else
                correct_miss_with_data4(snr_idx) = correct_miss_with_data4(snr_idx) + 1;
            end

        end
        
    end
    
    false_detection_with_data4 = false_detection_with_data4./NUM_ITER;
    correct_miss_with_data4 = correct_miss_with_data4./NUM_ITER;
end



if(0)

if(enable_ofdm_with_pss_plus_data5)
    
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
    
    
    correct_miss_with_data5 = zeros(1,length(noise_variance));
    false_detection_with_data5 = zeros(1,length(noise_variance));
    miss_detection_with_data5 = zeros(1,length(noise_variance));
    false_detection_peak_pos5 = zeros(length(noise_variance),NUM_ITER);
    miss_detection_psr5 = zeros(length(noise_variance), NUM_ITER);
    miss_detection_psr_cnt5 = ones(length(noise_variance));
    psr_avg5 = zeros(length(noise_variance));
    psr_cnt5 = zeros(length(noise_variance));     
    
    psr_max_vector = zeros(1, NUM_ITER);
 
    smaller_psr_max_vector = zeros(1, NUM_ITER);   
    
    rng(12041988);
    for snr_idx = 1:1:length(noise_variance)
        
        false_detection_peak_pos_cnt = 1;
        for n = 1:1:NUM_ITER
            
            if(add_noise == 1)
                noisy_signal = sqrt(noise_variance(snr_idx))*(1/sqrt(2))*complex(randn(SUBFRAME_LENGTH(phy_bw_idx),1), randn(SUBFRAME_LENGTH(phy_bw_idx),1));
            else
                noisy_signal = complex(zeros(SUBFRAME_LENGTH(phy_bw_idx),1), zeros(SUBFRAME_LENGTH(phy_bw_idx),1));
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
                
                aux = input_signal(peak_pos-NDFT:peak_pos-1);
                r = xcorr((local_time_domain_pss),aux);
                
                smaller_local_conj_pss_fft = fft(smaller_local_time_domain_pss, NDFT3);
                
                smaller_input_signal = [input_signal(peak_pos-2*NDFT:peak_pos-1); zeros(NDFT,1)];
                
                %smaller_input_signal = [input_signal(peak_pos-2*NDFT:peak_pos+NDFT-1)];
                
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
            
            
            psr_max_vector(n) = psr_max;
 
            smaller_psr_max_vector(n) = smaller_psr_max;
            
            threshold = 3.5;
            
            smaller_threshold = 2.4;
            
            if((psr_max) >= smaller_threshold && (smaller_psr_max) >= threshold)
                false_detection_with_data5(snr_idx) = false_detection_with_data5(snr_idx) + 1;
            else
                correct_miss_with_data5(snr_idx) = correct_miss_with_data5(snr_idx) + 1;
            end
            

     
            
            
            
        end
        
    end
    
    false_detection_with_data5 = false_detection_with_data5./NUM_ITER;
    correct_miss_with_data5 = correct_miss_with_data5./NUM_ITER;
    
    psr_avg5(snr_idx) = psr_avg5(snr_idx)./psr_cnt5(snr_idx);    
end

end





if(enable_ofdm_with_pss_plus_data6)
    
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
    
    
    NDFT3 = NDFT;
    
    smaller_local_time_domain_pss = conj(local_time_domain_pss);    
    
  
    
    
    correct_miss_with_data6 = zeros(1,length(noise_variance));
    false_detection_with_data6 = zeros(1,length(noise_variance));
    psr_avg6 = zeros(length(noise_variance));
    psr_cnt6 = zeros(length(noise_variance));     
    
    psr_max_vector6 = zeros(1, NUM_ITER);
 
    smaller_psr_max_vector6 = zeros(1, NUM_ITER);   
    correct_rejection_counter = 0;
    false_alarm_counter = 0;
    rng(12041988);
    for snr_idx = 1:1:length(noise_variance)
        
        false_detection_peak_pos_cnt = 1;
        for n = 1:1:NUM_ITER
            
            if(add_noise == 1)
                noisy_signal = sqrt(noise_variance(snr_idx))*(1/sqrt(2))*complex(randn(SUBFRAME_LENGTH(phy_bw_idx),1), randn(SUBFRAME_LENGTH(phy_bw_idx),1));
            else
                noisy_signal = complex(zeros(SUBFRAME_LENGTH(phy_bw_idx),1), zeros(SUBFRAME_LENGTH(phy_bw_idx),1));
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
                
                %aux = input_signal(peak_pos-NDFT:peak_pos-1);
                %r = xcorr((local_time_domain_pss),aux);
                
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
                    
                    Tcme = 3.5;%6.9078; % for 1 sample.
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
                    alpha = finv(1-Pfa,(2*1),(2*I*1))/I;  
                    
                    
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

                    smaller_input_signal = input_signal(peak_pos-NDFT:peak_pos-1);

                    smaller_received_signal_fft = fft(smaller_input_signal, NDFT3);

                    smaller_prod = smaller_received_signal_fft.*smaller_local_conj_pss_fft;

                    smaller_convolution = ifftshift(ifft(smaller_prod, NDFT3));

                    smaller_power_conv = abs(smaller_convolution).^2;

                    [smaller_peak_value, smaller_peak_pos] = max(smaller_power_conv);
                end
                
                
                
                if(0)
                    
                    smaller_local_conj_pss_fft = smaller_local_time_domain_pss;
                    
                    smaller_input_signal = input_signal(peak_pos-NDFT:peak_pos-1);
                    
                    smaller_received_signal_fft = smaller_input_signal;
                    
                    smaller_prod = smaller_received_signal_fft.*smaller_local_conj_pss_fft;
                    
                    smaller_convolution = ifftshift(ifft(smaller_prod, NDFT3));
                    
                    smaller_power_conv = abs(smaller_convolution).^2;
                    
                    [smaller_peak_value, smaller_peak_pos] = max(smaller_power_conv);
                    
                end
                
                %fprintf(1,'smaller_peak_pos: %d\n',smaller_peak_pos);
                
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
                
                smaller_ratio = 0;
                
                alpha = Inf;
                
            end
            
            
            psr_max_vector6(n) = psr_max;
 
            smaller_psr_max_vector6(n) = smaller_psr_max;
            
%             if(psr_max >= smaller_threshold && smaller_psr_max >= threshold)
%                 false_detection_with_data6(snr_idx) = false_detection_with_data6(snr_idx) + 1;
%             else
%                 correct_miss_with_data6(snr_idx) = correct_miss_with_data6(snr_idx) + 1;
%             end

            if(psr_max >= smaller_threshold && smaller_ratio >= alpha)
                false_detection_with_data6(snr_idx) = false_detection_with_data6(snr_idx) + 1;
            else
                correct_miss_with_data6(snr_idx) = correct_miss_with_data6(snr_idx) + 1;
            end
            

     
            
            
            
        end
        
    end
    
    false_detection_with_data6 = false_detection_with_data6./NUM_ITER;
    correct_miss_with_data6 = correct_miss_with_data6./NUM_ITER;
    
    psr_avg6(snr_idx) = psr_avg6(snr_idx)./psr_cnt6(snr_idx);    
end

%% --------------------------------------------------------------------------
if(enable_ofdm_with_pss_plus_data7)
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
    
    correct_detection_with_data7 = zeros(1,length(noise_variance));
    miss_detection_with_data7 = zeros(1,length(noise_variance));
    false_detection_with_data7 = zeros(1,length(noise_variance));
    correct_miss_with_data7 = zeros(1,length(noise_variance));
    false_detection_peak_pos7 = zeros(length(noise_variance),NUM_ITER);
    miss_detection_psr7 = zeros(length(noise_variance), NUM_ITER);
    miss_detection_psr_cnt7 = ones(length(noise_variance));
    psr_avg7 = zeros(length(noise_variance));
    psr_cnt7 = zeros(length(noise_variance));     
    rng(12041988);
    for snr_idx = 1:1:length(noise_variance)
        
        false_detection_peak_pos_cnt = 1;
        for n = 1:1:NUM_ITER
            
            if(add_noise == 1)
                noisy_signal = sqrt(noise_variance(snr_idx))*(1/sqrt(2))*complex(randn(SUBFRAME_LENGTH(phy_bw_idx),1), randn(SUBFRAME_LENGTH(phy_bw_idx),1));
            else
                noisy_signal = complex(zeros(SUBFRAME_LENGTH(phy_bw_idx),1), zeros(SUBFRAME_LENGTH(phy_bw_idx),1));
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
            first_stage_threshold = 2.5;
            second_stage_threshold = 3.5;
            if(psr_max >= first_stage_threshold && psr_max < second_stage_threshold && (peak_pos-NDFT) > 0)
            %if(1)
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
                false_detection_with_data7(snr_idx) = false_detection_with_data7(snr_idx) + 1;
            else
                correct_miss_with_data7(snr_idx) = correct_miss_with_data7(snr_idx) + 1;
            end
            
        end
        
    end
    
    correct_detection_with_data7 = correct_detection_with_data7./NUM_ITER;
    miss_detection_with_data7 = miss_detection_with_data7./NUM_ITER;
    false_detection_with_data7 = false_detection_with_data7./NUM_ITER;
    correct_miss_with_data7 = correct_miss_with_data7./NUM_ITER;
    
    psr_avg7(snr_idx) = psr_avg7(snr_idx)./psr_cnt7(snr_idx);    
end

%% --------------------------------------------------------------------------
if(enable_ofdm_with_pss_plus_data8)
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
    
    correct_detection_with_data8 = zeros(1,length(noise_variance));
    miss_detection_with_data8 = zeros(1,length(noise_variance));
    false_detection_with_data8 = zeros(1,length(noise_variance));
    correct_miss_with_data8 = zeros(1,length(noise_variance));
    false_detection_peak_pos8 = zeros(length(noise_variance),NUM_ITER);
    miss_detection_psr8 = zeros(length(noise_variance), NUM_ITER);
    miss_detection_psr_cnt8 = ones(length(noise_variance));
    psr_avg8 = zeros(length(noise_variance));
    psr_cnt8 = zeros(length(noise_variance));     
    rng(12041988);
    for snr_idx = 1:1:length(noise_variance)
        
        false_detection_peak_pos_cnt = 1;
        for n = 1:1:NUM_ITER
            
            if(add_noise == 1)
                noisy_signal = sqrt(noise_variance(snr_idx))*(1/sqrt(2))*complex(randn(SUBFRAME_LENGTH(phy_bw_idx),1), randn(SUBFRAME_LENGTH(phy_bw_idx),1));
            else
                noisy_signal = complex(zeros(SUBFRAME_LENGTH(phy_bw_idx),1), zeros(SUBFRAME_LENGTH(phy_bw_idx),1));
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
            first_stage_threshold = 2.5;
            second_stage_threshold = 3.5;
            if(psr_max >= first_stage_threshold && psr_max < second_stage_threshold && (peak_pos-NDFT) > 0)
            %if(1)
                smaller_input_signal = input_signal(peak_pos-NDFT:peak_pos-1);
                
                smaller_received_signal_fft = fft(smaller_input_signal);
                
                %pos_null = [1, (SRSLTE_PSS_LEN/2)+2:NDFT3-(SRSLTE_PSS_LEN/2) ];
                
                %smaller_received_signal_fft(pos_null) = zeros(length(pos_null), 1);
                
                
                
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
                false_detection_with_data8(snr_idx) = false_detection_with_data8(snr_idx) + 1;
            else
                correct_miss_with_data8(snr_idx) = correct_miss_with_data8(snr_idx) + 1;
            end
            
        end
        
    end
    
    correct_detection_with_data8 = correct_detection_with_data8./NUM_ITER;
    miss_detection_with_data8 = miss_detection_with_data8./NUM_ITER;
    false_detection_with_data8 = false_detection_with_data8./NUM_ITER;
    correct_miss_with_data8 = correct_miss_with_data8./NUM_ITER;
    
    psr_avg8(snr_idx) = psr_avg8(snr_idx)./psr_cnt8(snr_idx);    
end






%% --------------------------------------------------------------------------
if(enable_ofdm_with_pss_plus_data9)
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
    
    peak_value_2nd_stage_vector = [];
    correct_detection_with_data9 = zeros(1,length(noise_variance));
    miss_detection_with_data9 = zeros(1,length(noise_variance));
    false_detection_with_data9 = zeros(1,length(noise_variance));
    correct_miss_with_data9 = zeros(1,length(noise_variance));
    false_detection_peak_pos9 = zeros(length(noise_variance),NUM_ITER);
    miss_detection_psr9 = zeros(length(noise_variance), NUM_ITER);
    miss_detection_psr_cnt9 = ones(length(noise_variance));
    psr_avg9 = zeros(length(noise_variance));
    psr_cnt9 = zeros(length(noise_variance));     
    rng(12041988);
    for snr_idx = 1:1:length(noise_variance)
        
        false_detection_peak_pos_cnt = 1;
        for n = 1:1:NUM_ITER
            
            if(add_noise == 1)
                noisy_signal = sqrt(noise_variance(snr_idx))*(1/sqrt(2))*complex(randn(SUBFRAME_LENGTH(phy_bw_idx),1), randn(SUBFRAME_LENGTH(phy_bw_idx),1));
            else
                noisy_signal = complex(zeros(SUBFRAME_LENGTH(phy_bw_idx),1), zeros(SUBFRAME_LENGTH(phy_bw_idx),1));
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
            first_stage_threshold = 2.5;%2.5;
            second_stage_threshold = 3.5;
            if(psr_max >= first_stage_threshold && psr_max < second_stage_threshold && (peak_pos-NDFT) > 0)
            %if(1)
                smaller_input_signal = input_signal(peak_pos-NDFT:peak_pos-1);
                
                smaller_received_signal_fft = fft([smaller_input_signal; zeros(NDFT, 1)], 2*NDFT);
                
                smaller_local_conj_pss_fft = fft([smaller_local_time_domain_pss; zeros(NDFT, 1)], 2*NDFT);
                
                smaller_prod = smaller_received_signal_fft.*smaller_local_conj_pss_fft;
                
                smaller_convolution = (ifft(smaller_prod, 2*NDFT));
                
                smaller_power_conv = abs(smaller_convolution).^2;
                
                [peak_value_2nd_stage, peak_pos_2nd_stage] = max(smaller_power_conv);
                
                peak_value_2nd_stage_vector = [peak_value_2nd_stage_vector peak_value_2nd_stage];
                
                
                if(peak_pos_2nd_stage == (NDFT + 1) && peak_value_2nd_stage < 2.0)

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
                false_detection_with_data9(snr_idx) = false_detection_with_data9(snr_idx) + 1;
            else
                correct_miss_with_data9(snr_idx) = correct_miss_with_data9(snr_idx) + 1;
            end
            
        end
        
    end
    
    correct_detection_with_data9 = correct_detection_with_data9./NUM_ITER;
    miss_detection_with_data9 = miss_detection_with_data9./NUM_ITER;
    false_detection_with_data9 = false_detection_with_data9./NUM_ITER;
    correct_miss_with_data9 = correct_miss_with_data9./NUM_ITER;
    
    psr_avg9(snr_idx) = psr_avg9(snr_idx)./psr_cnt9(snr_idx);    
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
if(enable_ofdm_with_pss_plus_data)
    plot_cnt = plot_cnt + 1;
    subplot(nof_plots,1,plot_cnt);
    yyaxis left
    plot(10*log10(noise_variance), correct_miss_with_data, 'LineWidth', 1);
    ylabel('Correct miss');
    hold on;
    yyaxis right
    plot(10*log10(noise_variance), false_detection_with_data, 'LineWidth', 1);
    ylabel('False detection');
    grid on;
    xlabel('Noise power [dBW]')
    title('OFDM Symbol with PSS + Data - Legacy PSR-based Threshold');
    hold off;
end

if(enable_ofdm_with_pss_plus_data_filter)
    plot_cnt = plot_cnt + 1;
    subplot(nof_plots,1,plot_cnt);
    yyaxis left
    plot(10*log10(noise_variance), correct_miss_with_data_filter, 'LineWidth', 1);
    ylabel('Correct miss');
    hold on;
    yyaxis right
    plot(10*log10(noise_variance), false_detection_with_data_filter, 'LineWidth', 1);
    ylabel('False detection');
    grid on;
    xlabel('SNR [dB]')
    titleStr = sprintf('OFDM Symbol with PSS + Data + Filter(%d) - Averaged-PSR-based Threshold', FIR_ORDER);
    title(titleStr);
    hold off;
end

if(enable_ofdm_with_pss_plus_data2)
    plot_cnt = plot_cnt + 1;
    subplot(nof_plots,1,plot_cnt);
    yyaxis left
    plot(10*log10(noise_variance), correct_miss_with_data2, 'LineWidth', 1);
    ylabel('Correct miss');
    hold on;
    yyaxis right
    plot(10*log10(noise_variance), false_detection_with_data2, 'LineWidth', 1);
    ylabel('False detection');
    grid on;
    xlabel('Noise power [dBW]')
    title('OFDM Symbol with PSS + Data - Averaged-PSR-based Threshold');
    hold off;
end

if(enable_ofdm_with_pss_plus_data3)
    plot_cnt = plot_cnt + 1;
    subplot(nof_plots,1,plot_cnt);
    yyaxis left
    plot(10*log10(noise_variance), correct_miss_with_data3, 'LineWidth', 1);
    ylabel('Correct miss');
    hold on;
    yyaxis right
    plot(10*log10(noise_variance), false_detection_with_data3, 'LineWidth', 1);
    ylabel('False detection');
    grid on;
    xlabel('Noise power [dBW]')
    title('OFDM Symbol with PSS + Data - Min-value-PSR-based Threshold');
    hold off;
end

if(enable_ofdm_with_pss_plus_data4)
    plot_cnt = plot_cnt + 1;
    subplot(nof_plots,1,plot_cnt);
    yyaxis left
    plot(10*log10(noise_variance), correct_miss_with_data4, 'LineWidth', 1);
    ylabel('Correct miss');
    hold on;
    yyaxis right
    plot(10*log10(noise_variance), false_detection_with_data4, 'LineWidth', 1);
    ylabel('False detection');
    grid on;
    xlabel('Noise power [dBW]')
    title('OFDM Symbol with PSS + Data - Min-value-PSR-based Threshold');
    hold off;
end

if(enable_ofdm_with_pss_plus_data5)
    plot_cnt = plot_cnt + 1;
    subplot(nof_plots,1,plot_cnt);
    yyaxis left
    plot(10*log10(noise_variance), correct_miss_with_data5, 'LineWidth', 1);
    ylabel('Correct miss');
    hold on;
    yyaxis right
    plot(10*log10(noise_variance), false_detection_with_data5, 'LineWidth', 1);
    ylabel('False detection');
    grid on;
    xlabel('Noise power [dBW]')
    title('OFDM Symbol with PSS + Data - Min-value-PSR-based Threshold');
    hold off;
end

if(enable_ofdm_with_pss_plus_data6)
    plot_cnt = plot_cnt + 1;
    subplot(nof_plots,1,plot_cnt);
    yyaxis left
    plot(10*log10(noise_variance), correct_miss_with_data6, 'LineWidth', 1);
    ylabel('Correct miss');
    hold on;
    yyaxis right
    plot(10*log10(noise_variance), false_detection_with_data6, 'LineWidth', 1);
    ylabel('False detection');
    grid on;
    xlabel('Noise power [dBW]')
    title('OFDM Symbol with PSS + Data - Min-value-PSR-based Threshold');
    hold off;
end

if(enable_ofdm_with_pss_plus_data7)
    plot_cnt = plot_cnt + 1;
    subplot(nof_plots,1,plot_cnt);
    yyaxis left
    plot(10*log10(noise_variance), correct_miss_with_data7, 'LineWidth', 1);
    ylabel('Correct miss');
    hold on;
    yyaxis right
    plot(10*log10(noise_variance), false_detection_with_data7, 'LineWidth', 1);
    ylabel('False detection');
    grid on;
    xlabel('Noise power [dBW]')
    title('OFDM Symbol with PSS + Data - Min-value-PSR-based Threshold');
    hold off;
end

if(enable_ofdm_with_pss_plus_data8)
    plot_cnt = plot_cnt + 1;
    subplot(nof_plots,1,plot_cnt);
    yyaxis left
    plot(10*log10(noise_variance), correct_miss_with_data8, 'LineWidth', 1);
    ylabel('Correct miss');
    hold on;
    yyaxis right
    plot(10*log10(noise_variance), false_detection_with_data8, 'LineWidth', 1);
    ylabel('False detection');
    grid on;
    xlabel('Noise power [dBW]')
    title('OFDM Symbol with PSS + Data - Min-value-PSR-based Threshold');
    hold off;
end

if(enable_ofdm_with_pss_plus_data9)
    plot_cnt = plot_cnt + 1;
    subplot(nof_plots,1,plot_cnt);
    yyaxis left
    plot(10*log10(noise_variance), correct_miss_with_data9, 'LineWidth', 1);
    ylabel('Correct miss');
    hold on;
    yyaxis right
    plot(10*log10(noise_variance), false_detection_with_data9, 'LineWidth', 1);
    ylabel('False detection');
    grid on;
    xlabel('Noise power [dBW]')
    title('OFDM Symbol with PSS + Data - Min-value-PSR-based Threshold');
    hold off;
end