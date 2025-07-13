cmake_minimum_required(VERSION 3.30)
project(alpha_c)

set(CMAKE_CXX_STANDARD 20)

# Add include directories
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/external)

# Source files using glob
file(GLOB_RECURSE SOURCE_FILES 
    ${PROJECT_SOURCE_DIR}/src/*.cpp
)

# Create executable
add_executable(${PROJECT_NAME} 
    extra/main.cpp 
    ${SOURCE_FILES}
)

# Add any required libraries
target_link_libraries(${PROJECT_NAME}
    PRIVATE
)

set(LSP_SOURCES
    lsp/lsp_server.cpp
    lsp/json_rpc_handler.cpp
    lsp/json_serialization.cpp
    lsp/main.cpp
    lsp/notification_handler.cpp
)

add_executable(alpha_lsp ${LSP_SOURCES} ${SOURCE_FILES})

add_executable(test_compiler tests/test_compiler.cpp ${SOURCE_FILES})
add_executable(test_semantic tests/test_semantic.cpp ${SOURCE_FILES})
add_executable(test_codegen tests/test_codegen.cpp ${SOURCE_FILES})
add_executable(test_syscall tests/test_syscall.cpp ${SOURCE_FILES})
add_executable(debug_symbols tests/debug_symbols.cpp ${SOURCE_FILES})
add_executable(debug_test tests/debug_test.cpp ${SOURCE_FILES})


