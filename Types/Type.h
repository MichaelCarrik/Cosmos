//
// Created by zhangyw on 5/10/19.
//

#ifndef TRADEBOTS_TYPE_H
#define TRADEBOTS_TYPE_H

#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/daily_file_sink.h"
#include "spdlog/async.h"
#include <variant>
#include <array>
#include <vector>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <thread>
#include <atomic>
#include <string>
#include <memory.h>
#include <shared_mutex>
#include <filesystem>


namespace Cosmos{
    namespace Types{

        constexpr double g_epsilon = 0.00001;

        constexpr int SingeInstrumenBuffSize = 9;

        constexpr int MarketBuffSize = 90000;

        constexpr int OrderBuffSize = 20000;

        constexpr int ThreadQueueBuffSize = 20000;

        enum class OrderSide {buy,sell};

        enum class HedgeType {hedge,spec};

        enum class OrderTimeType{COMMON,IOC};

        enum class PositionEffectType  {open, T_close, Y_close};

        enum class OrderStatus{signal, cancel, accept, partTraded, allTraded,
            failed, canceled,unknown};

        enum class ExecuteIntension{EIPut, EIHit, EIMid};

        enum class OrderIntension{OIPut, OIHit, OIMid, OINoT};

        enum class ExchangeType{CFFEX, SHFE, ZCE, DCE, INE, GFE};

        enum class ProductClass{future, option};


           //     BuyInventory, SellInventory,
            //SpreadAggressive,  BuyHedgeOpen, SellHedgeOpen, BuyHedgeClose, SellHedgeClose};

        using TradingDay_t = std::array<char,9>;

        using Instrument_t  = std::array<char,35>;
        using Product_t = std::array<char,10>;
        using UpdateTime_t = std::array<char,9>;

        static std::unordered_map<OrderStatus, int>  orderActionMap{{OrderStatus::signal,  1}, {OrderStatus::unknown,  2},
                                                                    {OrderStatus::accept,  3},{OrderStatus::partTraded,  4},
                                                                    {OrderStatus::allTraded,  5}, {OrderStatus::canceled,  5},
                                                                    {OrderStatus::failed,  5}};

        static std::unordered_map<OrderStatus, std::array<char,9>>  orderStatusMap{{OrderStatus::signal,  std::array<char,9>{"signal"}},
                                                                                   {OrderStatus::cancel,  std::array<char,9>{"cancel"}},
                                                                                   {OrderStatus::accept,  std::array<char,9>{"accept"}},{OrderStatus::partTraded,  std::array<char,9>{"pafill"}},
                                                                                   {OrderStatus::allTraded,  std::array<char,9>{"filled"}},{OrderStatus::failed,  std::array<char,9>{"failed"}},
                                                                                   {OrderStatus::canceled,  std::array<char,9>{"canced"}},{OrderStatus::unknown,  std::array<char,9>{"uknown"}}};

        static std::unordered_map<PositionEffectType, std::array<char,9>>  positionEffectMap{
                {PositionEffectType::open,  std::array<char,9>{"open"}},
                {PositionEffectType::T_close,  std::array<char,9>{"todc"}},
                {PositionEffectType::Y_close,  std::array<char,9>{"yedc"}}};

        static std::unordered_map<OrderSide, std::array<char,9>>  orderSideMap{
                {OrderSide::buy,  std::array<char,9>{"buy"}},
                {OrderSide::sell,  std::array<char,9>{"sel"}}};

        static std::unordered_map<HedgeType, std::array<char,9>>  hedgeMap{
                {HedgeType::hedge,  std::array<char,9>{"hedg"}},
                {HedgeType::spec,  std::array<char,9>{"spec"}}};

        static std::unordered_map<ExchangeType, std::array<char,9>>  exchangeMap{
                {ExchangeType::CFFEX,  std::array<char,9>{"cffex"}},
                {ExchangeType::SHFE,  std::array<char,9>{"SHFE"}},
                {ExchangeType::ZCE,  std::array<char,9>{"ZCE"}},
                {ExchangeType::DCE,  std::array<char,9>{"DCE"}},
                {ExchangeType::INE,  std::array<char,9>{"INE"}}
                };

        static std::unordered_map<ExecuteIntension, std::array<char,9>>  EIMap{
                    {ExecuteIntension::EIPut,  std::array<char,9>{"EIPut"}},
                    {ExecuteIntension::EIHit,  std::array<char,9>{"EIHit"}},
                    {ExecuteIntension::EIMid,  std::array<char,9>{"EIMid"}}};

        static const std::unordered_map<std::string, ExecuteIntension> configParamToEIMap{
                    {"EIPut", ExecuteIntension::EIPut},
                    {"EIHit", ExecuteIntension::EIHit},
                    {"EIMid", ExecuteIntension::EIMid}
            };

        static std::unordered_map<OrderIntension, std::array<char,9>>  OIMap{
                        {OrderIntension::OIPut,  std::array<char,9>{"OIPut"}},
                        {OrderIntension::OIHit,  std::array<char,9>{"OIHit"}},
                        {OrderIntension::OIMid,  std::array<char,9>{"OIMid"}}};

        // static const std::unordered_map<std::string, ExecuteIntension> configParamToOIMap{
        //                 {"EIPut", ExecuteIntension::EIPut},
        //                 {"EIHit", ExecuteIntension::EIHit},
        //                 {"EIMid", ExecuteIntension::EIMid}
        // };






        struct InstrumentHash
        {
            std::size_t operator()(const Instrument_t & instrument) const
            {
                std::size_t hash = 0 ;
                for(auto i=0; i<7; i++){
                    hash += hash*128  + instrument[i];
                }
                return hash ;
            }
        };


        template <typename E>
        constexpr auto to_underlying(E e) noexcept
        {
            return static_cast<std::underlying_type_t<E>>(e);
        }
    }
}



#endif //TRADEBOTS_TYPE_H
