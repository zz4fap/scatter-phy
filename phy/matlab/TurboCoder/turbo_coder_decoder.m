clear all;close all;clc;

txBits = randi([0 1],6144,1);
codedData = lteTurboEncode(txBits);
txSymbols = lteSymbolModulate(codedData,'QPSK');
noise = 0.5*complex(randn(size(txSymbols)),randn(size(txSymbols)));
rxSymbols = txSymbols + noise;

scatter(real(rxSymbols),imag(rxSymbols),'co'); 
hold on;

scatter(real(txSymbols),imag(txSymbols),'rx')
legend('Rx constellation','Tx constellation')

softBits = lteSymbolDemodulate(rxSymbols,'QPSK','Soft');
rxBits = lteTurboDecode(softBits);
numberErrors = sum(rxBits ~= int8(txBits))