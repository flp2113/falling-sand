#include <SDL3/SDL.h>
#include "../config.h"
#include "display.h"

static void display_destroy_resources(Display *display) {
    if (display->texture)
        SDL_DestroyTexture(display->texture);

    if (display->renderer)
        SDL_DestroyRenderer(display->renderer);

    if (display->window)
        SDL_DestroyWindow(display->window);

    SDL_QuitSubSystem(display->init_flags);
    *display = (Display){0};
}

bool display_initialize(Display *display, const DisplayConfig *config) {
    if (!display || !config)
        return false;

    const char *title = config->title;
    int width = config->width;
    int height = config->height;
    SDL_WindowFlags window_flags = config->window_flags;
    SDL_InitFlags init_flags = config->init_flags;
    SDL_RendererLogicalPresentation presentation = config->presentation;

    *display = (Display){0};
    display->init_flags = init_flags;

    if (!SDL_InitSubSystem(init_flags))
        goto failed;

    if (!SDL_CreateWindowAndRenderer(title, width, height, window_flags, &display->window, &display->renderer))
        goto failed;

    display->texture = SDL_CreateTexture(display->renderer, SDL_PIXELFORMAT_RGBA32,
                                         SDL_TEXTUREACCESS_STREAMING, GRID_WIDTH, GRID_HEIGHT);

    if (!display->texture)
        goto failed;

    if (!SDL_SetTextureScaleMode(display->texture, SDL_SCALEMODE_NEAREST))
        goto failed;

    if (!SDL_SetRenderLogicalPresentation(display->renderer, width, height, presentation))
        goto failed;

    if (!SDL_SetRenderVSync(display->renderer, 1))
        goto failed;

    return true;

failed:
    display_destroy_resources(display);
    return false;
}

void display_cleanup(Display *display) {
    if (!display)
        return;

    display_destroy_resources(display);
}
