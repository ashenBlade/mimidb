# Сам исполняемый файл БД
add_executable(${SERVER_EXEC_NAME} main.cpp)

# Заголовки
target_include_directories(${SERVER_EXEC_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../include")

# Все поддиректории
add_subdirectory(access)
add_subdirectory(db)
add_subdirectory(lock)
add_subdirectory(storage)
add_subdirectory(transam)
add_subdirectory(worker)

target_link_libraries(${SERVER_EXEC_NAME} PRIVATE ${LIBMIMI_IFACE_NAME})
