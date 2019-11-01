clear all;close all;clc

M = 32;

counter = 0;
for Tcme=0:0.0002:2
    part1 = exp(-1*(M*Tcme));
    k = 0:1:M-1;
    part2 = sum((1./factorial(k)).*(M.*Tcme).^k);
    counter = counter + 1;
    res(counter) = part1*part2;
end

plot(0:0.0002:2, res)

% detection_window_size = 1;
% Tcme = 6.9078;

% detection_window_size = 2;
% Tcme = 4.6168; 

% detection_window_size = 13;
% Tcme = 2.079;

% detection_window_size = 16;
% Tcme = 1.9528; 

% detection_window_size = 32;
% Tcme = 1.6362;