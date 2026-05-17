add_library(${LIBMIMI_IFACE_NAME} SHARED)

target_include_directories(${LIBMIMI_IFACE_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_sources(${LIBMIMI_IFACE_NAME} PRIVATE 
    src/MimiClient.cpp
    src/NetworkReader.cpp
    src/NetworkWriter.cpp
    src/serialization.cpp

    src/packets/QueryPacket.cpp
    src/packets/ErrorResponsePacket.cpp
    src/packets/DataRowPacket.cpp
    src/packets/TupleDescriptionPacket.cpp
    src/packets/CommandCompletePacket.cpp
)
