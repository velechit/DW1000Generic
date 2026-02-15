add_library(DWM1000 DW1000Time.cpp DW1000.cpp DW1000Device.cpp DW1000Mac.cpp DW1000Ranging.cpp m_log.cpp)

target_include_directories(DWM1000  PUBLIC ${CMAKE_SOURCE_DIR})

target_include_directories(DWM1000  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
