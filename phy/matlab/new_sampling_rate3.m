clear all;close all;clc

NFFT = 2048; % for 20 MHz BW.
num_of_ofdm_symbols = 7;

slot_duration = 0.5e-3;
subframe_duration = 1e-3; % 1 milisecond.

for Ncp=1:1:1000
    delta_f = (num_of_ofdm_symbols*(Ncp+NFFT)) / (slot_duration*NFFT);
    Ts = (1/(delta_f*NFFT));
    Tcp = Ncp*Ts;
    fprintf(1,'Ncp: %d  - delta_f %f - Tcp: %d\n',Ncp,delta_f,Tcp);
    
    res=mod(delta_f,1);
    if(res==0 && Tcp > 5.12e-6)
        break;
    end
end

total_cp_duration = num_of_ofdm_symbols*Ncp*Ts;
total_symbol_duration = num_of_ofdm_symbols*NFFT*Ts;

total_cp_duration+total_symbol_duration