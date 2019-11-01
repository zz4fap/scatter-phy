clear all;close all;clc

plot_figures = 0;

add_noise = 1;

NUM_ITER = 1e3;

SNR = -30:2.5:0;

cell_id = 0;
NDFT = 384; % For 5 MHz PHY BW.
delta_f = 15e3; % subcarrier spacing in Hz.
USEFUL_SUBCARRIERS = 300;
PSS_GUARD_BAND = 10; % in number of subcarriers.
SRSLTE_PSS_LEN = 62;
FRAME_SIZE = 5760; % For 5 MHz PHY BW.
NDFT2 = FRAME_SIZE + NDFT;

enable_ofdm_with_pss_only = false;
if(enable_ofdm_with_pss_only)
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
            
            %plot(abs((local_conj_pss_fft)).^2);
            
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
    
end

%% --------------------------------------------------------------------------
enable_ofdm_with_pss_plus_data = false;
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
    
    % generate QPSK data.
    data = randi([0 3], USEFUL_SUBCARRIERS-SRSLTE_PSS_LEN-PSS_GUARD_BAND-1, 1);
    % Apply QPSK modulation
    data_symbols = qammod(data, 4, 'UnitAveragePower', true);
    % Create OFDM symbol number 6, the one which carries PSS.
    tx_pss_zero_pad = [0; pss((SRSLTE_PSS_LEN/2)+1:end); zeros(5,1); data_symbols(1:114,1); zeros(84,1); data_symbols(114+1:end,1); zeros(5,1); pss(1:SRSLTE_PSS_LEN/2)];
    tx_time_domain_pss = (sqrt(NDFT)/sqrt(SRSLTE_PSS_LEN))*ifft(tx_pss_zero_pad, NDFT);
    
    %if(1)
    if(plot_figures == 1)
        ofdm_symbol = (1/sqrt(NDFT))*ifftshift(fft(tx_time_domain_pss,NDFT));
        figure;
        plot(0:1:NDFT-1,10*log10(abs(ofdm_symbol)),'b-')
    end
    
    % Conjugated local version of PSS in time domain.
    local_conj_time_domain_pss = conj(local_time_domain_pss);
    
    local_conj_time_domain_pss = [local_conj_time_domain_pss; zeros(FRAME_SIZE,1)];
    
    correct_detection_with_data = zeros(1,length(SNR));
    false_detection_with_data = zeros(1,length(SNR));
    rng(12041988);
    for snr_idx = 1:1:length(SNR)
        
        for n=1:1:NUM_ITER
            
            % Generate random QPSK data.
            data = randi([0 3], USEFUL_SUBCARRIERS-SRSLTE_PSS_LEN-PSS_GUARD_BAND-1, 1);
            % Apply QPSK modulation.
            data_symbols = qammod(data, 4, 'UnitAveragePower', true);
            tx_pss_zero_pad = [0; pss((SRSLTE_PSS_LEN/2)+1:end); zeros(5,1); data_symbols(1:114,1); zeros(84,1); data_symbols(114+1:end,1); zeros(5,1); pss(1:SRSLTE_PSS_LEN/2)];
            tx_time_domain_pss = (sqrt(NDFT)/sqrt(SRSLTE_PSS_LEN))*ifft(tx_pss_zero_pad, NDFT);
            
            num_int = randi(FRAME_SIZE-NDFT);
            
            rx_signal = [zeros(num_int,1); tx_time_domain_pss; zeros(FRAME_SIZE-NDFT,1)];
            
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
                correct_detection_with_data(snr_idx) = correct_detection_with_data(snr_idx) + 1;
            else
                false_detection_with_data(snr_idx) = false_detection_with_data(snr_idx) + 1;
            end
            
        end
        
    end
    
    correct_detection_with_data = correct_detection_with_data./NUM_ITER;
    false_detection_with_data = false_detection_with_data./NUM_ITER;
end

