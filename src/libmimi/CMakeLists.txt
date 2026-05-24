add_library(${LIBMIMI_IFACE_NAME} SHARED)

target_include_directories(${LIBMIMI_IFACE_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

add_headers_and_sources(libmimi src)
add_headers_and_sources(libmimi src/packets)

target_sources(${LIBMIMI_IFACE_NAME} PRIVATE ${libmimi_sources} ${libmimi_headers})
