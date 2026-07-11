% Load and process the dynamic-CCC simulation output for all PU probabilities
pu_probs = [0.3, 0.5, 0.7];
colors = lines(3);  % different colors for each probability
ch_ids = 0:4;

pdr_data = zeros(5, 3);              % [CHs x Probs]
packet_loss_data = zeros(5, 3);     % [CHs x Probs]
outage_duration_data = zeros(5, 3); % [CHs x Probs]

% Process each probability
for p_idx = 1:length(pu_probs)
    prob = pu_probs(p_idx);
    filename = sprintf('packet_based_sim_pu_%d_summary.csv', round(prob*100));

    % read summary file
    data = readtable(filename);

    % Extract metrics for each CH
    for ch = ch_ids
        prefix = sprintf('CH%d_', ch);

        % PDR (packets delivered)
        pdr_row = startsWith(data.Metric, [prefix 'PDR']);
        pdr_data(ch+1, p_idx) = data.Value(pdr_row);

        % Packet loss percentage
        loss_row = startsWith(data.Metric, [prefix 'Packet_Lost_Percentage']);
        packet_loss_data(ch+1, p_idx) = data.Value(loss_row);

        % Outage duration
        outage_row = startsWith(data.Metric, [prefix 'Avg_Outage_Duration']);
        outage_duration_data(ch+1, p_idx) = data.Value(outage_row);
    end
end

%% Plot CH PDR
figure;
hold on;
for p_idx = 1:length(pu_probs)
    plot(ch_ids, pdr_data(:, p_idx), 'o-', 'LineWidth', 2, ...
        'Color', colors(p_idx, :), 'MarkerSize', 8, 'MarkerFaceColor', colors(p_idx, :));
end
hold off;
title('Packet Delivery Ratio per Cluster Head');
xlabel('Cluster Head ID');
ylabel('PDR');
legend(arrayfun(@(p) sprintf('PU Activity = %.1f', p), pu_probs, 'UniformOutput', false));
grid on;
set(gca, 'XTick', ch_ids);
ylim([0, 1.05]);

%% Plot Packet Loss Percentage
figure;
hold on;
for p_idx = 1:length(pu_probs)
    plot(ch_ids, packet_loss_data(:, p_idx)*100, 's-', 'LineWidth', 2, ...
        'Color', colors(p_idx, :), 'MarkerSize', 8, 'MarkerFaceColor', colors(p_idx, :));
end
hold off;
title('Packet Loss Percentage per Cluster Head');
xlabel('Cluster Head ID');
ylabel('Packet Loss (%)');
legend(arrayfun(@(p) sprintf('PU Activity = %.1f', p), pu_probs, 'UniformOutput', false));
grid on;
set(gca, 'XTick', ch_ids);
ylim([0, max(packet_loss_data(:))*100*1.1]);

%% Plot Average Outage Duration
figure;
hold on;
for p_idx = 1:length(pu_probs)
    plot(ch_ids, outage_duration_data(:, p_idx), 'd-', 'LineWidth', 2, ...
        'Color', colors(p_idx, :), 'MarkerSize', 8, 'MarkerFaceColor', colors(p_idx, :));
end
hold off;
title('Average Outage Duration per Cluster Head');
xlabel('Cluster Head ID');
ylabel('Outage Duration (slots)');
legend(arrayfun(@(p) sprintf('PU Activity = %.1f', p), pu_probs, 'UniformOutput', false));
grid on;
set(gca, 'XTick', ch_ids);
ylim([0, max(outage_duration_data(:))*1.1]);

%% State Transitions for CH0 (PU = 0.7)
prob = 0.7;
filename = sprintf('packet_based_sim_pu_%d_ch_states.csv', round(prob*100));
data = readtable(filename);

% Filter for CH0
ch0_data = data(data.CH_ID == 0, :);

% state mapping
state_map = containers.Map();
state_map('NORMAL') = 0;
state_map('DISPLACED') = 1;
state_map('ON_UNDERLAY') = 2;

% initialize state array with NaN for unhandled states
state_nums = nan(height(ch0_data), 1);

% Map the states
for i = 1:height(ch0_data)
    state_str = ch0_data.State{i};
    if isKey(state_map, state_str)
        state_nums(i) = state_map(state_str);
    end
end

% Plot for all 30,000 slots
figure;
plot(ch0_data.Slot, state_nums, 'LineWidth', 1);
title(sprintf('State Transitions for CH0 (PU Activity = %.1f)', prob));
xlabel('Time Slot');
ylabel('State');
yticks([0, 1, 2]);
yticklabels({'NORMAL', 'DISPLACED', 'ON_UNDERLAY'});
grid on;
ylim([-0.5, 2.5]);

% Horizontal lines for state levels
hold on;
yline(0, ':', 'NORMAL', 'LabelHorizontalAlignment', 'left', 'LineWidth', 1);
yline(1, ':', 'DISPLACED', 'LabelHorizontalAlignment', 'left', 'LineWidth', 1);
yline(2, ':', 'ON_UNDERLAY', 'LabelHorizontalAlignment', 'left', 'LineWidth', 1);
hold off;

% Improve layout
set(gcf, 'Position', [100, 100, 1200, 400]);
set(gca, 'FontSize', 10);
