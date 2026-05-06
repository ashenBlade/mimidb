# Сам исполняемый файл БД
add_executable(${PROJECT_NAME} main.cpp)

# Заголовки
target_include_directories(${PROJECT_NAME} PUBLIC "include")

# Все поддиректории
add_subdirectory(access)
add_subdirectory(db)
add_subdirectory(lock)
add_subdirectory(storage)
add_subdirectory(transam)
add_subdirectory(worker)

# Клиент
add_executable(client client.cpp)