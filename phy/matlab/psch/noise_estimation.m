clear all;close all;clc

LEN = 22;

a = 2.3;

noise = (sqrt(a))*(1/(sqrt(2)))*complex(randn(LEN,1),randn(LEN,1));

noise_est = (noise'*noise)/LEN;



