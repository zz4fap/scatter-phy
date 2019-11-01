
numFFT_1_08 = 72;                                       % Number of FFT points for 1.08 MHz band (1.08MHz / 15KHz = 72)
numFFT_1_44 = 96;                                       % Number of FFT points for 1.44 MHz band (1.44MHz / 15KHz = 96)
bitsPerSubCarrier = 2;                                  % 2: QPSK, 4: 16QAM, 6: 64QAM, 8: 256QAM

McF_coe_file_path  = '/home/mmehari/McF.coe';
McF_bin_file_path  = '/home/mmehari/McF.bin';

plotPSD = 0;                                            % Plot or do not plot power spectral density (PSD)

symRate = 15;                                           % OFDM symbol rate per msec interval
chan_cnt = 8;                                           % number of virtual channels to use (1 chan = 1.44 Msps)

% QAM Symbol mapper.
qamMapper = comm.RectangularQAMModulator('ModulationOrder', 2^bitsPerSubCarrier, 'BitInput', true, 'NormalizationMethod', 'Average power');

McF_coe_fd  = fopen(McF_coe_file_path,  'w');
McF_bin_fd  = fopen(McF_bin_file_path,  'w');

fprintf(McF_coe_fd, 'memory_initialization_radix=16;\nmemory_initialization_vector=\n');
for i = 1:symRate

    % Generate McF symbol
    McF_symbol = zeros(chan_cnt * numFFT_1_44, 1);
    for j = 1:chan_cnt
        % Generate channel symbol
        bitsIn = randi([0 1], bitsPerSubCarrier*numFFT_1_08, 1);

        symbolIn = qamMapper(bitsIn);

        % Pack data into a symbol 
        offset = (numFFT_1_44-numFFT_1_08)/2;

        Idx = (j-1)*numFFT_1_44 + 1;
        chan_symbol = [zeros(offset,1); symbolIn; zeros(offset,1)];
        McF_symbol(Idx : Idx + numFFT_1_44 - 1) = chan_symbol;
    end
    
    % Convert McF symbol into IQ (fixed point integer) samples
    McF_IQ = ifft(ifftshift(McF_symbol), length(McF_symbol));
    McF_IQ_fi = fi(McF_IQ,1,16,15);

    % Store McF (fixed point) IQ samples into a file
    for j = 1:length(McF_IQ_fi)
        I = typecast(real(McF_IQ_fi.int(j)), 'uint16');
        Q = typecast(imag(McF_IQ_fi.int(j)), 'uint16');
        if (i == symRate && j == length(McF_IQ_fi))
            fprintf(McF_coe_fd, '%04x%04x;\n', I, Q);
        else
            fprintf(McF_coe_fd, '%04x%04x,\n', I, Q);
        end
    end

    % Store McF (floating point) IQ samples into a file    
    for j = 1:length(McF_IQ)
        fwrite(McF_bin_fd, real(McF_IQ(j)), 'float');
        fwrite(McF_bin_fd, imag(McF_IQ(j)), 'float');
    end
    
    if(plotPSD)
        % Plot power spectral density (PSD)
        signal2plot = McF_IQ_fi;
        hFig = figure('Position', figposition([1.5*46 1.5*50 1.5*30 1.5*30]), 'MenuBar', 'none');
        axis([-0.5 0.5 -230 10]);
        hold on; 
        grid on
        xlabel('Normalized frequency');
        ylabel('PSD (dBW/Hz)')
        [psd,f] = periodogram(signal2plot, rectwin(length(signal2plot)), numFFT_1_44*2, 1, 'centered'); 
        plot(f,10*log10(psd));
    end
 end
fclose(McF_coe_fd);
fclose(McF_bin_fd);

