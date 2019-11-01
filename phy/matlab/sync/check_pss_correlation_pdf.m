clear all;close all;clc

plot_figures = 0;

add_noise = 1;

generate_lte_pss = 1;

calculate_pfa = 1;

NUM_ITER = 100;

SNR = -20; % in dB

noisepower = -20; % in dBW

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
local_time_domain_pss = (sqrt(NDFT)/sqrt(SRSLTE_PSS_LEN))*ifft(pss_zero_pad, NDFT);

if(plot_figures == 1)
    ofdm_symbol = (1/sqrt(NDFT))*fftshift(fft(local_time_domain_pss,NDFT));
    figure;
    plot(0:1:NDFT-1,10*log10(abs(ofdm_symbol)),'b-')
end

% Conjugated local version of PSS in time domain.
local_conj_time_domain_pss = conj(local_time_domain_pss);

local_conj_time_domain_pss = [local_conj_time_domain_pss; zeros(FRAME_SIZE,1)];

local_conj_pss_fft = fft(local_conj_time_domain_pss,NDFT2);

convolution_pdf = [];
noisy_signal_pdf = [];
power_conv_pdf = [];
for numIter = 1:1:NUM_ITER
    
    num_int = 5376; %randi(FRAME_SIZE);
    
    if(calculate_pfa == 0)
        rx_signal = [zeros(num_int,1); local_time_domain_pss; zeros(FRAME_SIZE-NDFT,1)];
        
        rx_signal_buffer = rx_signal(1:FRAME_SIZE);
    end
    
    if(add_noise == 1)
        if(calculate_pfa == 1)         
            %noisy_signal = wgn(FRAME_SIZE, 1, noisepower, 'complex');
            
            noisy_signal = (sqrt(0.2))*(1/sqrt(2))*complex(randn(FRAME_SIZE,1),randn(FRAME_SIZE,1));
        else
            noisy_signal = awgn(rx_signal_buffer,SNR,'measured');
        end
    else
        noisy_signal = rx_signal_buffer;
    end
    
    noisy_signal_pdf = [noisy_signal_pdf; noisy_signal];
    
    % Detection of PSS.
    input_signal = [noisy_signal; zeros(NDFT2-FRAME_SIZE,1)];
    
    received_signal_fft = fft(input_signal,NDFT2);
    
    prod = received_signal_fft.*local_conj_pss_fft;
    
    convolution = ifft(prod,NDFT2);
    
    power_conv = abs(convolution).^2;
    
    convolution_pdf = [convolution_pdf; convolution];
    
    power_conv_pdf = [power_conv_pdf; power_conv];
    
end

gaussian_control = (sqrt(0.2))*(1/sqrt(2))*complex(randn(1,length(convolution_pdf)),randn(1,length(convolution_pdf)));

h1 = histogram(real(convolution_pdf));
hold on
h2 = histogram(real(gaussian_control));

h1.Normalization = 'probability';
h1.BinWidth = 0.005;

h2.Normalization = 'probability';
h2.BinWidth = 0.005;

legend('conv','control');
hold off

% h1 = histogram(real(noisy_signal_pdf));
% hold on
% h2 = histogram(real(convolution_pdf));
% 
% h1.Normalization = 'probability';
% h1.BinWidth = 0.005;
% 
% h2.Normalization = 'probability';
% h2.BinWidth = 0.005;
% 
% legend('Noisy signal','Convolution');
% hold off

%% ---------------------------- References --------------------------------

% [1] Janne J. Lehtomake, Markku Juntti, and Harri Saarnisaari, "CFAR Strategies for Channelized Radiometer", IEEE Signal Processing Letters, Vol. 12, No. 1, January, 2005.
% [2] Janne J. Lehtomake, Johanna Vartiainen, Markku Juntti, and Harri Saarnisaari , "CFAR Outlier Detection With Forward Methods", IEEE Transactions on Signal Processing, vol. 55, no. 9, September 2007
