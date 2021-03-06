
%% Split up 3ph sine waves into 100 sections
% First produce the underlying sine wave waveforms
x = linspace(0,2*pi,100);
% For half H-bridge driving, the sinusoids will be offset by the amplitude
y1 = sin(x) + 1;
y2 = sin(x + (2*pi/3)) + 1;
y3 = sin(x + (4*pi/3)) + 1;

plot(x, y1, 'LineWidth', 2);
hold on;
plot(x, y2, 'LineWidth', 2);
plot(x, y3, 'LineWidth', 2);
hold off;
grid on;
legend('y1', 'y2', 'y3');

% pause;

PGX_PERIOD = 999;
MAX_DUTY_CYCLE_VAL = PGX_PERIOD;

% Get percentage of maxes of sinusoids
percent_of_max1 = y1 / 2;
percent_of_max2 = y2 / 2;
percent_of_max3 = y3 / 2;

% Convert to PGX_DC values for the dsPIC33CK64MC105
pg1_duty_cycle_val = ceil(percent_of_max1 * MAX_DUTY_CYCLE_VAL);
pg2_duty_cycle_val = ceil(percent_of_max2 * MAX_DUTY_CYCLE_VAL);
pg3_duty_cycle_val = ceil(percent_of_max3 * MAX_DUTY_CYCLE_VAL);


%% Now output this into a c file and header file containing the PGxDC values as a constant array
% The header file that you include to gain access to the arrays defined in
% the source file
fid = fopen('duty_cycle.h','w');
fprintf(fid, '#include <stdint.h>\n\n\n');
fprintf(fid, 'extern const uint16_t pwm1_duty_cycle_vals[100];\n');
fprintf(fid, 'extern const uint16_t pwm2_duty_cycle_vals[100];\n');
fprintf(fid, 'extern const uint16_t pwm3_duty_cycle_vals[100];\n');
fclose(fid);


% The source file where the arrays will be placed
fid = fopen('duty_cycle.c','w');
fprintf(fid, '#include <stdint.h>\n\n\n');

fprintf(fid, 'const uint16_t pwm1_duty_cycle_vals[100] = {\n');
for i=1:5:length(pg1_duty_cycle_val)
    
    fprintf(fid, '\t%d, %d, %d, %d, %d', pg1_duty_cycle_val(i), pg1_duty_cycle_val(i+1), ...
        pg1_duty_cycle_val(i+2), pg1_duty_cycle_val(i+3), pg1_duty_cycle_val(i+4));
    if i < (length(pg1_duty_cycle_val) - 5)
        fprintf(fid, ',\n');
    else
        fprintf(fid, '\n');
    end
    
end
fprintf(fid,'};\n\n');

fprintf(fid, 'const uint16_t pwm2_duty_cycle_vals[100] = {\n');
for i=1:5:length(pg2_duty_cycle_val)
    
    fprintf(fid, '\t%d, %d, %d, %d, %d', pg2_duty_cycle_val(i), pg2_duty_cycle_val(i+1), ...
        pg2_duty_cycle_val(i+2), pg2_duty_cycle_val(i+3), pg2_duty_cycle_val(i+4));
    if i < (length(pg2_duty_cycle_val) - 5)
        fprintf(fid, ',\n');
    else
        fprintf(fid, '\n');
    end
    
end
fprintf(fid,'};\n\n');

fprintf(fid, 'const uint16_t pwm3_duty_cycle_vals[100] = {\n');
for i=1:5:length(pg3_duty_cycle_val)
    
    fprintf(fid, '\t%d, %d, %d, %d, %d', pg3_duty_cycle_val(i), pg3_duty_cycle_val(i+1), ...
        pg3_duty_cycle_val(i+2), pg3_duty_cycle_val(i+3), pg3_duty_cycle_val(i+4));
    if i < (length(pg3_duty_cycle_val) - 5)
        fprintf(fid, ',\n');
    else
        fprintf(fid, '\n');
    end
    
end
fprintf(fid,'};\n\n');

fclose(fid);

