clear all;close all;clc

numIter = 1e3;

SNR = -20:2:10;

% Generate PSS signal.
pss = lte_pss_zc(0);

sigPower = sum(abs(pss(:)).^2)/length(pss(:));
sigPower = 10*log10(sigPower);

noisePower = sigPower-SNR;

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

false_detection = zeros(1, length(SNR));
correct_rejection = zeros(1, length(SNR));
for snr_idx = 1:1:length(SNR)
    
    false_detection_counter = 0;
    correct_rejection_counter = 0;
    for trial=1:1:numIter
        
        rx_pss = wgn(Nzc, 1, noisePower(snr_idx), 'complex');
   
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
        
        ratio = (pdp_detection_aux((Nzc/2) + 1))/Zref;
        if(ratio >= alpha)
            false_detection_counter = false_detection_counter + 1;
        else
            correct_rejection_counter = correct_rejection_counter + 1;
        end

    end
    
    false_detection(snr_idx) = false_detection_counter/numIter;
    correct_rejection(snr_idx) = correct_rejection_counter/numIter;
    
end

figure;
plot(SNR, false_detection, 'LineWidth', 1)
hold on
plot(SNR, correct_rejection, 'LineWidth', 1)
hold off
grid on
legend('False detection','Correct rejection')

