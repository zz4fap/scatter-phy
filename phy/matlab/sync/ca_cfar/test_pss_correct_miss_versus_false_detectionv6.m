clear all;close all;clc

phy_bw_idx = 3;

plot_figures = 0;

add_noise = 1;

NUM_ITER = 1e4;

SNR = -20:2:6; %-30:2.5:0;

noisePower = -SNR;

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

enable_ofdm_with_pss_plus_data        = false;
enable_ofdm_with_pss_plus_data9       = true;
enable_ofdm_with_pss_plus_data10      = true;

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
    
    correct_detection_with_data = zeros(1,length(SNR));
    correct_miss_with_data = zeros(1,length(SNR));
    false_detection_with_data = zeros(1,length(SNR));
    false_detection_peak_pos = zeros(length(SNR),NUM_ITER);
    rng(12041988);
    for snr_idx = 1:1:length(SNR)
        
        false_detection_peak_pos_cnt = 1;
        for n = 1:1:NUM_ITER
            
            if(add_noise == 1)
                %noisy_signal = sqrt(SNR(snr_idx))*(1/sqrt(2))*complex(randn(SUBFRAME_LENGTH(phy_bw_idx),1),randn(SUBFRAME_LENGTH(phy_bw_idx),1));
                noisy_signal = wgn(SUBFRAME_LENGTH(phy_bw_idx), 1, noisePower(snr_idx), 'complex');
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
if(enable_ofdm_with_pss_plus_data9)
    % Generate PSS signal.
    pss = lte_pss_zc(cell_id);
    
    % Create OFDM symbol number 6, the one which carries PSS.
    pss_zero_pad = [0;pss((SRSLTE_PSS_LEN/2)+1:end);zeros(NDFT-(SRSLTE_PSS_LEN+1),1);pss(1:SRSLTE_PSS_LEN/2)];
    local_time_domain_pss = (sqrt(NDFT)/sqrt(SRSLTE_PSS_LEN))*ifft(pss_zero_pad, NDFT);
    
    % Conjugated local version of PSS in time domain.
    local_conj_time_domain_pss = conj(local_time_domain_pss);
    
    local_conj_time_domain_pss = [local_conj_time_domain_pss; zeros(NDFT2-length(local_conj_time_domain_pss),1)];
    
    smaller_local_time_domain_pss = conj(local_time_domain_pss); 
    
    correct_detection_with_data9 = zeros(1,length(SNR));
    miss_detection_with_data9 = zeros(1,length(SNR));
    false_detection_with_data9 = zeros(1,length(SNR));
    correct_miss_with_data9 = zeros(1,length(SNR));
    false_detection_peak_pos9 = zeros(length(SNR),NUM_ITER);
    miss_detection_psr9 = zeros(length(SNR), NUM_ITER);
    miss_detection_psr_cnt9 = ones(length(SNR));
    psr_avg9 = zeros(length(SNR));
    psr_cnt9 = zeros(length(SNR));     
    rng(12041988);
    for snr_idx = 1:1:length(SNR)
        
        false_detection_peak_pos_cnt = 1;
        for n = 1:1:NUM_ITER
            
            if(add_noise == 1)
                %noisy_signal = sqrt(SNR(snr_idx))*(1/sqrt(2))*complex(randn(SUBFRAME_LENGTH(phy_bw_idx),1),randn(SUBFRAME_LENGTH(phy_bw_idx),1));
                noisy_signal = wgn(SUBFRAME_LENGTH(phy_bw_idx), 1, noisePower(snr_idx), 'complex');
            else
                noisy_signal = complex(zeros(SUBFRAME_LENGTH(phy_bw_idx),1),zeros(SUBFRAME_LENGTH(phy_bw_idx),1));
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
                
                smaller_received_signal_fft = fft([smaller_input_signal; zeros(NDFT, 1)], 2*NDFT);
                
                smaller_local_conj_pss_fft = fft([smaller_local_time_domain_pss; zeros(NDFT, 1)], 2*NDFT);
                
                smaller_prod = smaller_received_signal_fft.*smaller_local_conj_pss_fft;
                
                smaller_convolution = ifft(smaller_prod, 2*NDFT);
                
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






