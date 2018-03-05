cmake_minimum_required(VERSION 2.8)
project(libmbus)

add_definitions(-DWIN32 -D_WIN32 -D__FUNCSIG__="not-available") 

include_directories(../mbus/ ../test/ ../win/)

add_library(mbus SHARED ../mbus/mbus.c ../mbus/mbus-protocol.c ../mbus/mbus-protocol-aux.c ../mbus/mbus-serial.c ../mbus/mbus-tcp.c ../win/termiWin.c)

add_executable(mbus-serial-request-data ../bin/mbus-serial-request-data.c)
target_link_libraries(mbus-serial-request-data mbus)

add_executable(mbus-serial-scan-secondary ../bin/mbus-serial-scan-secondary.c)
target_link_libraries(mbus-serial-scan-secondary mbus)

add_executable(mbus-serial-scan ../bin/mbus-serial-scan.c)
target_link_libraries(mbus-serial-scan mbus)

add_executable(mbus-serial-select-secondary ../bin/mbus-serial-select-secondary.c)
target_link_libraries(mbus-serial-select-secondary mbus)

add_executable(mbus_parse ../test/mbus_parse.c)
target_link_libraries(mbus_parse mbus)

add_executable(mbus_parse_hex ../test/mbus_parse_hex.c)
target_link_libraries(mbus_parse_hex mbus)
