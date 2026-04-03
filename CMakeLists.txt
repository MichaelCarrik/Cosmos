cmake_minimum_required(VERSION 3.14)

#SET(CMAKE_C_COMPILER "/usr/local/bin/gcc")
#SET(CMAKE_CXX_COMPILER "/usr/local/bin/g++")
project(Cosmos)

set(CMAKE_CXX_STANDARD 17)
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0  -g -ggdb")

INCLUDE_DIRECTORIES(${BOOST_INCLUDE_DIRS})

find_package(Boost )
find_package(Threads REQUIRED)


set(LIB_MYSQL /usr/lib64/mysql/libmysqlclient.so)
set(LIB_CTPMD ${PROJECT_SOURCE_DIR}/dependencies/sdk/ctp/v6.6.9/libthostmduserapi_se.so)
set(LIB_CTPTRADE ${PROJECT_SOURCE_DIR}/dependencies/sdk/ctp/v6.6.9/libthosttraderapi_se.so)
#set(LIB_OSTTRADE ${PROJECT_SOURCE_DIR}/dependencies/sdk/OST/libutapi.so)
#set(LIB_EESMD ${PROJECT_SOURCE_DIR}/dependencies/sdk/EES/lib/libTapQuoteAPI.so)
#set(LIB_EESTRADE ${PROJECT_SOURCE_DIR}/dependencies/sdk/EES/lib/libTapTradeAPI.so)
set(LIB_QUANTLIB /usr/local/lib/libQuantLib.so)

include_directories(/usr/local/include/ql)
include_directories(dependencies/sdk/ctp/v6.6.9)
#include_directories(dependencies/sdk/OST)
#include_directories(dependencies/sdk/EES/include)

include_directories(./KData)
add_subdirectory(KData)

include_directories(./Trader)
add_subdirectory(Trader)

include_directories(./Market)
add_subdirectory(Market)

#include_directories(./KBarEngine)
#add_subdirectory(KBarEngine)

include_directories(./UnderlyEngine)
add_subdirectory(UnderlyEngine)

include_directories(./Utils)
add_subdirectory(Utils)



add_executable(CosmosKBReader KBarEngine/KBarReadEngine.h KBarEngine/kbsRead_main.cpp)
target_link_libraries(CosmosKBReader Market Trader  Utils KData
	        ${LIB_CTPMD} ${LIB_CTPTRADE} ${LIB_QUANTLIB}
		        ${LIB_MYSQL} Threads::Threads stdc++fs)

#add_executable(CosmosKBSaver KBarEngine/kbs_main.cpp
#		UnderlyEngine/Execute.cpp
#		UnderlyEngine/Execute.h)
#target_link_libraries(CosmosKBSaver Market
#		Trader
#		KBarEngine Utils
#		${LIB_CTPMD} ${LIB_QUANTLIB}
#		${LIB_CTPTRADE} ${LIB_MYSQL}
#		Threads::Threads stdc++fs)

#
#add_executable(CosmosHedge_Test UnderlyEngine/UOTradingTest_main.cpp)
#
#target_link_libraries(CosmosHedge_Test Market Trader KData Utils UnderlyEngine
#		${LIB_CTPMD} ${LIB_CTPTRADE} ${LIB_QUANTLIB}
#		${LIB_MYSQL} Threads::Threads stdc++fs)

#add_executable(Underly_Option UnderlyOptionEngine/UOTrading_main.cpp)
#
#target_link_libraries(Underly_Option Market Trader KData Utils UnderlyOptionEngine
#		${LIB_CTPMD} ${LIB_CTPTRADE} ${LIB_QUANTLIB}
#		${LIB_MYSQL} Threads::Threads stdc++fs)







