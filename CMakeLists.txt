cmake_minimum_required(VERSION 3.10)
project(Terminal3D VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(${PROJECT_NAME})
list(APPEND SOURCES "src/main.c" "src/utils.c" "src/events.c" "src/render.c")
target_sources(${PROJECT_NAME} PRIVATE ${SOURCES}  "src/draw.c")
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)


target_compile_definitions(${PROJECT_NAME} PRIVATE VERSION_MAJOR=${PROJECT_VERSION_MAJOR} VERSION_MINOR=${PROJECT_VERSION_MINOR} VERSION_PATCH=${PROJECT_VERSION_PATCH})