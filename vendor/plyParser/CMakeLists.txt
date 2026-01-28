add_library(plyParser STATIC rply.c)
target_include_directories(plyParser PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)