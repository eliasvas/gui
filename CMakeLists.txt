cmake_minimum_required(VERSION 3.19...3.29)

# To build with different backends you can
# cmake -DUSE_OPENGL=ON -DUSE_D3D11=OFF -DUSE_SDL_GPU=OFF

# set the output directory for built objects.
# This makes sure that the dynamic library goes into the build directory automatically.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

# prevent installing to system directories.
set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}" CACHE INTERNAL "")

option(USE_OPENGL "Build with OpenGL support" ON)
option(USE_D3D11 "Build with Direct3D 11 support" OFF)
option(USE_SDL_GPU "Build with SDL_gpu support" OFF)

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
                        gui/src/gui_misc.c
                        gui/ext/stb/stb_truetype.h)
target_include_directories(guiLib PUBLIC gui/include)
if (DEFINED EMSCRIPTEN)
	set_target_properties(guiLib PROPERTIES COMPILE_FLAGS "-sWASM=1 -sSIDE_MODULE=1 -sSTANDALONE_WASM -sALLOW_MEMORY_GROWTH=1 -sMAXIMUM_MEMORY=2GB")
endif()



if (USE_D3D11)
    add_executable(sample_d3d11 sample/sample.c sample/d3d11_backend.c)
    target_include_directories(sample_d3d11 PUBLIC gui/include)
    target_link_libraries(sample_d3d11 PUBLIC guiLib)
    target_link_libraries(sample_d3d11 PRIVATE d3d11 dxguid d3dcompiler)
endif()

if(USE_OPENGL)
    add_executable(sample_gl sample/sample.c sample/gl_backend.c)
    target_include_directories(sample_gl PUBLIC gui/include)
    target_link_libraries(sample_gl PUBLIC guiLib)
    if(CMAKE_SYSTEM_NAME MATCHES Emscripten)
        #set(CMAKE_EXECUTABLE_SUFFIX ".html" CACHE INTERNAL "")
        #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s USE_SDL=2 -s FULL_ES3=1")
        set(USE_FLAGS "-s USE_SDL=2 -s INITIAL_MEMORY=100MB -s MAXIMUM_MEMORY=4000MB -s ALLOW_MEMORY_GROWTH=1 -s STACK_SIZE=5MB")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${USE_FLAGS}")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${USE_FLAGS}")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${USE_FLAGS}")
        set(CMAKE_EXECUTABLE_SUFFIX .html)
    elseif(CMAKE_SYSTEM_NAME MATCHES Windows)
        include(FetchContent)
        FetchContent_Declare(
            SDL
            GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
            GIT_TAG SDL2
        )
        FetchContent_MakeAvailable(SDL)
        # Fetch and link GLEW
        FetchContent_Declare(
            GLEW
            GIT_REPOSITORY https://github.com/Perlmint/glew-cmake.git
            GIT_TAG glew-cmake-2.1.0
        )
        FetchContent_MakeAvailable(GLEW)

        find_package(OpenGL REQUIRED)
        target_link_libraries(sample_gl PUBLIC SDL2::SDL2 OpenGL::GL libglew_static)
        target_link_libraries(sample_gl PUBLIC opengl32 gdi32 imm32)
    else()
        find_package(SDL2 REQUIRED)
        include_directories(${SDL2_INCLUDE_DIRS})
        target_link_libraries(sample_gl PUBLIC m ${SDL2_LIBRARIES} GLESv2 EGL)
    endif()
endif()