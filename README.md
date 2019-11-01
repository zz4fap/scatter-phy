# scatter-phy
This repo contains the source code of the physical layer developed for the DARPA Spectrum Collaboration Challenge (SC2).
SCATTER PHY is described in the paper below.

Reference:
[1] Pereira de Figueiredo, F.A.; Stojadinovic, D..; Maddala, P..; Mennes, R..; Jabandžic, I..; Jiao, X.; Moerman, I.. SCATTER PHY: An Open Source Physical Layer for the DARPA Spectrum Collaboration Challenge. Preprints 2019, 2019100115 (doi: 10.20944/preprints201910.0115.v1).



# Installation steps

Install gnuradio FROM SOURCE!!!. Now only gnuradio 3.7.10.1 is verified. Newer version may cause issues.

Then:
'''git clone https://github.com/zz4fap/scatter-phy.git
cd scatter/phy
mkdir build
cd build
cmake ../
make'''
