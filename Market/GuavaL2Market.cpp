//
// Created by zhangyw on 11/16/19.
//

#include "GuavaL2Market.h"
#include <iostream>
#include <sstream>
#include <string>
#include <assert.h>
#include <errno.h>
#include <chrono>
#include <sys/time.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <arpa/inet.h>


#include "../Utils/Utils.h"
#include "../Utils/TradingHours.h"


namespace TrendFollow {
    namespace Market {


        GuavaL2Market::GuavaL2Market(Driver::RealtimeDriver *driver, std::string &config_path ) : m_configPath(config_path){

            m_thrade_quit_flag = false;

            m_driver = driver;
            m_sock = MY_SOCKET_DEFAULT;

        }

        int GuavaL2Market::start() {
            //   input_param();
            boost::property_tree::ptree pt;
            boost::property_tree::read_xml(m_configPath, pt);
            m_multi_info.m_local_ip= pt.get_child("ThostUser2.ConnectConfig.LocalIP").get<std::string>("<xmlattr>.value");

            m_multi_info.m_local_port= pt.get_child("ThostUser2.ConnectConfig.LocalPort").get<int>("<xmlattr>.value");
            m_multi_info.m_remote_ip =  pt.get_child("ThostUser2.ConnectConfig.RemoteIP").get<std::string>("<xmlattr>.value");

            m_multi_info.m_remote_port= pt.get_child("ThostUser2.ConnectConfig.RemotePort").get<int>("<xmlattr>.value");

            m_exchangeID = pt.get_child("ThostUser2.ConnectConfig.exchangeID").get<char>("<xmlattr>.value");
            m_affiCore = pt.get_child("ThostUser2.ConnectConfig.affiCore").get<int>("<xmlattr>.value");
            fprintf(stderr, "m_exchangeID= %c, m_affiCore=%d\n", m_exchangeID, m_affiCore);


            bool ret = init();
            if (!ret) {
                string str_temp;
                //   close();
                return -1 ;
            }
            return 0;

        }

        void GuavaL2Market::SubScribeQuote(Types::SubScribeQuote const & subScribeQuote){

            auto itr = m_subScribeInstruments.find(subScribeQuote.instrumentID);
            if(itr == m_subScribeInstruments.end()){
                Types::PushMarket* pushMarket = new Types::PushMarket(0);
                m_subScribeInstruments[subScribeQuote.instrumentID] = pushMarket;
            }

            m_subScribeInstruments[subScribeQuote.instrumentID]->subscribePolicy.emplace_back(subScribeQuote.policyID);

        }

        GuavaL2Market::~GuavaL2Market(void) {


        }

        bool GuavaL2Market::sock_init()//const string& local_ip,
        {
            bool b_ret = false;
            const int CONST_ERROR_SOCK = -1;


            try {
                m_sock = socket(PF_INET, SOCK_DGRAM, 0);
                if (MY_SOCKET_ERROR == m_sock) {
                    throw CONST_ERROR_SOCK;
                }

                //socket????????????????????
                int flag = 1;
                if (setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char *) &flag, sizeof(flag)) != 0) {
                    throw CONST_ERROR_SOCK;
                }

                int options = fcntl(m_sock, F_GETFL);
                if (options < 0) {
                    throw CONST_ERROR_SOCK;
                }
                options = options | O_NONBLOCK;
                int i_ret = fcntl(m_sock, F_SETFL, options);
                if (i_ret < 0) {
                    throw CONST_ERROR_SOCK;
                }

                struct sockaddr_in local_addr;
                memset(&local_addr, 0, sizeof(local_addr));
                local_addr.sin_family = AF_INET;
                local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
                local_addr.sin_port = htons(m_multi_info.m_remote_port);    //multicast port
                if (bind(m_sock, (struct sockaddr *) &local_addr, sizeof(local_addr)) < 0) {
                    throw CONST_ERROR_SOCK;
                }

                struct ip_mreq mreq;
                mreq.imr_multiaddr.s_addr = inet_addr(m_multi_info.m_remote_ip.c_str());    //multicast group ip
                mreq.imr_interface.s_addr = inet_addr(m_multi_info.m_local_ip.c_str());

                if (setsockopt(m_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) != 0) {
                    throw CONST_ERROR_SOCK;
                }

                int receive_buf_size = RCV_BUF_SIZE;
                if (setsockopt(m_sock, SOL_SOCKET, SO_RCVBUF, (const char *) &receive_buf_size,
                               sizeof(receive_buf_size)) != 0) {
                    throw CONST_ERROR_SOCK;
                }

