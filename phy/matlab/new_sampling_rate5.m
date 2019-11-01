clear all;close all;clc

num_of_ofdm_symbols = 7;

slot_duration = 0.5e-3;
subframe_duration = 1e-3; % 1 milisecond.

for delta_f=15100
    for Ncp=1:1:1000
    %for Ncp=121
        NFFT = (num_of_ofdm_symbols*Ncp) / ((slot_duration*delta_f) - num_of_ofdm_symbols);
        Ts = (1/(delta_f*NFFT));
        Tcp = Ncp*Ts;
        
        modv = mod(NFFT,1);
        if(modv<1e-10)
            fprintf(1,'NFFT: %f - Ncp: %d - delta_f %f - Tcp: %d - Samp. Rate: %d\n',NFFT,Ncp,delta_f,Tcp,(NFFT*delta_f));
        end
        
    end
end

total_cp_duration = num_of_ofdm_symbols*Ncp*Ts;
total_symbol_duration = num_of_ofdm_symbols*NFFT*Ts;

total_cp_duration+total_symbol_duration