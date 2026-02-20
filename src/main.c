#define SDL_MAIN_USE_CALLBACKS 1 /* use the callbacks instead of main() */
#include "config.h"
#include "display/display.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

static Display display;

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
    if (!display_initialize(&display, DISPLAY_TITLE, DISPLAY_WIDTH,
                            DISPLAY_HEIGHT, DISPLAY_WINDOW_FLAGS,
                            DISPLAY_INIT_FLAGS, DISPLAY_LOGICAL_PRESENTATION)) {
        SDL_Log("Couldn't initialize display.");
        return SDL_APP_FAILURE;
    }
    return SDL_APP_CONTINUE;
}

/* This function runs when a new event (mouse input, keypresses, etc) occurs. */
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    if (event->type == SDL_EVENT_KEY_DOWN || event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS; /* end the program, reporting success to the OS.
                                 */
    }
    return SDL_APP_CONTINUE;
}

/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void *appstate) {
    const char *message = "Hello World!";
    int w = 0, h = 0;
    float x, y;
    const float scale = 4.0f;

    /* Center the message and scale it up */
    SDL_GetRenderOutputSize(display.renderer, &w, &h);
    SDL_SetRenderScale(display.renderer, scale, scale);
    x = ((w / scale) - SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * SDL_strlen(message)) / 2;
    y = ((h / scale) - SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE) / 2;

    /* Draw the message */
    SDL_SetRenderDrawColor(display.renderer, 0, 0, 0, 255);
    SDL_RenderClear(display.renderer);
    SDL_SetRenderDrawColor(display.renderer, 255, 255, 255, 255);
    SDL_RenderDebugText(display.renderer, x, y, message);
    SDL_RenderPresent(display.renderer);

    return SDL_APP_CONTINUE;
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    display_cleanup(&display);
}