if(enable_ofdm_with_pss_plus_data10)
    
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
    
    correct_detection_with_data10 = zeros(1,length(SNR));
    correct_miss_with_data10 = zeros(1,length(SNR));
    miss_detection_with_data10 = zeros(1,length(SNR));
    false_detection_with_data10 = zeros(1,length(SNR));
    false_detection_peak_pos10 = zeros(length(SNR),NUM_ITER);
    miss_detection_psr10 = zeros(length(SNR), NUM_ITER);
    miss_detection_psr_cnt10 = ones(length(SNR));
    psr_avg10 = zeros(length(SNR));
    psr_cnt10 = zeros(length(SNR));     
    rng(12041988);
    for snr_idx = 1:1:length(SNR)
        
        false_detection_peak_pos_cnt = 1;
        for n = 1:1:NUM_ITER
            
            if(add_noise == 1)
                %noisy_signal = sqrt(SNR(snr_idx))*(1/sqrt(2))*complex(randn(SUBFRAME_LENGTH(phy_bw_idx),1),randn(SUBFRAME_LENGTH(phy_bw_idx),1));
                noisy_signal = wgn(SUBFRAME_LENGTH(phy_bw_idx), 1, noisePower(snr_idx), 'complex');
            else
                noisy_signal = complex(zeros(SUBFRAME_LENGTH(phy_bw_idx),1),zeros(SUBFRAME_LENGTH(phy_bw_idx),1));
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
                false_detection_with_data10(snr_idx) = false_detection_with_data10(snr_idx) + 1;
            else
                correct_miss_with_data10(snr_idx) = correct_miss_with_data10(snr_idx) + 1;
            end
            
            
            
            
        end
        
    end
    
    correct_detection_with_data10 = correct_detection_with_data10./NUM_ITER;
    miss_detection_with_data10 = miss_detection_with_data10./NUM_ITER;
    false_detection_with_data10 = false_detection_with_data10./NUM_ITER;
    correct_miss_with_data10 = correct_miss_with_data10./NUM_ITER;
    
    psr_avg10(snr_idx) = psr_avg10(snr_idx)./psr_cnt10(snr_idx); 
end





fdee_figure1 = figure('rend','painters','pos',[10 10 1000 900]);

nof_plots = 2;
plot_cnt = 0;

if(enable_ofdm_with_pss_plus_data)
    plot_cnt = plot_cnt + 1;
    subplot(nof_plots,1,plot_cnt);
    yyaxis left
    plot(SNR, correct_miss_with_data, 'LineWidth', 1);
    ylabel('Correct miss');
    hold on;
    yyaxis right
    plot(SNR, false_detection_with_data, 'LineWidth', 1);
    ylabel('False detection');
    grid on;
    xlabel('Noise power [dBW]')
    title('OFDM Symbol with PSS + Data - Legacy PSR-based Threshold');
    hold off;
end

if(enable_ofdm_with_pss_plus_data9)
    plot_cnt = plot_cnt + 1;
    subplot(nof_plots,1,plot_cnt);
    yyaxis left
    plot(SNR, correct_miss_with_data9, 'LineWidth', 1);
    ylabel('Correct miss');
    hold on;
    yyaxis right
    plot(SNR, false_detection_with_data9, 'LineWidth', 1);
    ylabel('False detection');
    grid on;
    xlabel('Noise power [dBW]')
    titleStr = sprintf('OFDM Symbol with PSS + Data - 2-stage (PSR)');
    title(titleStr)
    hold off;
end

if(enable_ofdm_with_pss_plus_data10)
    plot_cnt = plot_cnt + 1;
    subplot(nof_plots,1,plot_cnt);
    yyaxis left
    plot(SNR, correct_miss_with_data10, 'LineWidth', 1);
    ylabel('Correct miss');
    hold on;
    yyaxis right
    plot(SNR, false_detection_with_data10, 'LineWidth', 1);
    ylabel('False detection');
    grid on;
    xlabel('Noise power [dBW]')
    titleStr = sprintf('OFDM Symbol with PSS + Data - 2-stage (CA-CFAR)');
    title(titleStr)
    hold off;
end
