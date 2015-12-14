clear all; clc;
format long;
% Definitions
MAX_PPRZ = 9600;
PERIODIC_FREQUENCY = 512;
Ts = 1/PERIODIC_FREQUENCY;
ALPHA_FACTOR = bitshift(1,14);
MAX_INCREMENT = 340;

% Analog filter
s = tf('s');
omega_c = 20; % rad/s
tau = 1/omega_c;
H = 1 / (1 + tau*s);

t = 0:Ts:0.4;
u = ones(size(t))*-9600;

[y,t] = lsim(H,u,t);

% Discrete filter
Hd = c2d(H, Ts);
alpha = PERIODIC_FREQUENCY*ALPHA_FACTOR/(PERIODIC_FREQUENCY+omega_c);
alpha = floor(alpha);
ydis = zeros(size(t));
for i=2:length(t)
    ydis(i) = alpha/ALPHA_FACTOR*ydis(i-1) + (1-alpha/ALPHA_FACTOR)*u(i);
    ydis(i) = floor(ydis(i));
    % check max rate
    increment = ydis(i) - ydis(i-1);
    if abs(increment) > MAX_INCREMENT
        ydis(i) = ydis(i-1) + sign(increment)*MAX_INCREMENT;
    end
end

% Plot figure
figure(1); hold off;
plot(t,y, 'LineWidth', 2, 'LineStyle', '-.');
hold on;
stairs(t, ydis, 'r', 'LineWidth', 2);
% step(Hd);

% figure(2);
% plot(t, y-ydis);

%% Test verification:
ydis(1:10)