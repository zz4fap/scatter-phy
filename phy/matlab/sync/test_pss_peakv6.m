clear all;close all;clc

plot_figures = 0;

add_noise = 1;

generate_lte_pss = 1;

calculate_pfa = 1;

NUM_ITER = 1000;

SNR = -20; % in dB

noisepower = 0; % in dBW

Pfd = 0.001;

numSamplesSegment = 1;

Tcme = 6.9078; % for 1 sample

Pfa = 0.1/100; % 0.1%

cell_id = 0;
NDFT = 384; % For 5 MHz PHY BW.
SRSLTE_PSS_LEN = 62;
FRAME_SIZE = 5760; % For 5 MHz PHY BW.
NDFT2 = FRAME_SIZE + NDFT;

% Generate PSS signal.
if(generate_lte_pss==1)
    pss = lte_pss_zc(cell_id);
else
    prime_numbers = [2 3 5 7 11 13 17 19 23 29 31 37 41 43 47 53 59 61 67 71 73 79 83 89 97 101 103 107 109 113 127 131 137 139 149 151 157 163    167    173   179    181    191    193    197    199    211    223    227    229  233    239    241    251    257    263    269    271    277    281  283    293    307    311    313    317    331    337    347    349  353    359    367    373    379    383    389    397    401    409  419    421    431    433    439    443    449    457    461    463  467    479    487    491    499    503    509    521    523    541  547    557    563    569    571    577    587    593    599    601  607    613    617    619    631    641    643    647    653    659  661    673    677    683    691    701    709    719    727    733  739    743    751    757    761    769    773    787    797    809  811    821    823    827    829    839];
    u = prime_numbers(9);
    Nzc = prime_numbers(18);
    pss = [generate_zadoff_chu_seq(u,Nzc).';0];
end

% Create OFDM symbol number 6, the one which carries PSS.
pss_zero_pad = [0;pss((SRSLTE_PSS_LEN/2)+1:end);zeros(NDFT-(SRSLTE_PSS_LEN+1),1);pss(1:SRSLTE_PSS_LEN/2)];
%local_time_domain_pss = (sqrt(NDFT)/sqrt(SRSLTE_PSS_LEN))*ifft(pss_zero_pad, NDFT);
local_time_domain_pss = (sqrt(SRSLTE_PSS_LEN))*(sqrt(NDFT)/sqrt(SRSLTE_PSS_LEN))*ifft(pss_zero_pad, NDFT);


if(plot_figures == 1)
    ofdm_symbol = (1/sqrt(NDFT))*fftshift(fft(local_time_domain_pss,NDFT));
    figure;
    plot(0:1:NDFT-1,10*log10(abs(ofdm_symbol)),'b-')
end

% Conjugated local version of PSS in time domain.
local_conj_time_domain_pss = conj(local_time_domain_pss);

local_conj_time_domain_pss = [local_conj_time_domain_pss; zeros(FRAME_SIZE,1)];

local_conj_pss_fft = fft(local_conj_time_domain_pss,NDFT2);

correct_detection_counter = 0;
false_rejection_counter = 0;
false_alarm_counter = 0;
correct_rejection_counter = 0;
for numIter = 1:1:NUM_ITER
    
    num_int = 5376; %randi(FRAME_SIZE);
    
    if(calculate_pfa == 0)
        rx_signal = [zeros(num_int,1); local_time_domain_pss; zeros(FRAME_SIZE-NDFT,1)];
        
        rx_signal_buffer = rx_signal(1:FRAME_SIZE);
    end
    
    if(add_noise == 1)
        if(calculate_pfa == 1)                     
            noisy_signal = (sqrt(0.02))*(1/sqrt(2))*complex(randn(FRAME_SIZE,1),randn(FRAME_SIZE,1));
        else
            noisy_signal = awgn(rx_signal_buffer,SNR,'measured');
        end
    else
        noisy_signal = rx_signal_buffer;
    end
    
    % Detection of PSS.
    input_signal = [noisy_signal; zeros(NDFT2-FRAME_SIZE,1)];
    
    received_signal_fft = fft(input_signal,NDFT2);
    
    prod = received_signal_fft.*local_conj_pss_fft;
    
    convolution = ifft(prod,NDFT2);
    
    power_conv = abs(convolution).^2;
    
%    [peak_value, peak_pos] = max(power_conv);
%    % Subtract one from the found peak so that it starts from 0.
%    peak_pos = peak_pos - 1;
%    fprintf(1,'Symbol offset: %d - Peak position: %d - symbol start: %d\n',num_int,peak_pos, peak_pos-NDFT);
    
    %% ---------------------- Dectection Algorithm ------------------------
    
    % This is the implemenatation of the algorithm proposed in [1]
    % Sort Segmented Z vector in increasing order of energy.
    sorted_power_conv = sort(power_conv);
    
%     This is the way it is implemented in SW, should be improved as it is time-intensive.    
%     sorted_power_conv = power_conv;
%     for i=1:1:length(sorted_power_conv)
%         for k=1:1:length(sorted_power_conv)-1
%             if(sorted_power_conv(k) > sorted_power_conv(k+1))
%                 temp = sorted_power_conv(k+1);
%                 sorted_power_conv(k+1) = sorted_power_conv(k);
%                 sorted_power_conv(k) = temp;
%             end
%         end
%     end

    
    % Algorithm used to discard samples. Indeed, it is used to find a
    % speration between noisy samples and signal+noise samples.
    for I=10:1:length(sorted_power_conv)-1
        limiar = Tcme/I;
        if(sorted_power_conv(I+1) > limiar*sum(sorted_power_conv(1:I)))
            break;
        end
    end    
    
    % Output of the disposal module. It's used as a noise reference.
    Zref = sum(sorted_power_conv(1:I));
    
    % Calculate the scale factor.
    alpha = finv(1-Pfa,2*numSamplesSegment,(2*numSamplesSegment*I))/I;
    
    %% --------------------- Calculate Pfa and Pd ------------------------- 
    if(calculate_pfa == 1)
        [peak_value, peak_pos] = max(power_conv);
        ratio = peak_value/Zref;
        if(ratio > alpha)
            false_alarm_counter = false_alarm_counter + 1;
        else
            correct_rejection_counter = correct_rejection_counter + 1;
        end
    else
        ratio = power_conv(num_int+NDFT)/Zref;
        if(ratio > alpha)
            correct_detection_counter = correct_detection_counter + 1;
        else
            false_rejection_counter = false_rejection_counter + 1;
        end      
    end
    
end

correct_detection_probabilty = correct_detection_counter/NUM_ITER;

false_rejection_probabilty = false_rejection_counter/NUM_ITER;

false_alarm_probabilty = false_alarm_counter/NUM_ITER;

correct_rejection_probabilty = correct_rejection_counter/NUM_ITER;

fprintf(1,'Pfa: %f\n',false_alarm_probabilty);

%% ---------------------------- References --------------------------------

% [1] Janne J. Lehtomake, Markku Juntti, and Harri Saarnisaari, "CFAR Strategies for Channelized Radiometer", IEEE Signal Processing Letters, Vol. 12, No. 1, January, 2005.
% [2] Janne J. Lehtomake, Johanna Vartiainen, Markku Juntti, and Harri Saarnisaari , "CFAR Outlier Detection With Forward Methods", IEEE Transactions on Signal Processing, vol. 55, no. 9, September 2007
