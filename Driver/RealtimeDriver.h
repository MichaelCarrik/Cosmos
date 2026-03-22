//
// Created by zhangyw on 5/8/19.
//


#ifndef TRADEBOTS_DRIVER_REALTIMEDRIVER_H
#define TRADEBOTS_DRIVER_REALTIMEDRIVER_H

#include "../Types/Type.h"
#include "../Types/VarientTypes.h"

#include "../Driver/TheadPool.h"
#include "../Types/SubScribeQuote.h"
#include <any>
#include "TypeID.hpp"
#include <ctime>

//#define TESTMODEL


namespace Cosmos {
    namespace Driver {

        class RealtimeDriver {
        private:

        public:

             ThreadPool::ThreadPool _threadPool;

            RealtimeDriver(const RealtimeDriver &) = delete;

            RealtimeDriver &operator=(const RealtimeDriver) = delete;


            RealtimeDriver() : _threadPool() {

            }



            template<typename T, typename ...Args>
            void add_receiver(std::function<void(T const &, Args const &...args)> const &cb) {
                auto id = type_id<T>();
                if (id >= _callbacks.size()) {
                    _callbacks.resize(id + 1);
                }
                _callbacks[id].emplace_back(cb);
            }


            template<typename T, typename ...Args>
            void send(T const &data, Args const & ...args) {
                //  Stopwatch sw("send");
                auto id = type_id<T>();
                for (auto &b : _callbacks[id]) {
                    auto cf = std::any_cast<std::function<void(T const &, decltype(args)...)> const>(&b);
                    if (cf) {
                        (*cf)(data, args...);
                        continue;
                    }

                    assert(false && "send Unknown callback function type.");
                }
            }

            std::vector<std::vector<std::any>> _callbacks;


            void setPolicySize(int engineSize) {
                 _threadPool.setPolicySize(engineSize);
            }


            void callback_asyncEventData(const  Types::EventData* data, int id) {
                _threadPool.commitEventData(data,id);
            }


            void sendOrder( Types::OrderField const &order) {
                _sendOrderCallbacks(order);
            }

            void cancelOrder( Types::OrderField const &order, int64_t& epoch_time) {
                _cancelOrderCallbacks(order, epoch_time);

            }

            void sendAbtrgOrder(Types::ArbitrageOrderField const &order) {
                _sendAbtrgOrderCallbacks(order);
            }

            void cancelAbtrgOrder(Types::ArbitrageOrderField const &order, int64_t& epoch_time) {
                _cancelAbtrgOrderCallbacks(order, epoch_time);
            }

            void subscribeQuote( Types::SubScribeQuote const &subScribeQuote) {

                _subscribeQuoteCallbacks(subScribeQuote);

            }

            template <typename  Engine>
             void subscribePolicy( Types::SubscribeEngine const & subscribeEngine, Engine  * engine){
                _threadPool._subscribePolicy(subscribeEngine, engine);
            }

            void onStart(){
              _threadPool.detachThread();
            }


            void register_sendOrderFunction(std::function<void( Types::OrderField const &)> &&cb) {
                _sendOrderCallbacks = cb;
            }

            void register_cancelOrderFunction(std::function<void( Types::OrderField const &, int64_t &)> &&cb) {
                _cancelOrderCallbacks = cb;
            }

            void register_sendAbtrgOrderFunction(std::function<void(Types::ArbitrageOrderField const &)> &&cb) {
                _sendAbtrgOrderCallbacks = cb;
            }

            void register_cancelAbtrgOrderFunction(std::function<void(Types::ArbitrageOrderField const &, int64_t &)> &&cb) {
                _cancelAbtrgOrderCallbacks = cb;
            }

            void register_subscribeQuoteFunction(std::function<void( Types::SubScribeQuote const &)> &&cb) {
                _subscribeQuoteCallbacks = cb;
            }

            template<class RetVal, class T, class... Args>
            std::function<RetVal(Args...)> get_fun_type(RetVal (T::*)(Args...) const);

            template<class RetVal, class T, class... Args>
            std::function<RetVal(Args...)> get_fun_type(RetVal (T::*)(Args...));

            template<class T>
            auto passn(T t) -> decltype(get_fun_type(&T::operator())) {
                return t;
            }


            std::function<void( Types::OrderField const &)> _sendOrderCallbacks;
            std::function<void( Types::OrderField const &, int64_t&)> _cancelOrderCallbacks;
            std::function<void( Types::SubScribeQuote const &)> _subscribeQuoteCallbacks;

            std::function<void(Types::ArbitrageOrderField const &)> _sendAbtrgOrderCallbacks;
            std::function<void(Types::ArbitrageOrderField const &, int64_t&)> _cancelAbtrgOrderCallbacks;

        };

    }
}


#endif //TRADEBOTS_DRIVER_REALTIMEDRIVER_H
