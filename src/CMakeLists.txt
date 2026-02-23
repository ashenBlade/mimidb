cmake_minimum_required(VERSION 3.28)

project(mimidb VERSION 1.0 LANGUAGES CXX)

# Follow C++ standard strictly
if(NOT CMAKE_CXX_EXTENSIONS)
    set(CMAKE_CXX_EXTENSIONS OFF)
endif()

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

# Сам исполняемый файл БД
add_executable(${PROJECT_NAME}
    main.cpp
    
    access/heap/HeapPage.cpp
    access/heap/HeapTable.cpp
    access/heap/HeapTableScan.cpp
    
    storage/Buffer.cpp
    storage/BufferManager.cpp
    storage/BufferWrapper.cpp
    
    transam/Snapshot.cpp
    )

# Заголовочные файлы
target_include_directories(${PROJECT_NAME} PUBLIC "include")

