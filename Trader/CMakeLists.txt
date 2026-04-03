aux_source_directory(. DIR_TRADER_SRCS)
add_library(Trader
        MockTrader.h  MockTrader.cpp
      #   CtpTrader.h  CtpTrader.cpp
        )