                //???????
                b_ret = start_server_event_thread();
            }
            catch (...) {
                close(m_sock);
                b_ret = false;
            }

            return b_ret;
        }


        bool GuavaL2Market::sock_close() {
            bool b_ret = false;
            //??????
            b_ret = stop_server_event_thread();

            if (m_sock != MY_SOCKET_DEFAULT) {
                close(m_sock);
                m_sock = MY_SOCKET_DEFAULT;
            }

            return b_ret;
        }


        void *GuavaL2Market::socket_server_event_thread(void *ptr_param) {
            GuavaL2Market *ptr_this = (GuavaL2Market *) ptr_param;
            if (NULL == ptr_this) {
                return NULL;
            }

            return ptr_this->on_socket_server_event_thread();
        }

        void *GuavaL2Market::on_socket_server_event_thread() {
      //      fprintf(stderr, "this GUAVAl2Market, tid=%ld, cpu=%d\n" ,  (long int)syscall(SYS_gettid), 0 );
            char line[RCV_BUF_SIZE] = "";

            int n_rcved = -1;

            struct sockaddr_in muticast_addr;

            memset(&muticast_addr, 0, sizeof(muticast_addr));
            muticast_addr.sin_family = AF_INET;
            muticast_addr.sin_addr.s_addr = inet_addr(m_multi_info.m_remote_ip.c_str());
            muticast_addr.sin_port = htons(m_multi_info.m_remote_port);
        //    printf("bufferSize = %d\n", RCV_BUF_SIZE);


            while (true) {
                socklen_t len = sizeof(sockaddr_in);
                auto epoch_time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
                n_rcved = recvfrom(m_sock, line, RCV_BUF_SIZE, 0, (struct sockaddr *) &muticast_addr, &len);
                if(n_rcved>0) {

                    unsigned int proc_len=0;
                    while(proc_len < n_rcved && n_rcved % sizeof(guava_udp_head) ==0){
                        guava_udp_head* ptr_data = (guava_udp_head*)(line + proc_len);
	//		std::array<char,4> temp{""};
		        int hh  =  (int)ptr_data->m_update_time_h;
		        int mm = (int)ptr_data->m_update_time_m;
		        int ss  =  (int)ptr_data->m_update_time_s;
		      /*  fprintf(stderr, "begin print guava_udp_head");*/	
                     /*   fprintf(stderr,"sequence=%d, exchange_id=%c, symbol=%s, update_time_h=%c, update_time_m=%c, update_time_s=%c, "
					"snap_millisec=%d, last_px=%f, volume=%d, turnover=%f, open_interest=%f, bid1_px=%f, bid1_share=%d, "
					"ask1_px=%f. ask1_share=%d, hh=%d, mm=%d, ss=%d\n", ptr_data->m_sequence, ptr_data->m_exchange_id, ptr_data->m_symbol, ptr_data->m_update_time_h,
					ptr_data->m_update_time_m, ptr_data->m_update_time_s, ptr_data->m_millisecond, ptr_data->m_last_px, ptr_data->m_last_share,
					ptr_data->m_total_value, ptr_data->m_total_pos, ptr_data->m_bid1_px, ptr_data->m_bid1_share, ptr_data->m_ask1_px, ptr_data->m_ask1_share, hh, mm, ss);
                       */
		        if(ptr_data->m_exchange_id =='1'){
                            Types::Instrument_t instrument{""};
                            memcpy(instrument.data(), ptr_data->m_symbol, sizeof(char)*7);
                            auto itr = m_subScribeInstruments.find(instrument);
			 //   fprintf(stderr, "begin print guava_udp_head instrument=%s\n", instrument.data()); 
                            if(itr != m_subScribeInstruments.end()) {
                         //       fprintf(stderr, "begin print sendMarket instrument=%s\n", instrument.data());

                                sendMarket(ptr_data, itr->second, epoch_time);
                            }
                        }
                        proc_len +=  sizeof(guava_udp_head);
                    }

                }
            }
            return NULL;

        }

        bool GuavaL2Market::start_server_event_thread() {


            m_receive_md = std::thread([this]{
                cpu_set_t mask;
                cpu_set_t get;
                CPU_ZERO(&mask);
                CPU_SET(m_affiCore, &mask);
                pthread_t pid = pthread_self();
                if (pthread_setaffinity_np(pid, sizeof(mask), &mask) < 0) {
                    fprintf(stderr, "set thread affinity failed\n");
                }
                CPU_ZERO(&get);
                if (pthread_getaffinity_np(pid, sizeof(get), &get) < 0) {
                    fprintf(stderr, "get thread affinity failed\n");
                }


                if (CPU_ISSET(m_affiCore, &get)) {
                    fprintf(stderr, "this GUAVAl2Market tid {} is running in processor {}", pid, m_affiCore );
                }
                fprintf(stderr, "this GUAVAl2Market=%ld, tid=%ld, cpu=%d\n" , pid, (long int)syscall(SYS_gettid), m_affiCore );
                this->on_socket_server_event_thread();
            });
            m_thrade_quit_flag = false;




            return true;
        }

        bool GuavaL2Market::stop_server_event_thread() {
            m_thrade_quit_flag = true;

            return true;
        }





        void GuavaL2Market::sendMarket(guava_udp_head* line, Types::PushMarket* pushMarket , int64_t epoch_time){
            auto marketData = pushMarket->marketDataList.getNewMemory();
            auto event = pushMarket->eventDataList.getNewMemory();

            marketData->epoch_time = epoch_time;
            marketData->bidPrice[0] = line->m_bid1_px;
            marketData->bidPrice[1] = line->m_bid2_px;
            marketData->bidPrice[2] = line->m_bid3_px;
            marketData->bidPrice[3] = line->m_bid4_px;
            marketData->bidPrice[4] = line->m_bid5_px;
            marketData->bidVolume[0] = line->m_bid1_share;
            marketData->bidVolume[1] = line->m_bid2_share;
            marketData->bidVolume[2] = line->m_bid3_share;
            marketData->bidVolume[3] = line->m_bid4_share;
            marketData->bidVolume[4] = line->m_bid5_share;

            marketData->askPrice[0] = line->m_ask1_px;
            marketData->askPrice[1] = line->m_ask2_px;
            marketData->askPrice[2] = line->m_ask3_px;
            marketData->askPrice[3] = line->m_ask4_px;
            marketData->askPrice[4] = line->m_ask5_px;
            marketData->askVolume[0] = line->m_ask1_share;
            marketData->askVolume[1] = line->m_ask2_share;
            marketData->askVolume[2] = line->m_ask3_share;
            marketData->askVolume[3] = line->m_ask4_share;
            marketData->askVolume[4] = line->m_ask5_share;

            marketData->lastPrice= line->m_last_px;

             memcpy(marketData->instrumentID.data(), line->m_symbol, sizeof(char)*7);
             sprintf(marketData->updateTime.data(), "%02d:%02d:%02d", line->m_update_time_h,line->m_update_time_m,line->m_update_time_s);
             marketData->milliSeconds = line->m_millisecond;
             marketData->psSecond = Utils::ToPsSeconds(marketData->updateTime, marketData->milliSeconds);
             marketData->midPrice = (marketData->bidPrice[0] + marketData->askPrice[0]) *0.5;
            marketData->amount = line->m_total_value;
             marketData->volume = line->m_last_share;

            event->point = marketData;
            event->type =0;
            for(auto i : pushMarket->subscribePolicy){
                m_driver->callback_asyncEventData(event, i);
            }
        //    spdlog::error("sendMarket TradingHours filter error : instrument={}, updateTime={},  useTIme={}", marketData->instrumentID.data(), marketData->updateTime.data(),
        //                  std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count()-  marketData->epoch_time);
        }



        void GuavaL2Market::input_param() {


            m_multi_info.m_remote_ip = "233.35.1.100";
            m_multi_info.m_remote_port = 55000;
            m_multi_info.m_local_ip = "10.10.10.190";
            m_multi_info.m_local_port = 23501;


        }


        bool GuavaL2Market::init() {
//            printf("init : remote_ip=%s, remote_port=%d, local_ip=%s, local_port=%d\n",
//                   m_multi_info.m_remote_ip.c_str(), m_multi_info.m_remote_port, m_multi_info.m_local_ip.c_str(),
//                   m_multi_info.m_local_port);
//            spdlog::info("init : remote_ip={}, remote_port={}, local_ip={}, local_port={}",
//                         m_multi_info.m_remote_ip.c_str(), m_multi_info.m_remote_port, m_multi_info.m_local_ip.c_str(),
//                         m_multi_info.m_local_port);
            bool ret = this->sock_init();
            if (!ret) {
               fprintf(stderr, " m_udp.sock_init ret =%d\n", ret);
                return false;
            }
          //  printf(" m_udp.sock_init ret =%d\n", ret);
            return true;
        }




    }
}
