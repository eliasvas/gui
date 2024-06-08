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
    -  [cmake🔪](https://cmake.org/download/) (for building)
    - [clang💻](https://releases.llvm.org/download.html) (for compiling C code)
- #### Building sample
    ```bash
    git clone https://github.com/eliasvas/gui gui/
    cd gui/
    cmake -S . -B build
    cmake --build build --target run 
    ```