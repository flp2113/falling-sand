#include "display.h"
#include "../config.h"
#include <SDL3/SDL.h>

bool display_initialize(Display *display, const DisplayConfig *config) {
    if (!display || !config)
        return false;

    const char *title = config->title;
    int width = config->width;
    int height = config->height;
    SDL_WindowFlags window_flags = config->window_flags;
    SDL_InitFlags init_flags = config->init_flags;
    SDL_RendererLogicalPresentation presentation = config->presentation;

    display->window = NULL;
    display->renderer = NULL;
    display->init_flags = init_flags;

    if (!SDL_InitSubSystem(init_flags))
        return false;

    if (!SDL_CreateWindowAndRenderer(title, width, height, window_flags, &display->window, &display->renderer))
        goto quit_subsystem;

    display->texture = SDL_CreateTexture(display->renderer, SDL_PIXELFORMAT_RGBA32,
                                         SDL_TEXTUREACCESS_STREAMING, GRID_WIDTH, GRID_HEIGHT);

    if (!display->texture)
        goto destroy_all;

    if (!SDL_SetTextureScaleMode(display->texture, SDL_SCALEMODE_NEAREST))
        goto destroy_all;

    if (!SDL_SetRenderLogicalPresentation(display->renderer, width, height,
                                          presentation))
        goto destroy_all;

    if (!SDL_SetRenderVSync(display->renderer, 1))
        goto destroy_all;

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
    if (!display)
        return;

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