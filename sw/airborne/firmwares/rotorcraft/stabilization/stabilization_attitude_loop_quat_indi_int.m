%% MATLAB implementation of INDI integer, roll axis only
clear all; clc;

%% INDI loop
dt = 1/512;

A = 1;
B = 0.001;
invB = 1/B;

COMMAND_ROLL = 15;
pgain = 11;
dgain = 12;
att_err = 230;
body_rate_now = 100;
body_rate_prev = 144;

virtual_control = (att_err * pgain) - (body_rate_now * dgain)

% Difference between current rate and next rate, can be considered as an 
next_desired_diff = virtual_control * dt;
current_diff = body_rate_now - body_rate_prev;

diff_diff = next_desired_diff - current_diff;

delta_u = diff_diff * invB
int32(delta_u)