/*
 * CPPExample.cpp
 *
 * This file contains an example of how you can use the LayerCommunicator and SafeQueue
 *
 *  Created on: 28 Mar 2017
 *      Author: rubenmennes
 */

#include "LayerCommunicator.h"
#include "interf.pb.h"
#include <unistd.h>
#include <iostream>
#include "logging/Logger.h"
#include "SafeQueue.h"
#include "utils.h"

extern "C" {
#include "../../phy/srslte/include/srslte/intf/intf.h"
}

using namespace communicator;

int main(int argc, char *argv[]) {
	Logger::set_log_level<SafeQueue::SafeQueue<TimestampedMessage>>(TRACE);
	Logger::set_global_log_level(TRACE);
	//READ THE MODULE FROM COMMAND LINE
	MODULE m;
	std::string str(argv[1]);

	//IF COMMAND LINE ARGUMENT IS A VALID MODULE
	if (argc > 1 && MODULE_Parse(str, &m)) {
		//START THE COMMUNICATOR FOR THIS LAYER
		LayerCommunicator<> lc(m, {MODULE::MODULE_PHY, MODULE::MODULE_MAC});

		//If you want to use a list instead of a deque as underlying container you can use LayerCommunicator<std::list<Message>>

		//SLEEP 4 SEC SO YOU HAVE TIME TO SETUP OTHER MODULES
		sleep(4);

		//START SENDING
		std::cout << "START SENDING" << std::endl;
		if (m == MODULE::MODULE_PHY) {
			//EXAMPLE TO SEND BACK AN SEND_R MESSAGE AFTER SENDING A PACKET OVER THE RADIO WHEN RECEIVING A SEND MESSAGE FROM THE MAC.

			//CREATE A SHARED_PTR TO A NEW PHY STAT MESSAGE
			std::shared_ptr<Internal> mes = std::make_shared<Internal>();

			//SET SOME DATA
			mes->set_transaction_index(1);		//SET THE SAME NUMBER AS THE SEND MESSAGE

			//Create Send_r message
			communicator::Send_r* send_r = new communicator::Send_r();
			send_r->set_result(communicator::TRANSACTION_RESULT::OK); //Sended succsfull

			//Create Phy stat message
			communicator::Phy_stat* stat = new communicator::Phy_stat();
			//Set some overall statistics
			stat->set_frame(200);
			stat->set_mcs(3);
			stat->set_ch(4);
			stat->set_slot(23);
			//For the complete list look at the inter.proto file.

			//Create a Phy stat TX message
			communicator::Phy_tx_stat *stat_tx = new communicator::Phy_tx_stat();
			stat_tx->set_power(40);

			//Add phy tx stat to phy stat
			stat->set_allocated_tx_stat(stat_tx);	//Stat has ownership over the stat_tx pointer

			//Add phy stat to Send_r message
			send_r->set_allocated_phy_stat(stat);	//send_r has ownership over the stat pointer

			//Add Send_r to the message
			mes->set_allocated_sendr(send_r);		//mes has ownership over the send_r pointer


			//CREATE A MESSAGE
			Message messa(m, communicator::MODULE_MAC, mes);

			//PUT THE MESSAGE IN THE SENDING QUEUE (HERE WE USE THE MOVE OPERATOR)
			Logger::log_info<CommManager>("SENDING MESSAGE IN MAIN AT {0}", clock_get_time_ns());
			while(true){
				lc.send(messa);
				sleep(1);
			}
			Logger::log_info<CommManager>("SENDED MESSAGE IN MAIN AT {0}", clock_get_time_ns());
			//m will have invalid data from this moment on



		}
//		else {
//			//SEND EXAMPLE TO THE PHY LAYER
//
//			//CREATE A SHARED_PTR TO A NEW PHY STAT MESSAGE
//			std::shared_ptr<Internal> mes = std::make_shared<Internal>();
//
//			//SET THE TRANACTION INDEX
//			mes->set_transaction_index(2);		//New number
//
//			//CREATE A SEND MESSAGE
//			communicator::Send *send = new communicator::Send();
//			mes->set_allocated_send(send);
//
//			communicator::Basic_ctrl *ctrl = new communicator::Basic_ctrl();
//			send->set_allocated_basic_ctrl(ctrl);		//send has ownership over ctrl
//
//			communicator::Application_data *data = new communicator::Application_data();
//			send->set_allocated_app_data(data);				//send has ownership over data
//
//			//Set application data
//			data->set_data("This would be an IPv4 message from the APP");
//			data->set_destination_ip(12342);
//			data->set_next_mac(12342);
//
//			//Set basic ctrl settings
//			ctrl->set_center_freq(32);
//			ctrl->set_ch(2);
//			ctrl->set_mcs(4);
//			//For the complete list take a look at the interf.proto file.
//
//			std::cout << "MSG created to be sent to PHY" << std::endl;
//
//			//CREATE A MESSAGE
//			Message messa(m, communicator::MODULE_PHY, mes);
//
//			//PUT THE MESSAGE IN THE SENDING QUEUE AND USE THE COPY OPERATOR
//			lc.send(messa);
//			//mes will have the same data as before
//
//			std::cout << "MSG sent to PHY" << std::endl;
//		}

		//WAIT UNTIL THERE IS A MESSAGE IN THE HIGH QUEUE

		Message mes;
		while(true){
			mes = lc.get_low_queue().pop_wait();
			Logger::log_info<CommManager>("RECEIVED MESSAGE IN MAIN AT {0}", clock_get_time_ns());
		}

		//PRINT THE MESSAGE
		std::cout << "Get message " << mes << std::endl;

		std::shared_ptr<communicator::Internal> imes = std::static_pointer_cast<communicator::Internal>(mes.message);
		std::cout << "TransactionIndex: " << imes->transaction_index() << std::endl;

		//IN THIS SWITCH CASE WE CAN SWITCH BETWEEN THE DIFFERENT MESSAGES
		switch (imes->payload_case()) {
			case communicator::Internal::kGet:
				std::cout << "Get message" << std::endl;
				std::cout << imes->get().attribute() << std::endl;
				break;
			case communicator::Internal::kGetr:
				std::cout << "Get_r message" << std::endl;
				break;
			case communicator::Internal::kSend:
				std::cout << "Send message" << std::endl;
				std::cout << "The application data is: " << imes->send().app_data().data() << std::endl;
				std::cout << "The channel of the message is: " << imes->send().basic_ctrl().ch() << std::endl;
				break;
			case communicator::Internal::kSendr:
				std::cout << "This is a send_r message" << std::endl;
				std::cout << "The transaction result of the send_r message is: " << communicator::TRANSACTION_RESULT_Name(imes->sendr().result()) << std::endl;
				//SWITCH BETWEEN PHY STAT AND MAC STAT
				switch(imes->sendr().payload_case()){
					case communicator::Send_r::kMacResultFieldNumber:
						std::cout << "We got mac stats " << imes->sendr().mac_result().DebugString() << std::endl;
						break;
					case communicator::Send_r::kPhyStatFieldNumber:
						std::cout << "We have PHY stats " << imes->sendr().phy_stat().DebugString() << std::endl;
						break;
					default:
						std::cout << "There were no phy nor mac stats" << std::endl;
				}
				break;
			default:
				std::cout << "Message type not in the switch statement" << std::endl;
				break;
		}

		//CHECK IF THERE IS ANOTHER MESSAGE IN THE QUEUE (NOW)
		if (lc.get_low_queue().pop(mes)) {
			std::cout << "Get message " << mes << std::endl;
		} else {
			std::cout << "No message yet" << std::endl;
		}

		//SLEEP BEFORE WE CLOSE ALL THE CONNECTIONS
		sleep(10);

		std::cout << "Sleep for 10 seconds" << std::endl;
	} else {
		std::cout << "Usage: CPPExample [MODULE_PHY|MODULE_MAC|MODULE_AI]" << std::endl;
	}

	std::cout << "Terminating app." << std::endl;
}
