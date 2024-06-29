cmake_minimum_required(VERSION 3.19...3.29)

# set the output directory for built objects.
# This makes sure that the dynamic library goes into the build directory automatically.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

# prevent installing to system directories.
set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}" CACHE INTERNAL "")

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED TRUE)

project(gui VERSION 1.0 LANGUAGES C)

# Build the gui library
add_library(guiLib STATIC  gui/include/gui.h
                        gui/src/gui_font.c
                        gui/src/gui_input.c
                        gui/src/gui_render.c
                        gui/src/gui_state.c
                        gui/src/gui_key.c
                        gui/src/gui_stacks.c
                        gui/src/gui_box.c
                        gui/src/gui_build.c
                        gui/src/gui_layout.c
                        gui/ext/stb/stb_truetype.h)
target_include_directories(guiLib PUBLIC gui/include)

# Build the D3D11 sample
# IF (WIN32)
#     add_executable(sample_d3d11 sample/sample.c sample/d3d11_backend.c)
#     target_include_directories(sample_d3d11 PUBLIC gui/include)
#     target_link_libraries(sample_d3d11 PUBLIC guiLib)
#     target_link_libraries(sample_d3d11 PRIVATE d3d11 dxguid d3dcompiler)
#     # Run the D3D11 sample, can be invoked by cmake --build build --target run
#     add_custom_target(run
#         COMMAND sample_d3d11
#         DEPENDS sample_d3d11
#         WORKING_DIRECTORY ${CMAKE_PROJECT_DIR}
#         USES_TERMINAL
#     )
#     add_dependencies(run sample_d3d11)
# ENDIF()

IF (WIN32)
    add_executable(sample_gl sample/sample.c sample/gl_backend.c)
    target_include_directories(sample_gl PUBLIC gui/include)
    target_link_libraries(sample_gl PUBLIC guiLib)
    if(CMAKE_SYSTEM_NAME MATCHES Emscripten)
        set(CMAKE_EXECUTABLE_SUFFIX ".html" CACHE INTERNAL "")
    endif()
    include(FetchContent)

    FetchContent_Declare(
        SDL
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG main
    )
    FetchContent_MakeAvailable(SDL)

    find_package(OpenGL REQUIRED)
    target_link_libraries(sample_gl PUBLIC SDL3::SDL3 OpenGL::GL)
    target_compile_definitions(sample_gl PUBLIC SDL_MAIN_USE_CALLBACKS)
ENDIF()