close all;clear all;clc

L1_dBm = [
3	-67.85	-83.54	-83.82;
6	-64.86	-83.36	-83.68;
9	-62.14	-82.57	-82.53;
12	-59.61	-81.49	-80.88;
15	-56.28	-79.37	-79.17;
18	-53.64	-76.75	-76.35;
21	-50.12	-73.02	-72.71;
24	-47.71	-67.09	-66.65;
27	-44.68	-55.89	-55.48;
30	-38.58	-41.87	-41.6    
];

L2_dBm = [
3	-83.98	-83.99	-84.14;
6	-83.74	-84.12	-84.34;
9	-83.6	-84.16	-83.63;
12	-83.31	-83.82	-83.52;
15	-82.88	-83.1	-82.9;
18	-81.99	-82.64	-82.25;
21	-80.85	-81.33	-81.06;
24	-79.13	-79.61	-79.27;
27	-74.93	-75.6	-75.29;
30	-63.08	-64.26	-63.76   
];

U1_dBm = [
3	-68.64	-83.46	-83.3;
6	-65.46	-83.23	-83.46;
9	-63.19	-82.25	-82.36;
12	-59.95	-81.19	-80.9;
15	-56.69	-79.16	-78.97;
18	-54.33	-76.43	-76.17;
21	-51.41	-72.48	-72.14;
24	-48.96	-66.48	-65.9;
27	-45.17	-55.68	-55.36;
30	-39.02	-41.76	-41.53
];

U2_dBm = [
3	-83.57	-83.86	-83.84;
6	-83.77	-83.86	-83.98;
9	-83.45	-83.65	-84;
12	-83.16	-83.46	-83.65;
15	-83.05	-83.26	-83.59;
18	-82.09	-82.56	-82.49;
21	-80.99	-81.31	-81.12;
24	-79.36	-79.61	-79.34;
27	-75.15	-75.75	-75.5;
30	-63.52	-64.64	-64.03
];

figure
plot(L1_dBm(:,1),L1_dBm(:,2))
hold on
plot(L1_dBm(:,1),L1_dBm(:,3))
plot(L1_dBm(:,1),L1_dBm(:,4))
hold off
grid on
axis([L1_dBm(1,1) L1_dBm(10,1) -85 -37])
xlabel('Tx Gain [dB]')
ylabel('dBm')
title('Lower band channel (L1) -5 MHz offset')
legend('No filter','64 order FIR','128 order FIR');

figure
plot(U1_dBm(:,1),U1_dBm(:,2))
hold on
plot(U1_dBm(:,1),U1_dBm(:,3))
plot(U1_dBm(:,1),U1_dBm(:,4))
hold off
grid on
axis([U1_dBm(1,1) U1_dBm(10,1) -85 -37])
xlabel('Tx Gain [dB]')
ylabel('dBm')
title('Upper band channel (U1) +5 MHz offset')
legend('No filter','64 order FIR','128 order FIR');

figure
plot(L2_dBm(:,1),L2_dBm(:,2))
hold on
plot(L2_dBm(:,1),L2_dBm(:,3))
plot(L2_dBm(:,1),L2_dBm(:,4))
hold off
grid on
axis([U2_dBm(1,1) U2_dBm(10,1) -85 -63])
xlabel('Tx Gain [dB]')
ylabel('dBm')
title('Lower band channel (L2) -10 MHz offset')
legend('No filter','64 order FIR','128 order FIR');

figure
plot(U2_dBm(:,1),U2_dBm(:,2))
hold on
plot(U2_dBm(:,1),U2_dBm(:,3))
plot(U2_dBm(:,1),U2_dBm(:,4))
hold off
grid on
axis([U2_dBm(1,1) U2_dBm(10,1) -85 -63])
xlabel('Tx Gain [dB]')
ylabel('dBm')
title('Upper band channel (U2) +10 MHz offset')
legend('No filter','64 order FIR','128 order FIR');


