add_executable(mbus_parse ${CMAKE_CURRENT_LIST_DIR}/mbus_parse.c)
target_link_libraries(mbus_parse PRIVATE libmbus::libmbus)

add_executable(mbus_parse_hex ${CMAKE_CURRENT_LIST_DIR}/mbus_parse_hex.c)
target_link_libraries(mbus_parse_hex PRIVATE libmbus::libmbus)

add_test(mbus_parse_hex ${CMAKE_BINARY_DIR}/bin/mbus_parse_hex)
add_test(mbus_parse ${CMAKE_BINARY_DIR}/bin/mbus_parse)
