clear all;close all;clc

numIter = 1e4;

% Generate PSS signal.
pss = lte_pss_zc(0);

pss_local = pss;

Nzc = 62;

Pfa = 0.0001;
Pfd = 0.001;

%detection_window_size = 16;
%Tcme = 1.9528; % for 16 samples

% detection_window_size = 2;
% Tcme = 4.6168; % for 2 samples

% detection_window_size = 32;
% Tcme = 1.6362; % for 16 samples

detection_window_size = 1;
Tcme = 6.9078; % for 16 samples

M = detection_window_size;

SNR = 1000; %-20:2:4;

correct_detection = zeros(1, length(SNR));
false_rejection = zeros(1, length(SNR));
for snr_idx = 1:1:length(SNR)
    
    detection_counter = 0;
    false_rejection_counter = 0;
    for trial=1:1:numIter
        
        rx_pss = awgn(pss, SNR(snr_idx), 'measured');
   
        % Multiplication in the frequency domain. (Point by point multiplication)
        mult = (rx_pss).*conj(pss_local);
        
        % Squared modulus used for peak detection.
        pdp_detection = abs(ifftshift(ifft(mult, Nzc))).^2;
        
        pdp_detection_aux = pdp_detection;
        
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
        for I = 10:1:Nzc-1
            limiar = -log(Pfd)/I;
            if(pdp_detection(I+1) > limiar*sum(pdp_detection(1:I)))
                break;
            end
        end
        
        % Output of the disposal module. It's used as a noise reference.
        Zref = sum(pdp_detection(1:I));
        
        % Calculate the scale factor.
        alpha = finv((1-Pfa),(2*M),(2*I))/(I/M);
        
        ratio = (pdp_detection_aux((Nzc/2) + 1))/Zref;
        if(ratio >= alpha)
            detection_counter = detection_counter + 1;
        else
            false_rejection_counter = false_rejection_counter + 1;
        end

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
legend('Correct detection','False rejection')
