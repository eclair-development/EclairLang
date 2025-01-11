cmake_minimum_required(VERSION 3.13.4)
project(eclair VERSION 0.1.0)



find_package(LLVM 15 REQUIRED CONFIG)

if(NOT "15" VERSION_EQUAL "${LLVM_VERSION_MAJOR}")
  message(FATAL_ERROR "Found LLVM ${LLVM_VERSION_MAJOR}, but need LLVM 14")
endif()

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})
list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
include(AddLLVM)

set(LLVM_LINK_COMPONENTS
  BitWriter
  Core
  ExecutionEngine
  MC
  MCJIT
  Support
  nativecodegen
  )

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")

add_llvm_executable(eclair main.cpp
                 lib/Basic/Diagnostic.cpp lib/Basic/TokenKinds.cpp
                 lib/Driver/Driver.cpp
                 lib/CodeGen/Decls.cpp lib/CodeGen/Exprs.cpp lib/CodeGen/Stmts.cpp lib/CodeGen/Types.cpp
                 lib/CodeGen2/Decls.cpp lib/CodeGen2/Exprs.cpp lib/CodeGen2/Stmts.cpp lib/CodeGen2/Types.cpp
                 lib/Lexer/Lexer.cpp lib/Lexer/TokenStream.cpp
                 lib/Parser/Parser.cpp
              )

target_include_directories(eclair PRIVATE "${PROJECT_SOURCE_DIR}/include")

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)