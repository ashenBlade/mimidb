cmake_minimum_required(VERSION 3.28)

project(mimidb VERSION 1.0 LANGUAGES CXX)

# Follow C++ standard strictly
if(NOT CMAKE_CXX_EXTENSIONS)
    set(CMAKE_CXX_EXTENSIONS OFF)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Отладка, пока могу безусловно добавлять, но оставлю пока так
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(
        -g
        -O0
    )
endif()

# Общие именования бинарей
set(SERVER_EXEC_NAME mimidb)
set(LIBMIMI_IFACE_NAME mimi)
set(CLIENT_EXEC_NAME miclient)
set(SQLPARSER_LIB_NAME hyrisesql)

# Данные для установки
set(INSTALL_BINDIR  "bin")
set(INSTALL_LIBDIR  "lib")
include(GNUInstallDirs)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Unit test
enable_testing()
add_subdirectory(external/googletest)
include(GoogleTest)

# Рекурсивно собираем все
add_subdirectory(external/sql-parser)
add_subdirectory(src)
