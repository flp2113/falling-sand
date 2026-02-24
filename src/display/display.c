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
    display->texture = NULL;
    display->init_flags = init_flags;

    if (!SDL_InitSubSystem(init_flags))
        goto cleanup;

    if (!SDL_CreateWindowAndRenderer(title, width, height, window_flags, &display->window, &display->renderer))
        goto cleanup;

    display->texture = SDL_CreateTexture(display->renderer, SDL_PIXELFORMAT_RGBA32,
                                         SDL_TEXTUREACCESS_STREAMING, GRID_WIDTH, GRID_HEIGHT);

    if (!display->texture)
        goto cleanup;

    if (!SDL_SetTextureScaleMode(display->texture, SDL_SCALEMODE_NEAREST))
        goto cleanup;

    if (!SDL_SetRenderLogicalPresentation(display->renderer, width, height,
                                          presentation))
        goto cleanup;

    if (!SDL_SetRenderVSync(display->renderer, 1))
        goto cleanup;

    return true;

cleanup:
    display_cleanup(display);
    return false;
}

void display_cleanup(Display *display) {
    if (!display)
        return;

    if (display->texture) {
        SDL_DestroyTexture(display->texture);
        display->texture = NULL;
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