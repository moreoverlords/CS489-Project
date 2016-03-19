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

%noise=cos(2*pi*f2*t.^2);                       %frequency sweep noise
noise=(rand(1,length(voice))-.5);            %white noise
%noise=cos(2*pi*f2*t.^2)+cos(2*pi*t*((f2+200)/t));                       %2-frequency sweeps noise
input=voice+noise;

ref=noise +.25*(rand-0.5);                       %noisy noise

[antinoise, error] = NLMS(input, ref, .006, 0, 50);