enable_ofdm_with_pss_only_plus_filter = false;
if(enable_ofdm_with_pss_only_plus_filter)
    % Generate PSS signal.
    pss = lte_pss_zc(cell_id);
    
    % Create OFDM symbol number 6, the one which carries PSS.
    padding = (NDFT-SRSLTE_PSS_LEN)/2;
    pss_zero_pad = [0;pss((SRSLTE_PSS_LEN/2)+1:end);zeros(NDFT-(SRSLTE_PSS_LEN+1),1);pss(1:SRSLTE_PSS_LEN/2)];
    local_time_domain_pss = (sqrt(NDFT)/sqrt(SRSLTE_PSS_LEN))*ifft(pss_zero_pad, NDFT);
    
    local_filter_pad = [0; ones((SRSLTE_PSS_LEN/2), 1); zeros(NDFT-(SRSLTE_PSS_LEN+1),1); ones((SRSLTE_PSS_LEN/2), 1)];
    local_filter_time_domain = (sqrt(NDFT)/sqrt(SRSLTE_PSS_LEN))*ifft(local_filter_pad, NDFT);
    
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
            
            filter_ = [local_filter_time_domain; zeros(NDFT2-length(local_filter_time_domain),1)];
            
            filter_fft = fft(filter_,NDFT2);
            
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
    
end






