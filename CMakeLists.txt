cmake_minimum_required(VERSION 3.28)

project(mimidb VERSION 1.0 LANGUAGES CXX)

# Follow C++ standard strictly
if(NOT CMAKE_CXX_EXTENSIONS)
    set(CMAKE_CXX_EXTENSIONS OFF)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Бьем по рукам за любые ошибки
add_compile_options(
    -Werror
    -Wall
    -Wextra
    -Wpedantic
    
    -Wcast-align
    -Wcast-qual
    -Wconversion
    -Wctor-dtor-privacy
    -Wenum-compare
    -Wfloat-equal
    -Wnon-virtual-dtor
    -Wold-style-cast
    -Woverloaded-virtual
    -Wredundant-decls
    -Wsign-conversion
    -Wsign-promo
)

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

# Рекурсивно собираем все
add_subdirectory(src)