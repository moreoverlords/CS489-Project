function [ antinoise, error ] = NLMS( input, ref, mu, delay, order )
% noise cancellation
% from https://www.clear.rice.edu/elec301/Projects00/site/code.html

% inputs:
%   input   = input signal, desired + noise
%   ref     = measurement of only noise
%   mu      = learning rate
%   delay   = simulates delay of antinoise calculation by a number of samples
%   order   = number of taps

% outputs:
%   antinoise = signal used to cancel noise
%   error = resulting signal w/ noise cancelled

%order=5;

fs=44100;                                %digital sampling frequency
N=length(input);                             %size of inputs
size=N/fs;                   %time duration of inputs
t=linspace(0, size, N);
%f1=440;                                %frequency of voice
%f2=500;                                %frequency of noise

graph_range = N;%4410;
%voice=cos(2*pi*f1*t);
%subplot(5,1,1)
%plot(t,voice);
%title('voice    (don''t have access to)')
%audiowrite('voice.wav', voice, fs);

%noise=cos(2*pi*f2*t.^2);                       %frequency sweep noise
%noise=(rand(1,length(voice))-.5);            %white noise
%noise=cos(2*pi*f2*t.^2)+cos(2*pi*t*((f2+200)/t));                       %2-frequency sweeps noise
%input=voice+noise;
subplot(4,1,1)
plot(t(1:graph_range),input(1:graph_range))
title('input = signal + noise   (interior mic)')
audiowrite('input.wav', input, fs);

%ref=noise +.25*(rand-0.5);                       %noisy noise
subplot(4,1,2)
plot(t(1:graph_range),ref(1:graph_range))
title('reference  (noisy measurement of noise)   (exterior mic)');
audiowrite('reference.wav', ref, fs);

w=zeros(order,1);
error = zeros(N-order, 1);
antinoise = zeros(N-order, 1);
%delay = 0;
%mu=.006;
for i=1:N-order
   buffer = ref(i:i+order-1);                  
   antinoise(i) = buffer*w;
   if i > delay
        error(i) = input(i)-antinoise(i-delay);      
   else 
        error(i) = input(i)-antinoise(i);            
   end
   w=w+(buffer.*mu*error(i)/(buffer*buffer'))';  
end

subplot(4,1,3)
plot(t(order+1:graph_range),-antinoise(1:graph_range-order))
title('Anti-noise (added to isolate signal)')

audiowrite('antinoise.wav', antinoise, fs);

subplot(4,1,4)
plot(t(order+1:graph_range),error(1:graph_range-order))
title('Adaptive output')

audiowrite('error.wav', error, fs);