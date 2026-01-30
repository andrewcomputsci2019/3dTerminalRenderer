cmake_minimum_required(VERSION 3.10)
project(Terminal3D VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

option(BACK_FACE "Enable backface culling" OFF)
option(DEMOS "Enable demos builds" OFF)

add_subdirectory(vendor)




add_library("TermRender" STATIC)

add_executable(${PROJECT_NAME})
list(APPEND SOURCES "src/utils.c" "src/events.c" "src/render.c" "src/draw.c" "src/ply_utils.c" "src/3d_event_helpers.c")
target_sources(${PROJECT_NAME} PRIVATE "src/main.c")
target_sources("TermRender" PRIVATE ${SOURCES})
target_include_directories("TermRender" PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)




if(BACK_FACE)
	message(STATUS "SETTING BACK FACE ENABLED")
	target_compile_definitions("TermRender" PRIVATE BACKFACE_CULL_ENABLED=1)
endif()

target_link_libraries("TermRender" PRIVATE plyParser)

target_compile_definitions("TermRender" PRIVATE VERSION_MAJOR=${PROJECT_VERSION_MAJOR} VERSION_MINOR=${PROJECT_VERSION_MINOR} VERSION_PATCH=${PROJECT_VERSION_PATCH})

target_link_libraries(${PROJECT_NAME} PRIVATE TermRender)

if(DEMOS)
	message(STATUS "BUILDING DEMO PROGRAMS")
	add_subdirectory(demos)
endif()