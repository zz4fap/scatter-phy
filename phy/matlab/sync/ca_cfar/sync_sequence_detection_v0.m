clear all;close all;clc

numIter = 1e3;

u = 129;
Nzc = 839;
NIFFT = 2048;
sc_offset = 527;
v = [10];
Ncs = 13;

% Generate local Zadoff-Chu sequence.
n = [0:1:(Nzc-1)];
xu_root = exp(-1i*(pi*u.*n.*(n+1))./Nzc);
signal = complex(0,0)*zeros(1,NIFFT);
xuv = zeros(1,Nzc);
for i=1:1:length(v)
    Cv = v(i)*Ncs;
    xuv = xuv + xu_root(mod((n+Cv),Nzc)+1);
end
Xuv = fft(xuv,Nzc);

% Generate local Zadoff-Chu sequence.
Xu_root = fft(xu_root, Nzc);

Pfa = 0.0001;
Pfd = 0.001;

trials_counter = 0;
trials_counter2 = 0;
false_alarm_counter = 0;
correct_rejection_counter = 0;
ratio_dist_counter = 0;
ratio_dist = zeros(1,100000);
detection_counter = 0;
false_rejection_counter = 0;

%detection_window_size = 16;
%Tcme = 1.9528; % for 16 samples

% detection_window_size = 2;
% Tcme = 4.6168; % for 2 samples

% detection_window_size = 32;
% Tcme = 1.6362; % for 16 samples

detection_window_size = 1;
Tcme = 6.9078; % for 16 samples

M = detection_window_size;

SNR = 10000; %-4:2:4;

correct_detection = zeros(1, length(SNR));
false_rejection = zeros(1, length(SNR));
for snr_idx = 1:1:length(SNR)
    
    detection_counter = 0;
    false_rejection_counter = 0;
    for trial=1:1:numIter
        
        Xuv_rx = awgn(Xuv, SNR(snr_idx), 'measured');
   
        % Multiplication in the frequency domain. (Point by point multiplication)
        Xu_root = (Xuv_rx).*conj(Xu_root);
        
        % Squared modulus used for peak detection.
        pdp_detection = abs(ifft(Xu_root, Nzc)).^2;
        
        %stem(0:1:Nzc-1,pdp_detection)
        
        % This is the implemenatation of the algorithm proposed in [3]
        % Sort Segmented PDP vector in increasing order of energy.
        for i=1:1:length(pdp_detection)
            for k=1:1:length(pdp_detection)-1
                if(pdp_detection(k)>pdp_detection(k+1))
                    temp = pdp_detection(k+1);
                    pdp_detection(k+1) = pdp_detection(k);
                    pdp_detection(k) = temp;
                end
            end
        end
        
        % Algorithm used to discard samples. Indeed, it is used to find a
        % speration between noisy samples and signal+noise samples.
        for I = 20:1:Nzc-1
            limiar = -log(Pfd)/I;
            if(pdp_detection(I+1) > limiar*sum(pdp_detection(1:I)))
                break;
            end
        end
        
        % Output of the disposal module. It's used as a noise reference.
        Zref = sum(pdp_detection(1:I));
        
        % Calculate the scale factor.
        alpha = finv((1-Pfa),(2*M),(2*I))/(I/M);
        
        ratio = (pdp_detection(710))/Zref;
        if(ratio >= alpha)
            detection_counter = detection_counter + 1;
        else
            false_rejection_counter = false_rejection_counter + 1;
        end

        
        %     for j=1:1:Nzc
        %         trials_counter = trials_counter + 1;
        %         ratio = segmented_pdp(j)/Zref;
        %         if(ratio > alpha)
        %             false_alarm_counter = false_alarm_counter + 1;
        %         else
        %             correct_rejection_counter = correct_rejection_counter + 1;
        %         end
        %     end
        %     false_alarm_rate = false_alarm_counter/trials_counter;
        %     correct_rejection_rate = correct_rejection_counter/trials_counter;
        %
        %     ratio_dist_counter = ratio_dist_counter+1;
        %     ratio_dist(ratio_dist_counter) = segmented_pdp(100)/Zref;
    end
    
    correct_detection(snr_idx) = detection_counter/numIter;
    false_rejection(snr_idx) = false_rejection_counter/numIter;
    
    
end

figure;
plot(SNR, correct_detection, 'LineWidth', 1)
hold on
plot(SNR, false_rejection, 'LineWidth', 1)
hold off
grid on

