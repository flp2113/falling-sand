#ifndef DISPLAY_H
#define DISPLAY_H

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
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
    SDL_InitFlags init_flags;
} Display;

bool display_initialize(Display *display, DisplayConfig config);
void display_cleanup(Display *display);

#endif