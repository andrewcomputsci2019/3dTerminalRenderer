cmake_minimum_required(VERSION 3.10)
project(Terminal3D VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_subdirectory(vendor)



add_library("TermRender" STATIC)

add_executable(${PROJECT_NAME})
list(APPEND SOURCES "src/main.c" "src/utils.c" "src/events.c" "src/render.c" "src/draw.c" "src/ply_utils.c" "src/3d_event_helpers.c")
target_sources(${PROJECT_NAME} PRIVATE ${SOURCES})
target_sources("TermRender" PRIVATE ${SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories("TermRender" PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(${PROJECT_NAME} PRIVATE plyParser)

target_compile_definitions(${PROJECT_NAME} PRIVATE VERSION_MAJOR=${PROJECT_VERSION_MAJOR} VERSION_MINOR=${PROJECT_VERSION_MINOR} VERSION_PATCH=${PROJECT_VERSION_PATCH})

target_link_libraries("TermRender" PRIVATE plyParser)

target_compile_definitions("TermRender" PRIVATE VERSION_MAJOR=${PROJECT_VERSION_MAJOR} VERSION_MINOR=${PROJECT_VERSION_MINOR} VERSION_PATCH=${PROJECT_VERSION_PATCH})


if(DEFINED DEMOS)
	message(STATUS "BUILDING DEMO PROGRAMS")
	add_subdirectory(demos)
endif()