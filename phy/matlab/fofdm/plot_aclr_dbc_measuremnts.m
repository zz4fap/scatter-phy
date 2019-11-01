close all;clear all;clc

L1_dBc = [
3	-43.35	-58.98	-59.25;
6	-43.25	-61.67	-61.95;
9	-43.62	-63.99	-63.98;
12	-43.92	-65.74	-65.19;
15	-43.49	-66.54	-66.36;
18	-43.81	-66.86	-66.5;
21	-43.17	-65.97	-65.71;
24	-43.59	-62.89	-62.49;
27	-43.4	-54.57	-54.1;
30	-39.93	-43.14	-42.86];

L2_dBc = [
3	-59.48	-59.43	-59.57;
6	-62.13	-62.43	-62.61;
9	-65.08	-65.58	-65.08;
12	-67.62	-68.07	-67.83;
15	-70.09	-70.27	-70.09;
18	-72.16	-72.75	-72.4;
21	-73.9	-74.28	-74.06;
24	-75.01	-75.41	-75.11;
27	-73.65	-74.28	-73.91;
30	-64.43	-65.53	-65.02];

U1_dBc = [
3	-44.14	-58.9	-58.73;
6	-43.85	-61.54	-61.73;
9	-44.67	-63.67	-63.81;
12	-44.26	-65.44	-65.21;
15	-43.9	-66.33	-66.16;
18	-44.5	-66.54	-66.32;
21	-44.46	-65.43	-65.14;
24	-44.84	-62.28	-61.74;
27	-43.89	-54.36	-53.98;
30	-40.37	-43.03	-42.79];

U2_dBc = [
3	-59.07	-59.3	-59.27;
6	-62.16	-62.17	-62.25;
9	-64.93	-65.07	-65.45;
12	-67.47	-67.71	-67.96;
15	-70.26	-70.43	-70.78;
18	-72.26	-72.67	-72.64;
21	-74.04	-74.26	-74.12;
24	-75.24	-75.41	-75.18;
27	-73.87	-74.43	-74.12;
30	-64.87	-65.91	-65.29];

figure
plot(L1_dBc(:,1),L1_dBc(:,2))
hold on
plot(L1_dBc(:,1),L1_dBc(:,3))
plot(L1_dBc(:,1),L1_dBc(:,4))
hold off
grid on
axis([L1_dBc(1,1) L1_dBc(10,1) -70 -40])
xlabel('Tx Gain [dB]')
ylabel('dBc')
title('Lower band channel (L1) -5 MHz offset')

figure
plot(U1_dBc(:,1),U1_dBc(:,2))
hold on
plot(U1_dBc(:,1),U1_dBc(:,3))
plot(U1_dBc(:,1),U1_dBc(:,4))
hold off
grid on
axis([U1_dBc(1,1) U1_dBc(10,1) -70 -40])
xlabel('Tx Gain [dB]')
ylabel('dBc')
title('Upper band channel (U1) +5 MHz offset')

figure
plot(L2_dBc(:,1),L2_dBc(:,2))
hold on
plot(L2_dBc(:,1),L2_dBc(:,3))
plot(L2_dBc(:,1),L2_dBc(:,4))
hold off
grid on
axis([U2_dBc(1,1) U2_dBc(10,1) -76 -58])
xlabel('Tx Gain [dB]')
ylabel('dBc')
title('Lower band channel (L2) -10 MHz offset')

figure
plot(U2_dBc(:,1),U2_dBc(:,2))
hold on
plot(U2_dBc(:,1),U2_dBc(:,3))
plot(U2_dBc(:,1),U2_dBc(:,4))
hold off
grid on
axis([U2_dBc(1,1) U2_dBc(10,1) -76 -58])
xlabel('Tx Gain [dB]')
ylabel('dBc')
title('Upper band channel (U2) +10 MHz offset')
