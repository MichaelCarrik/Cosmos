//
// Created by zhangyw on 5/11/19.
//

#ifndef TRADEBOTS_ORDERLIST_H
#define TRADEBOTS_ORDERLIST_H


#include <atomic>
#include <array>
#include "cstring"


namespace Cosmos {
    namespace Utils {
        template<typename T , std::size_t _resize_numb>
        class MemoryList {
        public:
            MemoryList(int input_index): index_atomic(input_index)  {

            }
            virtual ~MemoryList(){

            }

            T *getNewMemory(int &assignID) {
         //       std::lock_guard<std::mutex> lock(_getnew_mutex);
                int index = index_atomic++;
                int id = index % _resize_numb;
                T *t_order = &_orderList[id];
                if (index >= _resize_numb) {
                    memset(t_order, 0, sizeof(T));
                }

                assignID = index;
                return t_order;
            };

            T *getNewMemory() {
            //    std::lock_guard<std::mutex> lock(_getnew_mutex);
                int index = index_atomic++;
                int id = index % _resize_numb;
                T *t_order = &_orderList[id];
                if (index >= _resize_numb) {
                    memset(t_order, 0, sizeof(T));
                }

                return t_order;
            };



            //only one spi can modify T
            T *getMemoryById(int id) {
                if (id < index_atomic) {
                    return &_orderList[id % _resize_numb];
                } else {
                    return nullptr;
                }

            };

        private:
    //        std::mutex _getnew_mutex;
            std::array<T , _resize_numb> _orderList;
            int index_atomic{0};
        };

    }
}

#endif //TRADEBOTS_ORDERLIST_H
