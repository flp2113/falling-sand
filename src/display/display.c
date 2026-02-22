#include <SDL3/SDL.h>
#include "display.h"
#include "../config.h"

bool display_initialize(Display *display, const DisplayConfig *config) {
    if (!display) {
        SDL_Log("Couldn't find Display at Display initialization.");
        return false;
    }

    if (!config) {
        SDL_Log("Couldn't find DisplayConfig at Display initialization.");
        return false;
    }

    const char *title = config->title;
    int width = config->width;
    int height = config->height;
    SDL_WindowFlags window_flags = config->window_flags;
    SDL_InitFlags init_flags = config->init_flags;
    SDL_RendererLogicalPresentation presentation = config->presentation;

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
        goto quit_subsystem;
    }

    display->texture = SDL_CreateTexture(
        display->renderer,
        SDL_PIXELFORMAT_RGBA32,
        SDL_TEXTUREACCESS_STREAMING,
        GRID_WIDTH, GRID_HEIGHT
    );

    if (!display->texture) {
        SDL_Log("Couldn't create texture: %s", SDL_GetError());
        goto destroy_all;
    }

    if (!SDL_SetTextureScaleMode(display->texture, SDL_SCALEMODE_NEAREST)) {
        SDL_Log("Couldn't set texture scale mode: %s", SDL_GetError());
        goto destroy_all;
    }

    if (!SDL_SetRenderLogicalPresentation(display->renderer, width, height,
                                          presentation)) {
        SDL_Log("Couldn't set logical presentation: %s", SDL_GetError());
        goto destroy_all;
    }

    if (!SDL_SetRenderVSync(display->renderer, 1)) {
        SDL_Log("Couldn't set Vsync: %s", SDL_GetError());
        goto destroy_all;
    }

    return true;

destroy_all:
    SDL_DestroyRenderer(display->renderer);
    SDL_DestroyWindow(display->window);
    SDL_DestroyTexture(display->texture);
    SDL_QuitSubSystem(init_flags);
    display->renderer = NULL;
    display->window = NULL;
    display->texture = NULL;
quit_subsystem:
    SDL_QuitSubSystem(init_flags);
    return false;
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

    if (display->texture) {
        SDL_DestroyTexture(display->texture);
        display->texture = NULL;
    }

    SDL_QuitSubSystem(display->init_flags);
}