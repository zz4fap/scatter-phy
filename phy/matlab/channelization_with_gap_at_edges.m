clear all;close all;clc;

competition_bw = 10e6;

phy_bandwidth = 5e6;

center_frequency = 2e9;

for channel=0:1:floor(competition_bw/phy_bandwidth)-1
    
    if(channel == 0) 
        channel_center_frequency = center_frequency - (250e3 + ((phy_bandwidth-500e3)/2.0));
    elseif(channel == 1) 
        channel_center_frequency = center_frequency + (250e3 + ((phy_bandwidth-500e3)/2.0));
    end
    
    fprintf(1,'channel: %d - channel_center_frequency: %f [MHz]\n', channel, channel_center_frequency);
    
end


