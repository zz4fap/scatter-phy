clear all;close all;clc

NUM_ITER = 1000;

NDFT = 2048;

prime_numbers = [2 3 5 7 11 13 17 19 23 29 31 37 41 43 47 53 59 61 67 71 73 79 83 89 97 101 103 107 109 113 127 131 137 139 149 151 157 163    167    173   179    181    191    193    197    199    211    223    227    229  233    239    241    251    257    263    269    271    277    281  283    293    307    311    313    317    331    337    347    349  353    359    367    373    379    383    389    397    401    409  419    421    431    433    439    443    449    457    461    463  467    479    487    491    499    503    509    521    523    541  547    557    563    569    571    577    587    593    599    601  607    613    617    619    631    641    643    647    653    659  661    673    677    683    691    701    709    719    727    733  739    743    751    757    761    769    773    787    797    809  811    821    823    827    829    839];

Nzc = 409;

u = prime_numbers(13);

n = 0:1:Nzc-1;

preamble = exp((-1i.*pi.*u.*n.*(n+1))./Nzc).';

detection_preamble = conj(preamble);
detection_preamble_zero_pad = [detection_preamble;zeros(NDFT-Nzc,1)];
detection_preamble_fft = fft(detection_preamble_zero_pad,NDFT);

for n=1:1:NUM_ITER
    
    num_int = 2048-Nzc+300; %randi(NDFT);
    
    tx_signal = [zeros(num_int,1); preamble; zeros(NDFT-num_int-Nzc,1)];
    
    rx_noisy_signal = tx_signal(1:NDFT); %awgn(rx_signal,snr,'measured');
    
    % Preamble Detection.    
    input_signal_fft = fft(rx_noisy_signal,NDFT);
    
    prod = input_signal_fft.*detection_preamble_fft;
    
    convolution = ifft(prod,NDFT);
    
    power_conv = abs(convolution).^2;
    
    [value peak_pos] = max(power_conv);
    
    %fprintf(1,'%d\n',abs(num_int-peak_pos));
    fprintf(1,'%d - %d\n',abs(num_int+NDFT), peak_pos);
    
    plot(0:1:NDFT-1,power_conv);
    
    a=1;
    
end
