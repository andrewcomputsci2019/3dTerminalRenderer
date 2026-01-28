add_executable("PlyDemo")

target_sources("PlyDemo" PRIVATE "ply_demo.c")

target_link_libraries("PlyDemo" PRIVATE TermRender)
