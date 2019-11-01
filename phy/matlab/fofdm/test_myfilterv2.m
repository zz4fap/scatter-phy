clear all;close all;clc

load('tx_filter_25rb.mat');

load('rx_filter_25rb.mat');


N = 384+27;
txSigOFDM = randn(N,1);

L = length(FirTxFilter);

txSIgOFDMPadded = [txSigOFDM; zeros(L-1,1)];

txFOFDM = myFilter(FirTxFilter, txSIgOFDMPadded);

rxFOFDM = myFilter(FirTxFilter, txFOFDM);

rxFOFDM2 = rxFOFDM(L:end);

error1 = 0;
for i=1:1:length(txSigOFDM)
    
    error1 = error1 + (txSigOFDM(i) - rxFOFDM2(i));
    
end
error1 = error1/length(txSigOFDM);

error = 0;
for i=1:1:length(FirRxFilter)
    
    error = error + (FirTxFilter(i) - FirRxFilter(i));
    
end
error = error/length(FirRxFilter);

sig = txFOFDM;
for i=1:1:length(sig)
    if(i < length(sig))
        fprintf(1,'%e,\t',sig(i));
    else
        fprintf(1,'%e\n',sig(i));
    end
    if(mod(i,5)==0)
        fprintf(1,'\n');
    end
end
