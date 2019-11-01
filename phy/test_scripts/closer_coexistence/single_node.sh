#!/usr/bin/env bash

sleep 10

#nof_pkts=10000

#----------- MCS 0 ---------------
~/radio_api/stop.sh ; ~/radio_api/kill_stack.py ;

sleep 5

sudo /root/gitrepo/backup_plan/scatter/build/phy/srslte/examples/trx -f 1000000000 -B 20000000 -Y 28/28 &

sleep 20

sudo python3 ./measure_self_reception_performance.py --txchannel=1 --rxchannel=1 --txgain=20 --rxgain=20 --numpktstotx=10000 --txslots=10 --mcs=0 --savetofile --filename=single_node_mcs_0.txt ;

sleep 5

#----------- MCS 10 ---------------
~/radio_api/stop.sh ; ~/radio_api/kill_stack.py ;

sleep 5

sudo /root/gitrepo/backup_plan/scatter/build/phy/srslte/examples/trx -f 1000000000 -B 20000000 -Y 28/28 &

sleep 20

sudo python3 ./measure_self_reception_performance.py --txchannel=1 --rxchannel=1 --txgain=20 --rxgain=20 --numpktstotx=10000 --txslots=10 --mcs=10 --savetofile --filename=single_node_mcs_10.txt ;

sleep 5

#----------- MCS 12 ---------------
~/radio_api/stop.sh ; ~/radio_api/kill_stack.py ;

sleep 5

sudo /root/gitrepo/backup_plan/scatter/build/phy/srslte/examples/trx -f 1000000000 -B 20000000 -Y 28/28 &

sleep 20

sudo python3 ./measure_self_reception_performance.py --txchannel=1 --rxchannel=1 --txgain=20 --rxgain=20 --numpktstotx=10000 --txslots=10 --mcs=12 --savetofile --filename=single_node_mcs_12.txt ;

sleep 10

#----------- MCS 14 ---------------
~/radio_api/stop.sh ; ~/radio_api/kill_stack.py ;

sleep 5

sudo /root/gitrepo/backup_plan/scatter/build/phy/srslte/examples/trx -f 1000000000 -B 20000000 -Y 28/28 &

sleep 20

sudo python3 ./measure_self_reception_performance.py --txchannel=1 --rxchannel=1 --txgain=20 --rxgain=20 --numpktstotx=10000 --txslots=10 --mcs=14 --savetofile --filename=single_node_mcs_14.txt ;

sleep 10

#----------- MCS 16 ---------------
~/radio_api/stop.sh ; ~/radio_api/kill_stack.py ;

sleep 5

sudo /root/gitrepo/backup_plan/scatter/build/phy/srslte/examples/trx -f 1000000000 -B 20000000 -Y 28/28 &

sleep 20

sudo python3 ./measure_self_reception_performance.py --txchannel=1 --rxchannel=1 --txgain=20 --rxgain=20 --numpktstotx=10000 --txslots=10 --mcs=16 --savetofile --filename=single_node_mcs_16.txt ;

sleep 10

#----------- MCS 18 ---------------
~/radio_api/stop.sh ; ~/radio_api/kill_stack.py ;

sleep 5

sudo /root/gitrepo/backup_plan/scatter/build/phy/srslte/examples/trx -f 1000000000 -B 20000000 -Y 28/28 &

sleep 20

sudo python3 ./measure_self_reception_performance.py --txchannel=1 --rxchannel=1 --txgain=20 --rxgain=20 --numpktstotx=10000 --txslots=10 --mcs=18 --savetofile --filename=single_node_mcs_18.txt ;

sleep 10

#----------- MCS 20 ---------------
~/radio_api/stop.sh ; ~/radio_api/kill_stack.py ;

sleep 5

sudo /root/gitrepo/backup_plan/scatter/build/phy/srslte/examples/trx -f 1000000000 -B 20000000 -Y 28/28 &

sleep 20

sudo python3 ./measure_self_reception_performance.py --txchannel=1 --rxchannel=1 --txgain=20 --rxgain=20 --numpktstotx=10000 --txslots=10 --mcs=20 --savetofile --filename=single_node_mcs_20.txt ;

sleep 10

#----------- MCS 22 ---------------
~/radio_api/stop.sh ; ~/radio_api/kill_stack.py ;

sleep 5

sudo /root/gitrepo/backup_plan/scatter/build/phy/srslte/examples/trx -f 1000000000 -B 20000000 -Y 28/28 &

sleep 20

sudo python3 ./measure_self_reception_performance.py --txchannel=1 --rxchannel=1 --txgain=20 --rxgain=20 --numpktstotx=10000 --txslots=10 --mcs=22 --savetofile --filename=single_node_mcs_22.txt ;

sleep 10

#----------- MCS 24 ---------------
~/radio_api/stop.sh ; ~/radio_api/kill_stack.py ;

sleep 5

sudo /root/gitrepo/backup_plan/scatter/build/phy/srslte/examples/trx -f 1000000000 -B 20000000 -Y 28/28 &

sleep 20

sudo python3 ./measure_self_reception_performance.py --txchannel=1 --rxchannel=1 --txgain=20 --rxgain=20 --numpktstotx=10000 --txslots=10 --mcs=24 --savetofile --filename=single_node_mcs_24.txt ;

sleep 10

#----------- MCS 26 ---------------
~/radio_api/stop.sh ; ~/radio_api/kill_stack.py ;

sleep 5

sudo /root/gitrepo/backup_plan/scatter/build/phy/srslte/examples/trx -f 1000000000 -B 20000000 -Y 28/28 &

sleep 20

sudo python3 ./measure_self_reception_performance.py --txchannel=1 --rxchannel=1 --txgain=20 --rxgain=20 --numpktstotx=10000 --txslots=10 --mcs=26 --savetofile --filename=single_node_mcs_26.txt ;

sleep 10

#----------- MCS 28 ---------------
~/radio_api/stop.sh ; ~/radio_api/kill_stack.py ;

sleep 5

sudo /root/gitrepo/backup_plan/scatter/build/phy/srslte/examples/trx -f 1000000000 -B 20000000 -Y 28/28 &

sleep 20

sudo python3 ./measure_self_reception_performance.py --txchannel=1 --rxchannel=1 --txgain=20 --rxgain=20 --numpktstotx=10000 --txslots=10 --mcs=28 --savetofile --filename=single_node_mcs_28.txt ;

sleep 10
