function [ output, error ] = NLMS( waveform, desired, stepsize )
%NLMS: Normalized Least Means Squared
%   
    L = length(waveform);
    output = zeros(L);
    error = zeros(L);
    
    for i = 2:L
        error[i-1] = desired[i-1] - output[i-1];
        output[i] = output[i-1] + (stepsize * error[i-1] * waveform[i-1])/(waveform[i-1] * waveform[i-1]);
    end
end

