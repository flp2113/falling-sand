#ifndef FALLING_SAND_DISPLAY_H
#define FALLING_SAND_DISPLAY_H

#include <SDL3/SDL.h>
#include <stdbool.h>

typedef struct display_config {
    const char *title;
    int width;
    int height;
    SDL_WindowFlags window_flags;
    SDL_InitFlags init_flags;
    SDL_RendererLogicalPresentation presentation;
} DisplayConfig;
typedef struct display {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    SDL_InitFlags init_flags;
} Display;

bool display_initialize(Display *display, const DisplayConfig *config);
void display_cleanup(Display *display);

#endif