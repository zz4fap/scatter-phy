function [error] = calculateError(txSigFOFDM,txSigFOFDM2)

error = 0;
for i=1:1:length(txSigFOFDM)
    local_error = abs(txSigFOFDM(i)-txSigFOFDM2(i));
    error = error + local_error;
    if(local_error > 1e-14) 
        fprintf(1,'local_error: %f\n',local_error)
    end
end
error = error/length(txSigFOFDM);

end