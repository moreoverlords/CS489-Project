% noise cancellation
% from https://www.clear.rice.edu/elec301/Projects00/site/code.html
clear
close all

order=5;

size=2;                         %time duration of inputs
fs=44100;                                %digital sampling frequency
t=[0:1/fs:size];
N=fs*size;                      %size of inputs
f1=440;                                %frequency of voice
f2=500;                                %frequency of noise

voice=cos(2*pi*f1*t);
subplot(5,1,1)
plot(t,voice);
title('voice    (don''t have access to)')
audiowrite('voice.wav', voice, fs);

%noise=cos(2*pi*f2*t.^2);                       %frequency sweep noise
noise=(rand(1,length(voice))-.5);            %white noise
%noise=cos(2*pi*f2*t.^2)+cos(2*pi*t*((f2+200)/t));                       %2-frequency sweeps noise
input=voice+noise;
subplot(5,1,2)
plot(t,input)
title('input = voice + noise   (input1)')
audiowrite('input.wav', input, fs);

ref=noise +.25*(rand-0.5);                       %noisy noise
subplot(5,1,3)
plot(t,ref)
title('reference  (noisy noise)   (input2)');
audiowrite('reference.wav', ref, fs);

w=zeros(order,1);
error = zeros(N-order, 1);
antinoise = zeros(N-order, 1);
delay = 0;
mu=.006;
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

subplot(5,1,4)
plot(t(order+1:N),antinoise)
title('Anti-noise (added to isolate voice)')

audiowrite('antinoise.wav', antinoise, fs);

subplot(5,1,5)
plot(t(order+1:N),error)
title('Adaptive output  (hopefully it''s close to "voice")')

audiowrite('error.wav', error, fs);