clear all;close all;clc

LEN = 31;

NID1 = 0;

NID2 = 10;

q_prime = floor(NID1/30);

q = floor(((NID1+q_prime*(q_prime+1)/2))/30);

m_prime = NID1 + q *(q+1)/2;

m0 = mod(m_prime, LEN);

m1 = mod(m0 + floor(m_prime/LEN)+1,LEN);



%%%%%%%%%%%%%%%%% generate d_even() sequence %%%%%%%%%%%%%%%%

% Generate the sequence x_s() : x() for calculating s_tilda()

x_s = zeros(1,LEN);

x_s(1:5) = [0 0 0 0 1];

 

for i = 0:25

    x_s((i+5)+1) = mod(x_s(i+2+1)+x_s(i+1),2);

end

 

% Generate the sequence x_c() : x() for calculating c_tilda()

x_c = zeros(1,LEN);

x_c(1:5) = [0 0 0 0 1];

 

for i = 0:25

    x_c((i+5)+1) = mod(x_c(i+3+1)+x_c(i+1),2);

end

 

% Generate the sequence s_tilda()

s_tilda = zeros(1,LEN);

for i = 0:30

    s_tilda(i+1) = 1 - 2*x_s(i+1);

end

 

% Generate the sequence c_tilda()

c_tilda = zeros(1,LEN);

for i = 0:30

    c_tilda(i+1) = 1 - 2*x_c(i+1);

end

 

% Generate s0_m0_even()

s0_m0_even = zeros(1,LEN);

for n = 0:30

    s0_m0_even(n+1) = s_tilda(mod(n+m0,LEN)+1);

end

 

% Generate c0_even()

c0_even = zeros(1,LEN);

for n = 0:30

    c0_even(n+1) = c_tilda(mod(n+NID2,LEN)+1);

end

 

% Calculate d_even_sub0

d_even_sub0 = s0_m0_even .* c0_even;

 

%%%%%%%%%%%%%%%%% generate d_odd() sequence %%%%%%%%%%%%%%%%

% Generate the sequence x_s() : x() for calculating s_tilda()

x_z = zeros(1,LEN);

x_z(1:5) = [0 0 0 0 1];

 

for i = 0:25

    x_z((i+5)+1) = mod(x_z(i+4+1) + x_z(i+2+1) + x_z(i+1+1)+ x_z(i+1),2);

end

 

% Generate the sequence z_tilda()

z_tilda = zeros(1,LEN);

for i = 0:30

    z_tilda(i+1) = 1 - 2*x_z(i+1);

end

 

% Generate s1_m1_odd()

s1_m1_odd = zeros(1,LEN);

for n = 0:30

    s1_m1_odd(n+1) = s_tilda(mod(n+m1,LEN)+1);

end

 

% Generate c1_odd()

c1_odd = zeros(1,LEN);

for n = 0:30

    c1_odd(n+1) = c_tilda(mod(n+NID2+3,LEN)+1);

end

 

% Generate z1_m0_odd()

z1_m0_odd = zeros(1,LEN);

for n = 0:30

    z1_m0_odd(n+1) = z_tilda(mod(n+mod(m0,8),LEN)+1);

end

 

% Calculate d_odd_sub0

d_odd_sub0 = s1_m1_odd .* c1_odd .* z1_m0_odd;

 

% Calculate d_sub0

d_sub0 = zeros(1,62);

d_sub0(1:2:end-1) = d_even_sub0;

d_sub0(2:2:end) = d_odd_sub0;

 

subplot(1,5,1);

plot(real(d_sub0),imag(d_sub0),'ro','MarkerFaceColor',[1 0 1]);axis([-1.5 1.5 -1.5 1.5]);

subplot(1,5,[2 5]);




plot(real(d_sub0),'ro-');xlim([0 length(d_sub0)]);