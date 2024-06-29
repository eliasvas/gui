#include "gui.h"
#define SDL_MAIN_NOIMPL
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengles2.h>
#include <SDL3/SDL_main.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

// globals
SDL_Window *window;
SDL_GLContext glcontext;

int SDL_Fail(){ SDL_LogError(SDL_LOG_CATEGORY_CUSTOM, "Error %s", SDL_GetError()); return -1; }



void platform_init(u8 *font_atlas_data) {
    // init the library, here we make a window so we only need the Video capabilities.
    if (SDL_Init(SDL_INIT_VIDEO))
    {
        return;
    }
    // create a window
    window = SDL_CreateWindow("sample", 800, 600, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    glcontext = SDL_GL_CreateContext(window);
    if (!glcontext)
    {
        return;
    }

    // print some information about the window
    SDL_ShowWindow(window);
    {
        int width, height, bbwidth, bbheight;
        SDL_GetWindowSize(window, &width, &height);
        SDL_GetWindowSizeInPixels(window, &bbwidth, &bbheight);
        SDL_Log("Window size: %ix%i", width, height);
        SDL_Log("Backbuffer size: %ix%i", bbwidth, bbheight);
        if (width != bbwidth){
            SDL_Log("This is a highdpi environment.");
        }
    }
    SDL_Log("Application started successfully!");
}

void platform_update() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT)exit(1);
    }
}

vec2 platform_get_windim() {
    int ww,wh;
    SDL_GetWindowSize(window, &ww, &wh);
    return v2(ww,wh);
}

void platform_render(guiRenderCommand *rcommands, u32 command_count)
{
    float time = SDL_GetTicks() / 1000.f;
    float red = (sin(time) + 1) / 2.0;
    float green = (sin(time / 2) + 1) / 2.0;
    float blue = (sin(time) * 2 + 1) / 2.0;
    glClearColor(red, green, blue, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    // Here we draw
    SDL_GL_SwapWindow(window);
}

void platform_deinit() {}