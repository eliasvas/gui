#include <SDL3/SDL.h>
#include <SDL3/SDL_opengles2.h>
#include <SDL3/SDL_main.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct AppContext
{
    SDL_Window *window;
    SDL_GLContext glcontext;
    SDL_bool app_quit;
}AppContext;

int SDL_Fail(){
    SDL_LogError(SDL_LOG_CATEGORY_CUSTOM, "Error %s", SDL_GetError());
    return -1;
}

int SDL_AppInit(void **appstate, int argc, char *argv[])
{
    // init the library, here we make a window so we only need the Video capabilities.
    if (SDL_Init(SDL_INIT_VIDEO))
    {
        return SDL_Fail();
    }
    // create a window
    SDL_Window *window = SDL_CreateWindow("sample", 352, 430, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    SDL_GLContext glcontext = SDL_GL_CreateContext(window);
    if (!glcontext)
    {
        return SDL_Fail();
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

    // set up the application data
    *appstate = malloc(sizeof(AppContext));
    ((AppContext*)*appstate)->window = window;
    ((AppContext*)*appstate)->glcontext = glcontext;
    ((AppContext*)*appstate)->app_quit = 0;
    SDL_Log("Application started successfully!");

    return 0;
}

int SDL_AppEvent(void *appstate, const SDL_Event* event) {
    AppContext *app = (AppContext*)appstate;

    if (event->type == SDL_EVENT_QUIT) {
        app->app_quit = SDL_TRUE;
    }

    return 0;
}

int SDL_AppIterate(void *appstate)
{
    AppContext *app = (AppContext *)appstate;
    // draw a color
    float time = SDL_GetTicks() / 1000.f;
    float red = (sin(time) + 1) / 2.0;
    float green = (sin(time / 2) + 1) / 2.0;
    float blue = (sin(time) * 2 + 1) / 2.0;

    glClearColor(red, green, blue, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    // Draw your stuff here
    SDL_GL_SwapWindow(app->window);

    return app->app_quit;
}

void SDL_AppQuit(void *appstate)
{
    AppContext *app = (AppContext *)appstate;
    if (app)
    {
        SDL_GL_DeleteContext(app->glcontext);
        SDL_DestroyWindow(app->window);
        free(app);
    }
    SDL_Quit();
    SDL_Log("Application quit successfully!");
}