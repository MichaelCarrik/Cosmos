//
// Created by zhangyw on 11/16/19.
//

#ifndef HFT_MM_GUAVAL2MARKET_H
#define HFT_MM_GUAVAL2MARKET_H


#pragma once


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <functional>

#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>


#include <string>
#include <map>
#include <spdlog/logger.h>
#include <../Utils/MemoryList.h>
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/async.h"
#include "../Types/MarketData.h"
#include "../Driver/RealtimeDriver.h"
#include "../Types/VarientTypes.h"


using std::string;
using std::map;


#define GUAVA_LEV2_SYMBOL_LEN                        31
#define GUAVA_LEV2_TIME_LEN                            9

#define MY_SOCKET                            int


#define SL_SOCK_DEFAULT                        0
#define SL_SOCK_SEND                        1
#define SL_SOCK_RECV                        2
#define SL_SOCK_ERROR                        3

///socket????????????
#define MY_SOCKET_DEFAULT                    -1
///socket???????
#define MY_SOCKET_ERROR                        -1
///??????????????
#define    RCV_BUF_SIZE                        65535
///???????????????????????????
#define MAX_SOCKET_CONNECT                    1024


namespace TrendFollow {
    namespace Market {
#pragma pack(push, 1)

        struct guava_udp_head {
            unsigned int	m_sequence;				///<会话编号
            char			m_exchange_id;			///<市场  0 表示中金  1表示上期
            char			m_channel_id;			///<通道编号
            char			m_symbol[8];			///<合约
            char			m_update_time_h;		///<最后更新的时间hh
            char			m_update_time_m;		///<最后更新的时间mm
            char			m_update_time_s;		///<最后更新的时间ss
            unsigned short  m_millisecond;		    ///<最后更新的毫秒数

            double			m_last_px;				///<最新价
            unsigned int	m_last_share;			///<最新成交量
            double			m_total_value;			///<成交金额
            double			m_total_pos;			///<持仓量

            double			m_bid1_px;				///<买一价
            unsigned int	m_bid1_share;			///<买一量
            double			m_bid2_px;				///<买二价
            unsigned int	m_bid2_share;			///<买二量
            double			m_bid3_px;				///<买三价
            unsigned int	m_bid3_share;			///<买三量
            double			m_bid4_px;				///<买四价
            unsigned int	m_bid4_share;			///<买四量
            double			m_bid5_px;				///<买五价
            unsigned int	m_bid5_share;			///<买五量

            double			m_ask1_px;				///<卖一价
            unsigned int	m_ask1_share;			///<卖一量
            double			m_ask2_px;				///<卖二价
            unsigned int	m_ask2_share;			///<卖二量
            double			m_ask3_px;				///<卖三价
            unsigned int	m_ask3_share;			///<卖三量
            double			m_ask4_px;				///<卖四价
            unsigned int	m_ask4_share;			///<卖四量
            double			m_ask5_px;				///<卖五价
            unsigned int	m_ask5_share;			///<卖五量

            char            m_reserve;  			///<保留字段
        };


        struct multicast_info {
            std::string m_remote_ip;                        ///< ?��?????????
            unsigned short m_remote_port;                                            ///< ?��?????????
            std::string m_local_ip;                        ///< ?��???????
            unsigned short m_local_port;                                            ///< ?��???????
        };


#pragma pack(pop)

///-----------------------------------------------------------------------------
///??????????
///-----------------------------------------------------------------------------
        enum SOCKET_EVENT {
            EVENT_CONNECT,                //?????????
            EVENT_REMOTE_DISCONNECT,    //??????????
            EVENT_LOCALE_DISCONNECT,    //??????????
            EVENT_NETWORK_ERROR,        //???????
            EVENT_RECEIVE,                //??????????
            EVENT_SEND,                    //?????????????
            EVENT_RECEIVE_BUFF_FULL,    //???????????
            EVENT_UNKNOW,                //��??????
        };



        class GuavaL2Market {
        public:


            GuavaL2Market(Driver::RealtimeDriver *driver, std::string &config_path);

            virtual ~GuavaL2Market(void);

            /// \brief ?��????????
            bool sock_init();

            /// \brief ?��??????
            bool sock_close();

            void run() {
                input_param();

                bool ret = init();
                if (!ret) {
                    string str_temp;


                    return;
                }

                while (true) {
                    sleep(3600);
                }

            }

            bool init();

            int start();

            void SubScribeQuote(Types::SubScribeQuote const & subScribeQuote);

        protected:
            //----------------------------------------------------------------------------
            //????????????
            //----------------------------------------------------------------------------


            /// \brief ?��???????????????(linux ??)
            static void *socket_server_event_thread(void *ptr_param);


            /// \brief ?��??????????????
            void *on_socket_server_event_thread();

            /// \brief ?????��?????????
            bool start_server_event_thread();

            /// \brief ???��?????????
            bool stop_server_event_thread();

            void input_param();



            void sendMarket(guava_udp_head *line ,Types::PushMarket*, int64_t);


        protected:

            bool m_thrade_quit_flag;        ///< ??????????????

//	string					m_remote_ip;			///< ?��IP
//	unsigned short			m_remote_port;			///< ?��???
//	string					m_local_ip;				///< ????IP
//	unsigned short			m_local_port;			///< ??????

            MY_SOCKET m_sock;                    ///< ????
            multicast_info m_multi_info;
            std::string m_configPath;
            int m_revNumb{0};
            int m_affiCore{0};
            char m_exchangeID{'a'};
            bool m_isCheckCode{false};
            int marketId{0};
            Driver::RealtimeDriver *m_driver{nullptr};

            std::unordered_map<Types::Instrument_t, Types::PushMarket*, Types::InstrumentHash> m_subScribeInstruments;


            std::thread m_receive_md;

        };
    }
}



#endif //HFT_MM_GUAVAL2MARKET_H
