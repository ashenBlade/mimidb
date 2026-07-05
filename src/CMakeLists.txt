# define useful macro to collect header and source files
# taken from ClickHouse:cmake/dbms_glob_sources.cmake
macro(add_glob cur_list)
    file(GLOB __tmp CONFIGURE_DEPENDS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${ARGN})
    list(APPEND ${cur_list} ${__tmp})
endmacro()

macro(add_headers_and_sources prefix common_path)
    # collect base sources
    add_glob(${prefix}_headers ${common_path}/*.hpp)
    add_glob(${prefix}_sources ${common_path}/*.cpp)

    # collect unit test files
    add_glob(${prefix}_sources_unit ${common_path}/test/gtest_*.cpp)
    list(APPEND ${prefix}_sources_unit ${${prefix}_sources})
    list(APPEND ${prefix}_sources_unit ${${prefix}_headers})
endmacro()

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

add_subdirectory(libmimi)
add_subdirectory(server)
add_subdirectory(client)
