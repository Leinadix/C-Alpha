cmake_minimum_required(VERSION 3.20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
enable_testing()

project(alpha_c)


include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/external)


file(GLOB_RECURSE CORE_SOURCES
        ${PROJECT_SOURCE_DIR}/src/*.cpp
)

add_library(core_objects OBJECT ${CORE_SOURCES})


add_executable(${PROJECT_NAME}
        extra/main.cpp
)
target_link_libraries(${PROJECT_NAME}
        PRIVATE $<TARGET_OBJECTS:core_objects>
)


set(LSP_SOURCES
        lsp/lsp_server.cpp
        lsp/json_rpc_handler.cpp
        lsp/json_serialization.cpp
        lsp/main.cpp
        lsp/notification_handler.cpp
)
add_executable(alpha_lsp ${LSP_SOURCES})
target_link_libraries(alpha_lsp
        PRIVATE $<TARGET_OBJECTS:core_objects>
)

add_executable(test_compiler tests/test_compiler.cpp)
target_link_libraries(test_compiler PRIVATE $<TARGET_OBJECTS:core_objects>)
add_test(NAME test_compiler COMMAND test_compiler)


add_executable(test_semantic tests/test_semantic.cpp)
target_link_libraries(test_semantic PRIVATE $<TARGET_OBJECTS:core_objects>)
add_test(NAME test_semantic COMMAND test_semantic)

add_executable(test_codegen tests/test_codegen.cpp)
target_link_libraries(test_codegen PRIVATE $<TARGET_OBJECTS:core_objects>)
add_test(NAME test_codegen COMMAND test_codegen)

add_executable(test_syscall tests/test_syscall.cpp)
target_link_libraries(test_syscall PRIVATE $<TARGET_OBJECTS:core_objects>)
add_test(NAME test_syscall COMMAND test_syscall)

add_executable(test_symbols tests/debug_symbols.cpp)
target_link_libraries(test_symbols PRIVATE $<TARGET_OBJECTS:core_objects>)
add_test(NAME test_symbols COMMAND test_symbols)

add_executable(test_overall tests/debug_test.cpp)
target_link_libraries(test_overall PRIVATE $<TARGET_OBJECTS:core_objects>)
add_test(NAME test_overall COMMAND test_overall)

