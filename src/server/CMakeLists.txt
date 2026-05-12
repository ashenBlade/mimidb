# Сам исполняемый файл БД
add_executable(${SERVER_EXEC_NAME}
    main.cpp
    cluster_state.cpp)

# Заголовки
target_include_directories(${SERVER_EXEC_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(${SERVER_EXEC_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../libmimi/include")

# Все поддиректории
add_subdirectory(access)
add_subdirectory(db)
add_subdirectory(lock)
add_subdirectory(storage)
add_subdirectory(trans)
add_subdirectory(worker)
add_subdirectory(logger)

target_link_libraries(${SERVER_EXEC_NAME} PRIVATE ${LIBMIMI_IFACE_NAME})
