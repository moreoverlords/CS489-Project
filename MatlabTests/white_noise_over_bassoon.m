% noise cancellation
% from https://www.clear.rice.edu/elec301/Projects00/site/code.html
clear
close all

order=5;

voice = 2*audioread('TestWavs/Bassoon.mf.C3B3.aiff')';

fs=44100;                                %digital sampling frequency
size=length(voice)/fs;                         %time duration of inputs
t=[0:1/fs:size];
N=fs*size;                      %size of inputs
f1=440;                                %frequency of voice
f2=500;                                %frequency of noise

noise = (rand(1,length(voice))-.5);

%noise=cos(2*pi*f2*t.^2);                       %frequency sweep noise
%noise=cos(2*pi*f2*t.^2)+cos(2*pi*t*((f2+200)/t));                       %2-frequency sweeps noise
input=voice+noise;

ref=noise +.25*(rand(1,length(voice))-0.5);                       %noisy noise

[antinoise, error] = NLMS(input, ref, .006, 0, 5);

%plot(t(order:N), error)
%title('Adaptive output (over full duration)')