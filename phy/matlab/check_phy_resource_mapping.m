clear all;close all;clc

%filename = '/home/zz4fap/work/timed_comands_for_discontinous_tx/scatter/build/tx_side_assessment_5MHz.dat';

%filename = '/home/zz4fap/work/backup_plan/scatter/build/detection_decoding_error.dat';

%filename = '/home/zz4fap/work/backup_plan/scatter/build/check_subframe_discontinuity_sf_10_cnt_4.dat';

filename = '/home/zz4fap/Downloads/cp_ofdm_tx_side_assessment_5MHz_v0.dat';

fileID = fopen(filename);

phy_bw_to_be_used = 3;

SUBFRAME_LENGTH = [1920 3840 5760 11520 15360 23040];
numFFT = [128 256 384 768 1024 1536];       % Number of FFT points
deltaF = 15000;                             % Subcarrier spacing
numRBs = [6 15 25 50 75 100];               % Number of resource blocks
rbSize = 12;                                % Number of subcarriers per resource block
cpLen_1st_symb  = [10 20 30 60 80 120];     % Cyclic prefix length in samples only for very 1st symbol.
cpLen_other_symbs = [9 18 27 54 72 108];

signal_buffer_aux = complex(zeros(1,SUBFRAME_LENGTH(phy_bw_to_be_used)),zeros(1,SUBFRAME_LENGTH(phy_bw_to_be_used)));

value = fread(fileID, 2, 'float');
idx = 0;
while ~feof(fileID)
    
    idx = idx + 1;
    signal_buffer_aux(idx) = complex(value(1),value(2));
    
    value = fread(fileID, 2, 'float');
end
fclose(fileID);

signal_buffer = signal_buffer_aux;

symbol0 = signal_buffer(cpLen_1st_symb(phy_bw_to_be_used)+1:cpLen_1st_symb(phy_bw_to_be_used)+numFFT(phy_bw_to_be_used));
ofdm_symbol0 = (1/sqrt(numFFT(phy_bw_to_be_used)))*fftshift(fft(symbol0,numFFT(phy_bw_to_be_used))).';
%ofdm_symbol0 = (1/sqrt(numFFT(phy_bw_to_be_used)))*(fft(symbol0,numFFT(phy_bw_to_be_used)));

figure;
plot(0:1:numFFT(phy_bw_to_be_used)-1,10*log10(abs(ofdm_symbol0)),'b-')
title('Symbol #0')

figure;
plot(real(ofdm_symbol0(43:343)),imag(ofdm_symbol0(43:343)),'b*')
title('Constellation Symbol #0')

symbol1 = signal_buffer(cpLen_1st_symb(phy_bw_to_be_used)+numFFT(phy_bw_to_be_used)+cpLen_other_symbs(phy_bw_to_be_used)+1:cpLen_1st_symb(phy_bw_to_be_used)+2*numFFT(phy_bw_to_be_used)+cpLen_other_symbs(phy_bw_to_be_used));
ofdm_symbol1 = (1/sqrt(numFFT(phy_bw_to_be_used)))*fftshift(fft(symbol1,numFFT(phy_bw_to_be_used))).';
%ofdm_symbol1 = (1/sqrt(numFFT(phy_bw_to_be_used)))*(fft(symbol1,numFFT(phy_bw_to_be_used)));

figure;
plot(0:1:numFFT(phy_bw_to_be_used)-1,10*log10(abs(ofdm_symbol1)),'b-')
title('Symbol #1')

figure;
plot(real(ofdm_symbol1(43:343)),imag(ofdm_symbol1(43:343)),'b*')
title('Constellation Symbol #1')

sym2_start = cpLen_1st_symb(phy_bw_to_be_used)+numFFT(phy_bw_to_be_used)+cpLen_other_symbs(phy_bw_to_be_used)+numFFT(phy_bw_to_be_used)+cpLen_other_symbs(phy_bw_to_be_used)+1;
sym2_end = cpLen_1st_symb(phy_bw_to_be_used)+numFFT(phy_bw_to_be_used)+cpLen_other_symbs(phy_bw_to_be_used)+numFFT(phy_bw_to_be_used)+cpLen_other_symbs(phy_bw_to_be_used)+numFFT(phy_bw_to_be_used);
symbol2 = signal_buffer(sym2_start:sym2_end);
ofdm_symbol2 = (1/sqrt(numFFT(phy_bw_to_be_used)))*fftshift(fft(symbol2,numFFT(phy_bw_to_be_used))).';
%ofdm_symbol2 = (1/sqrt(numFFT(phy_bw_to_be_used)))*(fft(symbol2,numFFT(phy_bw_to_be_used)));

figure;
plot(0:1:numFFT(phy_bw_to_be_used)-1,10*log10(abs(ofdm_symbol2)),'b-')
title('Symbol #2')

figure;
plot(real(ofdm_symbol2(43:343)),imag(ofdm_symbol2(43:343)),'b*')
title('Constellation Symbol #2')

