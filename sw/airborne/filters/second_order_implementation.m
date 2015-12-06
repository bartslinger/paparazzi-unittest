%% Second order filter unittest
clear all; clc;

%% Construction of a specific second order discrete filter

% Global variables
Ts = 1/512;
MAX_PPRZ = 9600;

% Continuous filter construction
omega = 1/0.025;
zeta = 1;
s = tf('s');
H = omega^2/(s^2 + 2*zeta*omega*s + omega^2);

% Discretization step
sys = ss(H);
dsys = c2d(sys, Ts);

% Convert system so that C contains a 1, make the state vector 6.25 times
% bigger
dsys.b = dsys.b * dsys.c(2);
dsys.c = dsys.c ./ dsys.c(2);

% This can be verified with:
if 0
    figure(1); hold off;
    step(H); hold on;
    step(dsys);
end

    

%% Construct input signal
t = 0:Ts:0.25;
u = MAX_PPRZ*ones(size(t));
u(1) = 0;
y = zeros(size(t));
x = [0;0];

Ashift = 2^14;
Bshift = 2^18;

A = int16(dsys.a*Ashift);
B = int16(dsys.b*Bshift);
C = dsys.c;
D = dsys.d;

% Stupid matlab cannot do multiplications with integers, so convert back to
% doubles
A = double(A);
B = double(B);
%% Construct signal response to discrete system

for i=1:length(t)-1
        
    stateincrement = double(idivide(int32(A * x),int32(Ashift), 'floor'));
    inputpart = double(idivide(int32(B * u(i)), int32(Bshift), 'floor'));
    x = stateincrement + inputpart;
    y(i+1) = C*x +D*u(i);
        
    %xout(i) = x(1);
    %x = floor(x);
    %y(i) = floor(y(i));
end

% Plot results to verify, slight offset in steady-state error
if 1
    figure(1); hold off;
    yo = lsim(H, u, t); 
    plot(t, yo); hold on;
    stairs(t,y, 'r');
end

%% Show first 10 values, to be used in unit-test
[u(1:10)' y(1:10)']