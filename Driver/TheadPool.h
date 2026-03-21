//
// Created by zhangyw on 5/8/19.
//



#ifndef TRADEBOTS_THEADPOOL_H
#define TRADEBOTS_THEADPOOL_H

#include <vector>
#include <queue>
#include <thread>


#include "folly/ProducerConsumerQueue.h"
#include "../Types/MarketData.h"
#include "../Types/OrderField.h"
#include <sched.h>
#include "folly/SpinLock.h"
#include "../Types/SubPolicy.h"
#include <sys/syscall.h>


namespace TrendFollow {
    namespace ThreadPool {

        class ThreadPool {
            std::vector<std::thread> pool;
            std::vector<folly::ProducerConsumerQueue<const  Types::EventData*>* > _marketDataQueueVector;
            std::vector<folly::ProducerConsumerQueue<const  Types::EventData*>* > _orderDataQueueVector;
            std::vector<folly::ProducerConsumerQueue<const  Types::EventData*>* > _engineComStatQueueVector;

        public:

            void detachThread(){
                for(auto& pl: pool){
                    pl.detach();
                }
            }

            inline ThreadPool(unsigned short engineSize = 0) {

            }

            inline ~ThreadPool() {

//                for (std::thread &thread : pool) {
//
//                    if (thread.joinable())
//                        thread.join();
//                }
            }

        public:

           void  setPolicySize(int engineSize){
                for (auto i = 0; i < engineSize; i++) {
                    _marketDataQueueVector.emplace_back(new folly::ProducerConsumerQueue<const  Types::EventData*>{ Types::MarketBuffSize});
                    _orderDataQueueVector.emplace_back(new folly::ProducerConsumerQueue<const  Types::EventData*>{ Types::ThreadQueueBuffSize});
                    _engineComStatQueueVector.emplace_back(new folly::ProducerConsumerQueue<const  Types::EventData*>{ Types::ThreadQueueBuffSize});
                }


            }


            void commitEventData(const  Types::EventData * eventData , int threadId){
                while(!_marketDataQueueVector[threadId]->write(eventData)){
                    continue;
                };
            }



            template<class PolicyEngine>
            void _subscribePolicy( Types::SubscribeEngine const& subscribeEngine, PolicyEngine  * policyEngine) {
                fprintf(stdout, "_subscribeEngine  name=%s, policyid=%d , m_affiThreadId=%d\n",
                        subscribeEngine.policyName.c_str(), subscribeEngine.policyid, subscribeEngine.m_affiThreadId);
                auto marketQueue = _marketDataQueueVector[subscribeEngine.policyid];
                auto orderQueue = _orderDataQueueVector[subscribeEngine.policyid];
                auto engineStatQueue = _engineComStatQueueVector[subscribeEngine.policyid];
                int affiThreadId = subscribeEngine.m_affiThreadId;
                pool.emplace_back(std::thread([policyEngine, marketQueue, orderQueue, engineStatQueue, affiThreadId] {

                    //   std::function<void()> task;
                    if (affiThreadId > 0) {
                        cpu_set_t mask;
                        cpu_set_t get;
                        CPU_ZERO(&mask);
                        CPU_SET(affiThreadId, &mask);
                        pthread_t pid = pthread_self();
                        if (pthread_setaffinity_np(pid, sizeof(mask), &mask) < 0) {
                            fprintf(stderr, "set thread affinity failed\n");
                        }
                        CPU_ZERO(&get);
                        if (pthread_getaffinity_np(pid, sizeof(get), &get) < 0) {
                            fprintf(stderr, "get thread affinity failed\n");
                        }


                        if (CPU_ISSET(affiThreadId, &get)) {
                            spdlog::info("this {}  quote task thread {} is running in processor {}",
                                         policyEngine->m_engineName, pid, affiThreadId);
                        } else {
                            spdlog::error("this {} quote task thread {} is not running in processor {}",
                                          policyEngine->m_engineName, pid, affiThreadId);

                        }

                        fprintf(stderr, "name=%s, this eventThreadId=%ld, tid=%ld, cpu=%d\n",
                                policyEngine->m_engineName.c_str(), pid, (long int) syscall(SYS_gettid), affiThreadId);
                    }

                    while (true) {
                        auto market = marketQueue->frontPtr();
                        if (market != nullptr) {
                            policyEngine->onEventData(**market);
                            marketQueue->popFront();
                        }

                        auto order = orderQueue->frontPtr();
                        if (order != nullptr) {
                            policyEngine->onEventData(**order);
                            orderQueue->popFront();
                        }
                    }
                }));

            }

        };
    }

}


#endif //TRADEBOTS_THEADPOOL_H
