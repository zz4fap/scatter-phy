clear all;close all; clc

out = load('~/Downloads/fir_output_michael.txt');

real_s = out(:,2)./32768;

imag_s = out(:,3)./32768;

fid = fopen('./fir_output_converted_into_float.txt','w');


for i=1:1:length(imag_s)

    value = [real_s(i); imag_s(i)];
    
    fwrite(fid, value, 'float');

end


fclose(fid);

