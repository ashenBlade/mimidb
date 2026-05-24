# define useful macro to collect header and source files
# taken from ClickHouse:cmake/dbms_glob_sources.cmake
macro(add_glob cur_list)
    file(GLOB __tmp CONFIGURE_DEPENDS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${ARGN})
    list(APPEND ${cur_list} ${__tmp})
endmacro()

macro(add_headers_and_sources prefix common_path)
    add_glob(${prefix}_headers ${common_path}/*.hpp)
    add_glob(${prefix}_sources ${common_path}/*.cpp)
endmacro()

add_subdirectory(libmimi)
add_subdirectory(server)
add_subdirectory(client)
