aux_source_directory(. DIR_KBarEngine_SRCS)
add_library(KBarEngine ${DIR_KBarEngine_SRCS} KBarSaverEngine.h KBarSaverEngine.cpp)