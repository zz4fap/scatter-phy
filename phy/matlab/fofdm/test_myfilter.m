clear all;close all;clc

numTrials = 10000;
for trials=1:1:numTrials

N = randi(1000);
input = zeros(1,N);
input(1) = 1;

M = randi(1000);
coeffs = randn(M,1);

output = myFilterv2(coeffs, input);

if(length(coeffs) < length(input))
    final = length(coeffs);
else
    final = length(input);
end

error = sum(abs(output(1:final) - coeffs(1:final)))/length(coeffs);

if(error > 0)
    fprintf(1,'Error: %f\n',error);
end 

end