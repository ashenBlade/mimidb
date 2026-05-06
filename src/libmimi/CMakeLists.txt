add_library(${LIBMIMI_IFACE_NAME} SHARED)

target_include_directories(${LIBMIMI_IFACE_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../include")

target_sources(${LIBMIMI_IFACE_NAME} PRIVATE MimiClient.cpp)