sym3_start = cpLen_1st_symb(phy_bw_to_be_used)+3*cpLen_other_symbs(phy_bw_to_be_used)+3*numFFT(phy_bw_to_be_used)+1;
sym3_end = cpLen_1st_symb(phy_bw_to_be_used)+3*cpLen_other_symbs(phy_bw_to_be_used)+4*numFFT(phy_bw_to_be_used);
symbol3 = signal_buffer(sym3_start:sym3_end);
ofdm_symbol3 = (1/sqrt(numFFT(phy_bw_to_be_used)))*fftshift(fft(symbol3,numFFT(phy_bw_to_be_used))).';
%ofdm_symbol3 = (1/sqrt(numFFT(phy_bw_to_be_used)))*(fft(symbol3,numFFT(phy_bw_to_be_used)));

figure;
plot(0:1:numFFT(phy_bw_to_be_used)-1,10*log10(abs(ofdm_symbol1)),'b-')
title('Symbol #3')

figure;
plot(real(ofdm_symbol3(43:343)),imag(ofdm_symbol3(43:343)),'b*')
title('Constellation Symbol #3')

sym4_start = cpLen_1st_symb(phy_bw_to_be_used)+4*cpLen_other_symbs(phy_bw_to_be_used)+4*numFFT(phy_bw_to_be_used)+1;
sym4_end = cpLen_1st_symb(phy_bw_to_be_used)+4*cpLen_other_symbs(phy_bw_to_be_used)+5*numFFT(phy_bw_to_be_used);
symbol4 = signal_buffer(sym4_start:sym4_end);
ofdm_symbol4 = (1/sqrt(numFFT(phy_bw_to_be_used)))*fftshift(fft(symbol4,numFFT(phy_bw_to_be_used))).';

figure;
plot(0:1:numFFT(phy_bw_to_be_used)-1,10*log10(abs(ofdm_symbol4)),'b-')
title('Symbol #4')

figure;
plot(real(ofdm_symbol4(43:343)),imag(ofdm_symbol4(43:343)),'b*')
title('Constellation Symbol #4')

sym5_start = cpLen_1st_symb(phy_bw_to_be_used)+5*cpLen_other_symbs(phy_bw_to_be_used)+5*numFFT(phy_bw_to_be_used)+1;
sym5_end = cpLen_1st_symb(phy_bw_to_be_used)+5*cpLen_other_symbs(phy_bw_to_be_used)+6*numFFT(phy_bw_to_be_used);
symbol5 = signal_buffer(sym5_start:sym5_end);
ofdm_symbol5 = (1/sqrt(numFFT(phy_bw_to_be_used)))*fftshift(fft(symbol5,numFFT(phy_bw_to_be_used))).';

figure;
plot(0:1:numFFT(phy_bw_to_be_used)-1,10*log10(abs(ofdm_symbol5)),'b-')
title('Symbol #5')

figure;
plot(real(ofdm_symbol5(43:343)),imag(ofdm_symbol5(43:343)),'b*')
title('Constellation Symbol #5')

sym6_start = cpLen_1st_symb(phy_bw_to_be_used)+6*cpLen_other_symbs(phy_bw_to_be_used)+6*numFFT(phy_bw_to_be_used)+1;
sym6_end = cpLen_1st_symb(phy_bw_to_be_used)+6*cpLen_other_symbs(phy_bw_to_be_used)+7*numFFT(phy_bw_to_be_used);
symbol6 = signal_buffer(sym6_start:sym6_end);
ofdm_symbol6 = (1/sqrt(numFFT(phy_bw_to_be_used)))*fftshift(fft(symbol6,numFFT(phy_bw_to_be_used))).';

figure;
plot(0:1:numFFT(phy_bw_to_be_used)-1,10*log10(abs(ofdm_symbol6)),'b-')
title('Symbol #6')

figure;
plot(real(ofdm_symbol6(43:343)),imag(ofdm_symbol6(43:343)),'b*')
title('Constellation Symbol #6')

sym7_start = cpLen_1st_symb(phy_bw_to_be_used)+7*cpLen_other_symbs(phy_bw_to_be_used)+7*numFFT(phy_bw_to_be_used)+1;
sym7_end = cpLen_1st_symb(phy_bw_to_be_used)+7*cpLen_other_symbs(phy_bw_to_be_used)+8*numFFT(phy_bw_to_be_used);
symbol7 = signal_buffer(sym7_start:sym7_end);
ofdm_symbol7 = (1/sqrt(numFFT(phy_bw_to_be_used)))*fftshift(fft(symbol7,numFFT(phy_bw_to_be_used))).';

figure;
plot(0:1:numFFT(phy_bw_to_be_used)-1,10*log10(abs(ofdm_symbol7)),'b-')
title('Symbol #7')

figure;
plot(real(ofdm_symbol7(43:343)),imag(ofdm_symbol7(43:343)),'b*')
title('Constellation Symbol #7')

