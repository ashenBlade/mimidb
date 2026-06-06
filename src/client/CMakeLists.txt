add_executable(${CLIENT_EXEC_NAME} main.cpp)

target_include_directories(${CLIENT_EXEC_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(${CLIENT_EXEC_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../libmimi/src")

# Link program options
find_package(Boost 1.83.0 COMPONENTS program_options REQUIRED)

if (Boost_FOUND)
    target_include_directories(${CLIENT_EXEC_NAME} PRIVATE ${Boost_INCLUDE_DIRS})
    target_link_libraries(${CLIENT_EXEC_NAME} PRIVATE ${Boost_LIBRARIES})
endif()

target_sources(${CLIENT_EXEC_NAME} PRIVATE
    ClientOptions.cpp
    CommandSource.cpp
    formatting/TableFormatter.cpp)

target_link_libraries(${CLIENT_EXEC_NAME} PRIVATE ${LIBMIMI_IFACE_NAME})

install(TARGETS ${CLIENT_EXEC_NAME}
        DESTINATION ${INSTALL_BINDIR})
set_target_properties(
    ${CLIENT_EXEC_NAME} PROPERTIES
    INSTALL_RPATH "\${ORIGIN}/../lib"
)
