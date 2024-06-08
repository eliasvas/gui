cmake_minimum_required(VERSION 3.19...3.29)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED TRUE)

project(gui VERSION 1.0 LANGUAGES C)

# Build the gui library
add_library(guiLib STATIC  gui/include/gui.h 
                        gui/src/gui_font.c
                        gui/src/gui_input.c
                        gui/src/gui_render.c
                        gui/src/gui_state.c
                        gui/src/gui_style.c
                        gui/src/gui_key.c
                        gui/src/gui_widget.c
                        gui/src/gui.c
                        gui/ext/stb/stb_truetype.h)
target_include_directories(guiLib PUBLIC gui/include)

# Build the D3D11 sample
IF (WIN32)
    add_executable(sample_d3d11 sample/sample.c sample/d3d11_backend.c)
    target_include_directories(sample_d3d11 PUBLIC gui/include)
    target_link_libraries(sample_d3d11 PUBLIC guiLib)
    target_link_libraries(sample_d3d11 PRIVATE d3d11 dxguid d3dcompiler)
    # Run the D3D11 sample, can be invoked by cmake --build build --target run
    add_custom_target(run
        COMMAND sample_d3d11 
        DEPENDS sample_d3d11 
        WORKING_DIRECTORY ${CMAKE_PROJECT_DIR}
        USES_TERMINAL
    )
    add_dependencies(run sample_d3d11)
ENDIF()