function [x,y] = plot_spec(s, sampling_rate, num_points, varargin)
s = s(:);
% num_points = 2048;
orig_input_len = length(s);
freq_resolution = sampling_rate/num_points;
% s = vec2mat(s, num_points).';
num_row = num_points;
tmp = ceil(length(s)/num_row);
tmp = tmp*num_row;
s = [s; zeros(tmp-length(s),1)];
num_col = length(s)/num_row;
s = reshape(s, [num_row, num_col]);
num_reset = size(s,1)*size(s,2) - orig_input_len;
s((end-num_reset+1):end,end) = s(1:num_reset,1);
f = 10.*log10(mean(abs(fft(s)).^2,2));
% f = mean(abs(fft(s)).^2,2);
f = [f(((num_points/2)+1) : end); f(1:(num_points/2))];
x = (-(num_points/2):((num_points/2)-1)).'.*freq_resolution;
y = f;
if nargin == 4
    format_str = varargin{1};
    plot(x./1e3, y, format_str); xlabel('kHz'); ylabel('dB'); grid on;
    max_dB = 60;
    if sampling_rate == 3e6 % plot bins
      bin_boundary = -1500:100:(1500-100);
      hold on;
      for i = 1:length(bin_boundary)
          plot([bin_boundary(i) bin_boundary(i)],[0, max_dB],'r');
          text(bin_boundary(i)+25,max_dB,num2str(i-1));
      end
    end
end
