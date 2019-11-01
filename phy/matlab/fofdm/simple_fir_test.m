clear all;close all;clc

L = 16;

coeffs = [0:1:L-1]';

input = complex([0:1:15]',[0:1:15]');

% Zero-padding to flush tail.
inputPadded = [input; input; zeros(L,1)];

% Apply implemented filter.
output = myFilterv2(coeffs, inputPadded);

output1 = filter(coeffs,1,inputPadded);

error = calculateError(output,output1);

output_real = myFilterv2(coeffs, real(inputPadded));

output_imag = myFilterv2(coeffs, imag(inputPadded));

output_split = complex(output_real,output_imag);

error = calculateError(output_split,output1);

error = calculateError(output_split,output);

signalToPrint = output;
is_complex = 1;
if(is_complex==1)
    for i=1:1:length(signalToPrint)
        if(i < length(signalToPrint))
            if(imag(signalToPrint(i)) < 0)
                fprintf(1,'%1.4e %1.4ei,\t',real(signalToPrint(i)),imag(signalToPrint(i)));
            else
                fprintf(1,'%1.4e +%1.4ei,\t',real(signalToPrint(i)),imag(signalToPrint(i)));
            end
        else
            if(imag(signalToPrint(i)) < 0)
                fprintf(1,'%1.4e %1.4ei\n',real(signalToPrint(i)),imag(signalToPrint(i)));
            else
               fprintf(1,'%1.4e +%1.4ei\n',real(signalToPrint(i)),imag(signalToPrint(i))); 
            end
        end
        if(mod(i,5)==0)
            fprintf(1,'\n');
        end
    end
else
    for i=1:1:length(signalToPrint)
        if(i < length(signalToPrint))
            fprintf(1,'%1.4e,\t',signalToPrint(i));
        else
            fprintf(1,'%1.4e\n',signalToPrint(i));
        end
        if(mod(i,5)==0)
            fprintf(1,'\n');
        end
    end
end