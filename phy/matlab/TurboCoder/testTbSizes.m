clear all;close all;clc

tbLen = 19848% 19336; %
[C, Kplus, validK] = CblkSegParams(tbLen);

% Total number of bits in subframe #5
% 4464 - 1.4 MHz
% 13104 - 3 MHz
% 22704 - 5 MHz