clear all;close all;clc

phy_bw_idx = 3;

plot_figures = 0;

add_noise = 1;

NUM_ITER = 1e3;

noise_variance = linspace(0.001,1,100);

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

enable_ofdm_with_pss_plus_data        = true;
enable_ofdm_with_pss_plus_data_filter = true;
enable_ofdm_with_pss_plus_data2       = true;
enable_ofdm_with_pss_plus_data3       = true;

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
            
            if(psr_max >= 3.0)
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
            
            threshold = 3.0;

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
             
            threshold = 3.0;
            
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

if(0)
    % Get timestamp for saving files.
    timeStamp = datestr(now,30);
    fileName = sprintf('test_pss_correct_detection_versus_false_detectionv1_%s.mat',timeStamp);
    save(fileName);
end

fdee_figure1 = figure('rend','painters','pos',[10 10 1000 900]);

if(enable_ofdm_with_pss_plus_data)
    subplot(4,1,1);
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
    subplot(4,1,2);
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
    subplot(4,1,3);
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
    subplot(4,1,4);
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
