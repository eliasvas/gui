# gui (name WIP)
### _simple GUI library in C99_

This is an experiment on a hybrid UI system.
Major goals are portability, maintainability and expresiveness.
Main inspiration are Ryan's [blog posts](https://www.rfleury.com/p/ui-series-table-of-contents) on Immediate-Mode GUI design.
An interactive demo can be viewed [here](https://pages.github.com/).
### Features
- Zero dependencies C99 codebase
- Loads of layouting options (thanks to Retained state)
- stb-style header only version can be produced for easy integration
- drop-in implementations for various Graphics/Input APIs
### Building
- #### Deps
    - [git🧰](https://git-scm.com/downloads) (for cloning the repo)
    -  [python3🐍](https://www.python.org/downloads/) (for running the build scripts)
    - [clang💻](https://releases.llvm.org/download.html) (for compiling C code)
- #### Building sample
    ```bash
    git clone https://github.com/eliasvas/gui gui/
    cd gui/sample/
    python3 build.py --backend d3d11
    cd .build/
    sample_d3d11.exe
    ```
- #### Produce Single-header lib
    ```bash
    git clone https://github.com/eliasvas/gui gui/
    cd gui/gui/
    python3 produce.py gui_sh.h
    cp gui_sh.h my/code/path/ext/inc/gui_sh.h
    ```
    ```C++
    #define GUI_SH_IMPLEMENTATION
    #include "gui_sh.h"
    guiState my_gui;
    int main() {
        //..
        gui_state_init(&my_gui);
        //..
    }
    ```