enable_ofdm_with_pss_plus_data_filter = true;
if(enable_ofdm_with_pss_plus_data_filter)
    
    % Calculate sampling rate.
    sampling_rate = NDFT*delta_f;
    
    % Create FIR filter for improved PSS detection.
    FIR_ORDER = 256;
    pss_filter = fir1(FIR_ORDER, ((PSS_GUARD_BAND+SRSLTE_PSS_LEN)*delta_f)/sampling_rate).';
    %freqz(fir_coef,1,512)
    
    % Generate PSS signal.
    pss = lte_pss_zc(cell_id);
    
    % Create OFDM symbol number 6, the one which carries PSS.
    padding = (NDFT-SRSLTE_PSS_LEN)/2;
    pss_zero_pad = [0;pss((SRSLTE_PSS_LEN/2)+1:end);zeros(NDFT-(SRSLTE_PSS_LEN+1),1);pss(1:SRSLTE_PSS_LEN/2)];
    local_time_domain_pss = (sqrt(NDFT)/sqrt(SRSLTE_PSS_LEN))*ifft(pss_zero_pad, NDFT);
    
    %if(1)
    if(plot_figures == 1)
        ofdm_symbol = (1/sqrt(NDFT))*ifftshift(fft(local_time_domain_pss,NDFT));
        figure;
        plot(0:1:NDFT-1,10*log10(abs(ofdm_symbol)),'b-')
    end
    
    % generate QPSK data.
    data = randi([0 3], USEFUL_SUBCARRIERS-SRSLTE_PSS_LEN-PSS_GUARD_BAND-1, 1);
    % Apply QPSK modulation
    data_symbols = qammod(data, 4, 'UnitAveragePower', true);
    % Create OFDM symbol number 6, the one which carries PSS.
    tx_pss_zero_pad = [0; pss((SRSLTE_PSS_LEN/2)+1:end); zeros(5,1); data_symbols(1:114,1); zeros(84,1); data_symbols(114+1:end,1); zeros(5,1); pss(1:SRSLTE_PSS_LEN/2)];
    tx_time_domain_pss = (sqrt(NDFT)/sqrt(SRSLTE_PSS_LEN))*ifft(tx_pss_zero_pad, NDFT);
    
    %if(1)
    if(plot_figures == 1)
        ofdm_symbol = (1/sqrt(NDFT))*ifftshift(fft(tx_time_domain_pss,NDFT));
        figure;
        plot(0:1:NDFT-1,10*log10(abs(ofdm_symbol)),'b-')
    end
    
    % Conjugated local version of PSS in time domain.
    local_conj_time_domain_pss = conj(local_time_domain_pss);
    
    local_conj_time_domain_pss = [local_conj_time_domain_pss; zeros(FRAME_SIZE,1)];
    
    local_conj_pss_fft = fft(local_conj_time_domain_pss,NDFT2);
    
    correct_detection_with_data_filter = zeros(1,length(SNR));
    false_detection_with_data_filter = zeros(1,length(SNR));
    rng(12041988);
    for snr_idx = 1:1:length(SNR)
        
        for n=1:1:NUM_ITER
            
            % Generate random QPSK data.
            data = randi([0 3], USEFUL_SUBCARRIERS-SRSLTE_PSS_LEN-PSS_GUARD_BAND-1, 1);
            % Apply QPSK modulation.
            data_symbols = qammod(data, 4, 'UnitAveragePower', true);
            tx_pss_zero_pad = [0; pss((SRSLTE_PSS_LEN/2)+1:end); zeros(5,1); data_symbols(1:114,1); zeros(84,1); data_symbols(114+1:end,1); zeros(5,1); pss(1:SRSLTE_PSS_LEN/2)];
            tx_time_domain_pss = (sqrt(NDFT)/sqrt(SRSLTE_PSS_LEN))*ifft(tx_pss_zero_pad, NDFT);
            
            num_int = randi(FRAME_SIZE-NDFT);
            
            rx_signal = [zeros(num_int,1); tx_time_domain_pss; zeros(FRAME_SIZE-NDFT,1)];
            
            rx_signal_buffer = rx_signal(1:FRAME_SIZE);
            
            if(add_noise == 1)
                noisy_signal = awgn(rx_signal_buffer, SNR(snr_idx), 'measured');
            else
                noisy_signal = rx_signal_buffer;
            end

            % Filter the received signal.
            filtered_noisy_signal = filter(pss_filter, 1, noisy_signal);
            
            if(0)
                figure;
                plot(abs(fft(local_conj_time_domain_pss)).^2);
                figure;
                plot(abs(fft(noisy_signal)).^2);
            end
            
            % Detection of PSS.
            input_signal = [filtered_noisy_signal; zeros(NDFT2-FRAME_SIZE,1)];
            
            received_signal_fft = fft(input_signal,NDFT2);
            
            prod = received_signal_fft.*local_conj_pss_fft;
            
            convolution = ifft(prod,NDFT2);
            
            power_conv = abs(convolution).^2;
            
            [value, peak_pos] = max(power_conv);
            
            if(abs(num_int+NDFT) == (peak_pos - 1 - (FIR_ORDER/2)))
                correct_detection_with_data_filter(snr_idx) = correct_detection_with_data_filter(snr_idx) + 1;
            else
                false_detection_with_data_filter(snr_idx) = false_detection_with_data_filter(snr_idx) + 1;
            end
            
        end
        
    end
    
    correct_detection_with_data_filter = correct_detection_with_data_filter./NUM_ITER;
    false_detection_with_data_filter = false_detection_with_data_filter./NUM_ITER;
end





% Get timestamp for saving files.
timeStamp = datestr(now,30);
fileName = sprintf('test_pss_correct_detection_versus_false_detectionv1_%s.mat',timeStamp);
save(fileName);

figure;
if(enable_ofdm_with_pss_only)
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
    title('OFDM Symbol with PSS Only');
end

if(enable_ofdm_with_pss_plus_data)
    subplot(2,1,2);
    yyaxis left
    plot(SNR,correct_detection_with_data);
    ylabel('Correct Detection')
    hold on;
    yyaxis right
    plot(SNR,false_detection_with_data);
    ylabel('False Detection')
    grid on
    xlabel('SNR [dB]')
    title('OFDM Symbol with PSS + Data');
end

if(enable_ofdm_with_pss_plus_data_filter)
    subplot(2,1,2);
    yyaxis left
    plot(SNR,correct_detection_with_data_filter);
    ylabel('Correct Detection')
    hold on;
    yyaxis right
    plot(SNR,false_detection_with_data_filter);
    ylabel('False Detection')
    grid on
    xlabel('SNR [dB]')
    title('OFDM Symbol with PSS + Filter + Data');
end
