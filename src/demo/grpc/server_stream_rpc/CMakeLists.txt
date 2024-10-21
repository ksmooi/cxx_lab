# https://github.com/protocolbuffers/protobuf/blob/main/docs/cmake_protobuf_generate.md

set(PROTO_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(PROTO_IMPORT_DIRS "${CMAKE_CURRENT_LIST_DIR}/proto")

file(MAKE_DIRECTORY ${PROTO_BINARY_DIR})
file(MAKE_DIRECTORY ${PROTO_IMPORT_DIRS})

# message(STATUS "== CMAKE_CURRENT_LIST_DIR: ${CMAKE_CURRENT_LIST_DIR}")
# message(STATUS "== CMAKE_CURRENT_BINARY_DIR: ${CMAKE_CURRENT_BINARY_DIR}")
# message(STATUS "== PROTO_BINARY_DIR: ${PROTO_BINARY_DIR}")
# message(STATUS "== PROTO_IMPORT_DIRS: ${PROTO_IMPORT_DIRS}")

# --------------------------------------------------------------------------------------
# Add the library for streaming.proto
# --------------------------------------------------------------------------------------

add_library(
    streaming_proto 
    OBJECT "${CMAKE_CURRENT_LIST_DIR}/proto/streaming.proto"
)
target_link_libraries(
    streaming_proto
    PUBLIC protobuf::libprotobuf
    PUBLIC gRPC::grpc
    absl::log_internal_globals
)
target_include_directories(
    streaming_proto 
    PUBLIC "$<BUILD_INTERFACE:${PROTO_BINARY_DIR}>"
)

# generate the protobuf code
protobuf_generate(
    TARGET streaming_proto
    LANGUAGE cpp
    IMPORT_DIRS ${PROTO_IMPORT_DIRS}
    PROTOC_OUT_DIR "${PROTO_BINARY_DIR}"
)

# generate the grpc code
protobuf_generate(
    TARGET streaming_proto
    LANGUAGE grpc
    GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc
    PLUGIN "protoc-gen-grpc=\$<TARGET_FILE:gRPC::grpc_cpp_plugin>"
    IMPORT_DIRS ${PROTO_IMPORT_DIRS}
    PROTOC_OUT_DIR "${PROTO_BINARY_DIR}"
)

    
# --------------------------------------------------------------------------------------
# Add the executable for grpc_ss_server
# --------------------------------------------------------------------------------------

add_executable(grpc_ss_server grpc_ss_server.cpp)
target_link_libraries(grpc_ss_server PRIVATE
    streaming_proto
    gRPC::grpc++
    gRPC::grpc++_reflection
    protobuf::libprotobuf
    absl::log_internal_globals
)
target_include_directories(grpc_ss_server PRIVATE "${PROTO_BINARY_DIR}")

# --------------------------------------------------------------------------------------
# Add the executable for grpc_ss_client
# --------------------------------------------------------------------------------------

add_executable(grpc_ss_client grpc_ss_client.cpp)
target_include_directories(grpc_ss_client PRIVATE "${PROTO_BINARY_DIR}")
target_link_libraries(grpc_ss_client PRIVATE
    streaming_proto
    gRPC::grpc++
    gRPC::grpc++_reflection
    protobuf::libprotobuf
    absl::log_internal_globals
    boost_program_options
)
