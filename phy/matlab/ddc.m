function a = ddc(s, sampling_rate_in, sampling_rate_out, fir_coef, fo)
t = (0:(length(s)-1))./sampling_rate_in;
t = t.';
lo = exp(1i.*2.*pi.*fo.*t);
a = s.*lo;
a = conv(a, fir_coef);
a = downsample(a, sampling_rate_in/sampling_rate_out);
