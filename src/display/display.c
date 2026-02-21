#include "display.h"

bool display_initialize(Display *display, DisplayConfig config) {
    if (!display) {
        SDL_Log("Couldn't find Display at initialization.");
        return false;
    }

    const char *title = config.title;
    int width = config.width;
    int height = config.height;
    SDL_WindowFlags window_flags = config.window_flags;
    SDL_InitFlags init_flags = config.init_flags;
    SDL_RendererLogicalPresentation presentation = config.presentation;

    display->window = NULL;
    display->renderer = NULL;
    display->init_flags = init_flags;

    if (!SDL_InitSubSystem(init_flags)) {
        SDL_Log("Couldn't initialize SDL subsystem: %s", SDL_GetError());
        return false;
    }

    if (!SDL_CreateWindowAndRenderer(title, width, height, window_flags,
                                     &display->window, &display->renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        SDL_QuitSubSystem(init_flags);
        return false;
    }

    if (!SDL_SetRenderLogicalPresentation(display->renderer, width, height,
                                          presentation)) {
        SDL_Log("Couldn't set logical presentation: %s", SDL_GetError());
        SDL_DestroyRenderer(display->renderer);
        SDL_DestroyWindow(display->window);
        SDL_QuitSubSystem(init_flags);
        display->renderer = NULL;
        display->window = NULL;
        return false;
    }

    return true;
}

void display_cleanup(Display *display) {
    if (!display) {
        SDL_Log("Couldn't find Display at cleanup.");
        return;
    }

    if (display->renderer) {
        SDL_DestroyRenderer(display->renderer);
        display->renderer = NULL;
    }

    if (display->window) {
        SDL_DestroyWindow(display->window);
        display->window = NULL;
    }

    SDL_QuitSubSystem(display->init_flags);
}