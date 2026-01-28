add_executable("CubeDemo")

target_sources("CubeDemo" PRIVATE "cube_demo.c")

target_link_libraries("CubeDemo" PRIVATE TermRender)