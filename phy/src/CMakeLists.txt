#
# Copyright 2013-2015 Software Radio Systems Limited
#
# This file is part of the srsLTE library.
#
# srsLTE is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as
# published by the Free Software Foundation, either version 3 of
# the License, or (at your option) any later version.
#
# srsLTE is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# A copy of the GNU Affero General Public License can be found in
# the LICENSE file in the top-level directory of this distribution
# and at http://www.gnu.org/licenses/.
#

#################################################################
# Generate communicator_test executable.
#################################################################
include_directories("/usr/local/include")
include_directories("/usr/local/lib")
include_directories("../../../communicator/cpp")

link_directories("/usr/local/lib")

add_executable(test_communicator test_communicator.c)
target_link_libraries(test_communicator communicator protobuf zmq m boost_thread pthread)

#################################################################
# Applications
#################################################################

add_executable(synch_file synch_file.c)
target_link_libraries(synch_file srslte)

#################################################################
# These can be compiled without UHD or graphics support
#################################################################

OPTION(ENABLE_PHY_TX_FILTERING "PHY Tx filtering" OFF) # Disabled by default as we will use FPGA version.
IF(ENABLE_PHY_TX_FILTERING)
  ADD_DEFINITIONS(-DENABLE_PHY_TX_FILTERING)
ENDIF(ENABLE_PHY_TX_FILTERING)

OPTION(CONTROL_HARVESTING "Enable Control Harvesting" OFF) # Disabled by default.
IF(CONTROL_HARVESTING)
  ADD_DEFINITIONS(-DCONTROL_HARVESTING)
ENDIF(CONTROL_HARVESTING)

IF(RF_FOUND)
  IF(ENABLE_PHY_TX_FILTERING)
    message(STATUS "   PHY Tx filtering enabled")

    IF(ENBALE_SRS_GUI)
      add_executable(trx trx.c helpers.c phy_reception.c phy_transmission.c plot.c trx_filter.c)
    ELSE(ENBALE_SRS_GUI)
      add_executable(trx trx.c helpers.c phy_reception.c phy_transmission.c trx_filter.c)
    ENDIF(ENBALE_SRS_GUI)
    target_link_libraries(trx srslte pthread rt communicator protobuf zmq m boost_thread liquid)
    message(STATUS "   PHY will be installed.")

  ELSE(ENABLE_PHY_TX_FILTERING)
    message(STATUS "   PHY Tx filtering disabled")

    IF(ENBALE_SRS_GUI)
      add_executable(trx trx.c helpers.c phy_reception.c phy_transmission.c plot.c)
    ELSE(ENBALE_SRS_GUI)
      add_executable(trx trx.c helpers.c phy_reception.c phy_transmission.c)
    ENDIF(ENBALE_SRS_GUI)
    target_link_libraries(trx srslte pthread rt communicator protobuf zmq m boost_thread liquid)
    message(STATUS "   PHY will be installed.")

  ENDIF(ENABLE_PHY_TX_FILTERING)
ELSE(RF_FOUND)
  add_definitions(-DDISABLE_RF)
  message(STATUS "   PHY will NOT BE INSTALLED.")
ENDIF(RF_FOUND)

add_executable(read_dumped_file read_dumped_file.c)
target_link_libraries(read_dumped_file srslte pthread)

add_executable(check_tx_timestamps check_tx_timestamps.c)
target_link_libraries(check_tx_timestamps srslte pthread communicator protobuf zmq m boost_thread)

add_executable(pdsch_ue_pkt pdsch_ue_pkt.c)
target_link_libraries(pdsch_ue_pkt srslte pthread)

add_executable(pdsch_enodeb_pkt pdsch_enodeb_pkt.c)
target_link_libraries(pdsch_enodeb_pkt srslte pthread)

add_executable(pdsch_ue pdsch_ue.c)
target_link_libraries(pdsch_ue srslte pthread)

add_executable(pdsch_enodeb pdsch_enodeb.c)
target_link_libraries(pdsch_enodeb srslte pthread)

add_executable(test_fft test_fft.c)
target_link_libraries(test_fft srslte pthread boost_thread)

add_executable(measure_throughput measure_throughput.c helpers.c)
target_link_libraries(measure_throughput srslte pthread communicator protobuf zmq m boost_thread)

add_executable(measure_throughput_fd_auto measure_throughput_fd_auto.c helpers.c)
target_link_libraries(measure_throughput_fd_auto srslte pthread communicator protobuf zmq m boost_thread)

add_executable(check_phy_synchronization check_phy_synchronization.c helpers.c)
target_link_libraries(check_phy_synchronization srslte pthread communicator protobuf zmq m boost_thread)

add_executable(check_phy_with_channel_emulator check_phy_with_channel_emulator.c helpers.c)
target_link_libraries(check_phy_with_channel_emulator srslte pthread communicator protobuf zmq m boost_thread)

