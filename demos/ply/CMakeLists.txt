add_executable("PlyDemo")

target_sources("PlyDemo" PRIVATE "ply_demo.c")

target_link_libraries("PlyDemo" PRIVATE TermRender)

set(PlyDemo_ASSETS 
    ${CMAKE_SOURCE_DIR}/resources/teapot.ply
    ${CMAKE_SOURCE_DIR}/resources/bunny.ply
    ${CMAKE_SOURCE_DIR}/resources/dragon.ply
)

foreach(asset ${PlyDemo_ASSETS})
    add_custom_command(
        TARGET PlyDemo
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${asset}
            $<TARGET_FILE_DIR:PlyDemo>
    )
endforeach()