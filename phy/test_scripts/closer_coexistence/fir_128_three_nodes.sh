#!/usr/bin/env bash

sleep 10

#----------- MCS 0 ---------------
~/radio_api/stop.sh ; ~/radio_api/kill_stack.py ;

sleep 5

sudo /root/gitrepo/backup_plan/scatter/build/phy/srslte/examples/trx -f 1000000000 -B 20000000 -Y 28/28 -QQ &

sleep 20

sudo python3 ./measure_self_reception_performance.py --txchannel=1 --rxchannel=1 --txgain=20 --rxgain=20 --numpktstotx=10000 --txslots=10 --mcs=0 --savetofile --filename=closer_coexistance_128_filter_mcs_0.txt ;

sleep 10

#----------- MCS 10 ---------------
~/radio_api/stop.sh ; ~/radio_api/kill_stack.py ;

sleep 5

sudo /root/gitrepo/backup_plan/scatter/build/phy/srslte/examples/trx -f 1000000000 -B 20000000 -Y 28/28 -QQ &

sleep 20

sudo python3 ./measure_self_reception_performance.py --txchannel=1 --rxchannel=1 --txgain=20 --rxgain=20 --numpktstotx=10000 --txslots=10 --mcs=10 --savetofile --filename=closer_coexistance_128_filter_mcs_10.txt ;

sleep 10

#----------- MCS 12 ---------------
~/radio_api/stop.sh ; ~/radio_api/kill_stack.py ;

sleep 5

sudo /root/gitrepo/backup_plan/scatter/build/phy/srslte/examples/trx -f 1000000000 -B 20000000 -Y 28/28 -QQ &

sleep 20

sudo python3 ./measure_self_reception_performance.py --txchannel=1 --rxchannel=1 --txgain=20 --rxgain=20 --numpktstotx=10000 --txslots=10 --mcs=12 --savetofile --filename=closer_coexistance_128_filter_mcs_12.txt ;

sleep 10

#----------- MCS 14 ---------------
~/radio_api/stop.sh ; ~/radio_api/kill_stack.py ;

sleep 5

sudo /root/gitrepo/backup_plan/scatter/build/phy/srslte/examples/trx -f 1000000000 -B 20000000 -Y 28/28 -QQ &

sleep 20

sudo python3 ./measure_self_reception_performance.py --txchannel=1 --rxchannel=1 --txgain=20 --rxgain=20 --numpktstotx=10000 --txslots=10 --mcs=14 --savetofile --filename=closer_coexistance_128_filter_mcs_14.txt ;

sleep 10

#----------- MCS 16 ---------------
~/radio_api/stop.sh ; ~/radio_api/kill_stack.py ;

sleep 5

sudo /root/gitrepo/backup_plan/scatter/build/phy/srslte/examples/trx -f 1000000000 -B 20000000 -Y 28/28 -QQ &

sleep 20

sudo python3 ./measure_self_reception_performance.py --txchannel=1 --rxchannel=1 --txgain=20 --rxgain=20 --numpktstotx=10000 --txslots=10 --mcs=16 --savetofile --filename=closer_coexistance_128_filter_mcs_16.txt ;

sleep 10

#----------- MCS 18 ---------------
~/radio_api/stop.sh ; ~/radio_api/kill_stack.py ;

sleep 5

sudo /root/gitrepo/backup_plan/scatter/build/phy/srslte/examples/trx -f 1000000000 -B 20000000 -Y 28/28 -QQ &

sleep 20

sudo python3 ./measure_self_reception_performance.py --txchannel=1 --rxchannel=1 --txgain=20 --rxgain=20 --numpktstotx=10000 --txslots=10 --mcs=18 --savetofile --filename=closer_coexistance_128_filter_mcs_18.txt ;

sleep 10

#----------- MCS 20 ---------------
~/radio_api/stop.sh ; ~/radio_api/kill_stack.py ;

sleep 5

sudo /root/gitrepo/backup_plan/scatter/build/phy/srslte/examples/trx -f 1000000000 -B 20000000 -Y 28/28 -QQ &

sleep 20

sudo python3 ./measure_self_reception_performance.py --txchannel=1 --rxchannel=1 --txgain=20 --rxgain=20 --numpktstotx=10000 --txslots=10 --mcs=20 --savetofile --filename=closer_coexistance_128_filter_mcs_20.txt ;

sleep 10

#----------- MCS 22 ---------------
~/radio_api/stop.sh ; ~/radio_api/kill_stack.py ;

sleep 5

sudo /root/gitrepo/backup_plan/scatter/build/phy/srslte/examples/trx -f 1000000000 -B 20000000 -Y 28/28 -QQ &

sleep 20

sudo python3 ./measure_self_reception_performance.py --txchannel=1 --rxchannel=1 --txgain=20 --rxgain=20 --numpktstotx=10000 --txslots=10 --mcs=22 --savetofile --filename=closer_coexistance_128_filter_mcs_22.txt ;

sleep 10

#----------- MCS 24 ---------------
~/radio_api/stop.sh ; ~/radio_api/kill_stack.py ;

sleep 5

sudo /root/gitrepo/backup_plan/scatter/build/phy/srslte/examples/trx -f 1000000000 -B 20000000 -Y 28/28 -QQ &

sleep 20

sudo python3 ./measure_self_reception_performance.py --txchannel=1 --rxchannel=1 --txgain=20 --rxgain=20 --numpktstotx=10000 --txslots=10 --mcs=24 --savetofile --filename=closer_coexistance_128_filter_mcs_24.txt ;

sleep 10

#----------- MCS 26 ---------------
~/radio_api/stop.sh ; ~/radio_api/kill_stack.py ;

sleep 5

sudo /root/gitrepo/backup_plan/scatter/build/phy/srslte/examples/trx -f 1000000000 -B 20000000 -Y 28/28 -QQ &

sleep 20

sudo python3 ./measure_self_reception_performance.py --txchannel=1 --rxchannel=1 --txgain=20 --rxgain=20 --numpktstotx=10000 --txslots=10 --mcs=26 --savetofile --filename=closer_coexistance_128_filter_mcs_26.txt ;

sleep 10

#----------- MCS 28 ---------------
~/radio_api/stop.sh ; ~/radio_api/kill_stack.py ;

sleep 5

sudo /root/gitrepo/backup_plan/scatter/build/phy/srslte/examples/trx -f 1000000000 -B 20000000 -Y 28/28 -QQ &

sleep 20

sudo python3 ./measure_self_reception_performance.py --txchannel=1 --rxchannel=1 --txgain=20 --rxgain=20 --numpktstotx=10000 --txslots=10 --mcs=28 --savetofile --filename=closer_coexistance_128_filter_mcs_28.txt ;

sleep 10

#---------------------- Save files ---------------------
sudo mkdir results_fir_128_v1

sudo mv closer_coexistance_128_filter_mcs_* results_fir_128_v1/

sudo tar -cvzf results_fir_128_v1.tar.gz results_fir_128_v1/
