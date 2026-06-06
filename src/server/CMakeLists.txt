# Сам исполняемый файл БД
add_executable(${SERVER_EXEC_NAME} main.cpp)

# Заголовки
target_include_directories(${SERVER_EXEC_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(${SERVER_EXEC_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../libmimi/src")
target_include_directories(${SERVER_EXEC_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../external/sql-parser/src")

macro(add_server path)
    add_headers_and_sources(dbms ${path})
endmacro()

add_server(.)

add_server(access/heap)
add_server(access/heap/wal)
add_server(access/heap/undo)

add_server(access/table)

add_server(db)
add_server(db/builtin)
add_server(db/catalog)

add_server(executor)
add_server(executor/expr)
add_server(executor/func)
add_server(executor/plan)

add_server(lock)

add_server(logger)

add_server(parser)
add_server(planner)

add_server(storage)
add_server(storage/buffer)
add_server(storage/buffer/internal)
add_server(storage/io)
add_server(storage/undo)
add_server(storage/wal)

add_server(trans)
add_server(utils)
add_server(worker)

target_sources(${SERVER_EXEC_NAME} PRIVATE ${dbms_sources} ${dbms_headers})

# Network client
target_link_libraries(${SERVER_EXEC_NAME} PRIVATE ${LIBMIMI_IFACE_NAME})

# SQL parser
target_link_libraries(${SERVER_EXEC_NAME} PUBLIC ${SQLPARSER_LIB_NAME})

# Link program options
find_package(Boost 1.83.0 COMPONENTS program_options REQUIRED)

if (Boost_FOUND)
    target_include_directories(${SERVER_EXEC_NAME} PRIVATE ${Boost_INCLUDE_DIRS})
    target_link_libraries(${SERVER_EXEC_NAME} PRIVATE ${Boost_LIBRARIES})
endif()

# Installation
install(TARGETS ${SERVER_EXEC_NAME}
        DESTINATION ${INSTALL_BINDIR})

set_target_properties(
    ${SERVER_EXEC_NAME} PROPERTIES
    INSTALL_RPATH "\${ORIGIN}/../lib"
)
