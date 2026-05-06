add_executable(${CLIENT_EXEC_NAME} client.cpp)

target_include_directories(${CLIENT_EXEC_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(${CLIENT_EXEC_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../include")

target_link_libraries(${CLIENT_EXEC_NAME} PRIVATE ${LIBMIMI_IFACE_NAME})