sym8_start = cpLen_1st_symb(phy_bw_to_be_used)+8*cpLen_other_symbs(phy_bw_to_be_used)+8*numFFT(phy_bw_to_be_used)+1;
sym8_end = cpLen_1st_symb(phy_bw_to_be_used)+8*cpLen_other_symbs(phy_bw_to_be_used)+9*numFFT(phy_bw_to_be_used);
symbol8 = signal_buffer(sym8_start:sym8_end);
ofdm_symbol8 = (1/sqrt(numFFT(phy_bw_to_be_used)))*fftshift(fft(symbol8,numFFT(phy_bw_to_be_used))).';

figure;
plot(0:1:numFFT(phy_bw_to_be_used)-1,10*log10(abs(ofdm_symbol8)),'b-')
title('Symbol #8')

sym9_start = cpLen_1st_symb(phy_bw_to_be_used)+9*cpLen_other_symbs(phy_bw_to_be_used)+9*numFFT(phy_bw_to_be_used)+1;
sym9_end = cpLen_1st_symb(phy_bw_to_be_used)+9*cpLen_other_symbs(phy_bw_to_be_used)+10*numFFT(phy_bw_to_be_used);
symbol9 = signal_buffer(sym9_start:sym9_end);
ofdm_symbol9 = (1/sqrt(numFFT(phy_bw_to_be_used)))*fftshift(fft(symbol9,numFFT(phy_bw_to_be_used))).';

figure;
plot(0:1:numFFT(phy_bw_to_be_used)-1,10*log10(abs(ofdm_symbol9)),'b-')
title('Symbol #9')

sym10_start = cpLen_1st_symb(phy_bw_to_be_used)+10*cpLen_other_symbs(phy_bw_to_be_used)+10*numFFT(phy_bw_to_be_used)+1;
sym10_end = cpLen_1st_symb(phy_bw_to_be_used)+10*cpLen_other_symbs(phy_bw_to_be_used)+11*numFFT(phy_bw_to_be_used);
symbol10 = signal_buffer(sym10_start:sym10_end);
ofdm_symbol10 = (1/sqrt(numFFT(phy_bw_to_be_used)))*fftshift(fft(symbol10,numFFT(phy_bw_to_be_used))).';

figure;
plot(0:1:numFFT(phy_bw_to_be_used)-1,10*log10(abs(ofdm_symbol10)),'b-')
title('Symbol #10')

sym11_start = cpLen_1st_symb(phy_bw_to_be_used)+11*cpLen_other_symbs(phy_bw_to_be_used)+11*numFFT(phy_bw_to_be_used)+1;
sym11_end = cpLen_1st_symb(phy_bw_to_be_used)+11*cpLen_other_symbs(phy_bw_to_be_used)+12*numFFT(phy_bw_to_be_used);
symbol11 = signal_buffer(sym11_start:sym11_end);
ofdm_symbol11 = (1/sqrt(numFFT(phy_bw_to_be_used)))*fftshift(fft(symbol11,numFFT(phy_bw_to_be_used))).';

figure;
plot(0:1:numFFT(phy_bw_to_be_used)-1,10*log10(abs(ofdm_symbol11)),'b-')
title('Symbol #11')

sym12_start = cpLen_1st_symb(phy_bw_to_be_used)+12*cpLen_other_symbs(phy_bw_to_be_used)+12*numFFT(phy_bw_to_be_used)+1;
sym12_end = cpLen_1st_symb(phy_bw_to_be_used)+12*cpLen_other_symbs(phy_bw_to_be_used)+13*numFFT(phy_bw_to_be_used);
symbol12 = signal_buffer(sym12_start:sym12_end);
ofdm_symbol12 = (1/sqrt(numFFT(phy_bw_to_be_used)))*fftshift(fft(symbol12,numFFT(phy_bw_to_be_used))).';

figure;
plot(0:1:numFFT(phy_bw_to_be_used)-1,10*log10(abs(ofdm_symbol12)),'b-')
title('Symbol #12')

sym13_start = cpLen_1st_symb(phy_bw_to_be_used)+13*cpLen_other_symbs(phy_bw_to_be_used)+13*numFFT(phy_bw_to_be_used)+1;
sym13_end = cpLen_1st_symb(phy_bw_to_be_used)+13*cpLen_other_symbs(phy_bw_to_be_used)+14*numFFT(phy_bw_to_be_used);
symbol13 = signal_buffer(sym13_start:sym13_end);
ofdm_symbol13 = (1/sqrt(numFFT(phy_bw_to_be_used)))*fftshift(fft(symbol13,numFFT(phy_bw_to_be_used))).';

figure;
plot(0:1:numFFT(phy_bw_to_be_used)-1,10*log10(abs(ofdm_symbol13)),'b-')
title('Symbol #13')

if(0)
    hFig = figure('Position', figposition([1.5*46 1.5*50 1.5*30 1.5*30]), 'MenuBar', 'none');
    %axis([-0.5 0.5 -50 0]);
    hold on;
    grid on
    xlabel('Normalized frequency');
    ylabel('PSD (dBW/Hz)')
    [psd_ofdm,f_ofdm] = periodogram(signal_buffer, rectwin(length(signal_buffer)), numFFT(phy_bw_to_be_used)*2, 1, 'centered');
    plot(f_ofdm,10*log10(psd_ofdm));
end

signal_buffer = signal_buffer.';
