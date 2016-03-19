% noise cancellation
% from https://www.clear.rice.edu/elec301/Projects00/site/code.html
clear
close all

order=2;

size=2;                         %time duration of inputs
fs=44100;                                %digital sampling frequency
t=[0:1/fs:size];
N=fs*size;                      %size of inputs
f1=440;                                %frequency of voice
f2=500;                                %frequency of noise

voice=cos(2*pi*f1*t);
subplot(4,1,1)
plot(t,voice);
title('voice    (don''t have access to)')
audiowrite('voice.wav', voice, fs);

noise=cos(2*pi*f2*t.^2);                       %frequency sweep noise
%noise=.1*(rand(1,length(voice))-.5);            %white noise
primary=voice+noise;
subplot(4,1,2)
plot(t,primary)
title('primary = voice + noise   (input1)')
audiowrite('primary.wav', primary, fs);

ref=noise+.25*(rand-0.5);                       %noisy noise
subplot(4,1,3)
plot(t,ref)
title('reference  (noisy noise)   (input2)');
audiowrite('reference.wav', ref, fs);

w=zeros(order,1);
mu=.006;
for i=1:N-order
   buffer = ref(i:i+order-1);                   %current 32 points of reference
   desired(i) = primary(i)-buffer*w;            %dot product reference and coeffs
   w=w+(buffer.*mu*desired(i)/norm(buffer))';   %update coeffs
end

subplot(4,1,4)
plot(t(order+1:N),desired)
title('Adaptive output  (hopefully it''s close to "voice")')

audiowrite('desired.wav', desired, fs);