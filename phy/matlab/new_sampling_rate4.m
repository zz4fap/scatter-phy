clear all;close all;clc

num_of_ofdm_symbols = 7;

slot_duration = 0.5e-3;
subframe_duration = 1e-3; % 1 milisecond.

for NFFT=1:1:2048
    for Ncp=1:1:1000
        delta_f = (num_of_ofdm_symbols*(Ncp+NFFT)) / (slot_duration*NFFT);
        Ts = (1/(delta_f*NFFT));
        Tcp = Ncp*Ts;
        
        res=mod(delta_f,1);
        if(res==0 && Tcp > 5.12e-6)
            fprintf(1,'NFFT: %d - Ncp: %d - delta_f %f - Tcp: %d - Samp. Rate: %d\n',NFFT,Ncp,delta_f,Tcp,(NFFT*delta_f));
            break;
        end
    end
end

total_cp_duration = num_of_ofdm_symbols*Ncp*Ts;
total_symbol_duration = num_of_ofdm_symbols*NFFT*Ts;

total_cp_duration+total_symbol_duration