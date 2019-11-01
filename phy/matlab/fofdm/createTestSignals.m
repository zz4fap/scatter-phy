clear all;close all;clc


load('txSigOFDMpadding_input.mat');
load('txSigFOFDM_output.mat');


for i=1:1:length(txSigOFDMpadding)
    if(i < length(txSigOFDMpadding))
        fprintf(1,'%e,\t',txSigOFDMpadding(i));
    else
        fprintf(1,'%e\n',txSigOFDMpadding(i));
    end
    if(mod(i,5)==0)
        fprintf(1,'\n');
    end
end


fprintf(1,'\n\n\n\n');

for i=1:1:length(txSigFOFDM)
    if(i < length(txSigFOFDM))
        fprintf(1,'%e,\t',txSigFOFDM(i));
    else
        fprintf(1,'%e\n',txSigFOFDM(i));
    end
    if(mod(i,5)==0)
        fprintf(1,'\n');
    end
end