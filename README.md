# gui
### _Gud™️ UI library in C_

This is an ever-changing UI library I work in my spare time.
Main inspiration are Ryan's [blog posts](https://www.rfleury.com/p/ui-series-table-of-contents) on Immediate-Mode GUI design.
An interactive demo can be viewed [here](https://pages.github.com/).
### Features
- Zero dependencies C99 codebase
- Loads of layouting options
- Provided sample with multiple supported backends
### sample
| Backend         | Win | Linux | Mac | Web |
|-----------------|:---:|:-----:|:---:|:---:|
| [Direct3D11](https://github.com/eliasvas/gui/blob/main/sample/d3d11_backend.c)   | ✔️  |   ❌  | ❌  | ❌  |
| [OpenGL](https://github.com/eliasvas/gui/blob/main/sample/gl_backend.c)      | ✔️  |  ✔️   | ❌  | 🚧  |
| [sdl_gpu](https://github.com/libsdl-org/SDL_shader_tools/blob/main/docs/README-SDL_gpu.md)     | 🚧  |  🚧   | 🚧  | ❌  |
### Building
- #### Deps
    - [git🧰](https://git-scm.com/downloads)
    - [cmake🔪](https://cmake.org/download/)
    - [MSVC/clang💻](https://releases.llvm.org/download.html)
- #### Building sample
    ```bash
    git clone https://github.com/eliasvas/gui gui/
    cd gui/
    cmake -S . -B build/ -D CMAKE_BUILD_TYPE=Release
    cmake --build build --target run
    ```