add_executable(stress_test stress_test.c helpers.c)
target_link_libraries(stress_test srslte pthread communicator protobuf zmq m boost_thread)

add_executable(fir_filter_checking fir_filter_checking.c helpers.c)
target_link_libraries(fir_filter_checking srslte pthread communicator protobuf zmq m boost_thread)

add_executable(fir_filter_checking_mac_like fir_filter_checking_mac_like.c helpers.c)
target_link_libraries(fir_filter_checking_mac_like srslte pthread communicator protobuf zmq m boost_thread)

add_executable(sync_decode_rx sync_decode_rx.c helpers.c)
target_link_libraries(sync_decode_rx srslte communicator protobuf zmq m)

add_executable(sync_decode_tx sync_decode_tx.c helpers.c)
target_link_libraries(sync_decode_tx srslte communicator protobuf zmq m)

IF(ENABLE_PHY_TX_FILTERING)
  add_executable(test_trx_filter test_trx_filter.c trx_filter.c)
  target_link_libraries(test_trx_filter srslte pthread communicator protobuf zmq m boost_thread)
ENDIF(ENABLE_PHY_TX_FILTERING)

add_executable(check_discontinous_tx check_discontinous_tx.c)
target_link_libraries(check_discontinous_tx srslte pthread communicator protobuf zmq m boost_thread)

add_executable(cfo_check cfo_check.c)
target_link_libraries(cfo_check srslte pthread communicator protobuf zmq m boost_thread)

add_executable(psch_test psch_test.c)
target_link_libraries(psch_test srslte pthread communicator protobuf zmq m boost_thread)

add_executable(psch_test_loop psch_test_loop.c)
target_link_libraries(psch_test_loop srslte pthread communicator protobuf zmq m boost_thread)

add_executable(rx_combining_poc rx_combining_poc.c)
target_link_libraries(rx_combining_poc srslte pthread communicator protobuf zmq m boost_thread)

add_executable(sch_test_loop sch_test_loop.c)
target_link_libraries(sch_test_loop srslte pthread communicator protobuf zmq m boost_thread)

add_executable(pss_test_loop pss_test_loop.c)
target_link_libraries(pss_test_loop srslte pthread communicator protobuf zmq m boost_thread)

add_executable(test_higher_mcs test_higher_mcs.c)
target_link_libraries(test_higher_mcs srslte pthread communicator protobuf zmq m boost_thread)

add_executable(test_scatter_synch_sequence test_scatter_synch_sequence.c)
target_link_libraries(test_scatter_synch_sequence srslte pthread communicator protobuf zmq m boost_thread)

add_executable(check_cqi_mcs_mapping check_cqi_mcs_mapping.c)
target_link_libraries(check_cqi_mcs_mapping srslte pthread communicator protobuf zmq m boost_thread)

if(CONTROL_HARVESTING)
  add_executable(decode_subframe decode_subframe.c)
  target_link_libraries(decode_subframe srslte)

  add_executable(decode_superframe decode_superframe.c)
  target_link_libraries(decode_superframe srslte)

  add_executable(encode_subframe encode_subframe.c)
  target_link_libraries(encode_subframe srslte)

  add_executable(encode_superframe encode_superframe.c)
  target_link_libraries(encode_superframe srslte)

  add_executable(tbs_tables_new tbs_tables_new.c)
  target_link_libraries(tbs_tables_new srslte)
endif(CONTROL_HARVESTING)

if(RF_FOUND)

else(RF_FOUND)
  add_definitions(-DDISABLE_RF)
endif(RF_FOUND)

find_package(SRSGUI)

if(SRSGUI_FOUND)
  include_directories(${SRSGUI_INCLUDE_DIRS})
  target_link_libraries(pdsch_ue ${SRSGUI_LIBRARIES})
  target_link_libraries(trx ${SRSGUI_LIBRARIES})
  target_link_libraries(pdsch_ue_pkt ${SRSGUI_LIBRARIES})
else(SRSGUI_FOUND)
  add_definitions(-DDISABLE_GRAPHICS)
endif(SRSGUI_FOUND)


#################################################################
# These examples need the UHD driver
#################################################################

if(RF_FOUND)

  add_executable(cell_search cell_search.c)
  target_link_libraries(cell_search srslte)

  add_executable(cell_measurement cell_measurement.c)
  target_link_libraries(cell_measurement srslte)

  add_executable(usrp_capture usrp_capture.c)
  target_link_libraries(usrp_capture srslte)

  add_executable(usrp_capture_sync usrp_capture_sync.c)
  target_link_libraries(usrp_capture_sync srslte)

  add_executable(usrp_txrx usrp_txrx.c)
  target_link_libraries(usrp_txrx srslte)

  message(STATUS "   examples will be installed.")

else(RF_FOUND)
  message(STATUS "   examples will NOT BE INSTALLED.")
endif(RF_FOUND)

# Add subdirectories
add_subdirectory(tutorial_examples)
