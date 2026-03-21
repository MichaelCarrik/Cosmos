//
// Created by zhangyw on 5/8/19.
//

#ifndef TRADEBOTS_DRIVER_TYPEID_H
#define TRADEBOTS_DRIVER_TYPEID_H





#include <utility>
#include <vector>
#include <functional>

namespace TrendFollow {
    namespace Driver {

        template<typename T>
        int type_id();

        template<typename T>
        void type_id(int id);

        namespace detail {

//allocate incremental type_id
            inline int alloc_type_id()
            {
                static int id = 0;
                return id++;
            }


/** Allocate & Store unique, int, incremental type_id
 * ...in static variable's ctor.
 */
            template<typename T>
            class TypeID
            {
                friend int  Driver::type_id<T>();
                friend void  Driver::type_id<T>(int id);

                struct TypeRegister
                {
                    TypeRegister() {
                         Driver::type_id<T>(alloc_type_id());
                    }
                };

                static int _type_id;
                static TypeRegister _register __attribute__((used));
            };

            template<typename T>
            int TypeID<T>::_type_id;

            template<typename T>
            typename TypeID<T>::TypeRegister TypeID<T>::_register;

        } // namespace detial

/** Non-intrusive Get Type's type_id, include PODs
 *
 * class Object {
 *     ...
 * };
 *
 * cout << "Object : "type_id<Object>() << endl;
 * cout << "int : "type_id<int>() << endl;
 * cout << "double : "type_id<double>() << endl;
 * ...
 *
 * $ ./a.out
 * Object : 0
 * int : 1
 * double : 2
 * $
 *
 * warning: do NOT guess the order of ids...
 *
 */
        template<typename T>
        inline int type_id()
        {
            return detail::TypeID<T>::_type_id;
        }

/** Setter
 * ...Not be recommend to call it manually
 */
        template<typename T>
        inline void type_id(int id)
        {
            detail::TypeID<T>::_type_id = id;
        }


    } // namespace driver
} // namespace cerie


#endif //TRADEBOTS_DRIVER_TYPEID_H
