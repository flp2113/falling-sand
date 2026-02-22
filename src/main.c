#define SDL_MAIN_USE_CALLBACKS 1 /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <time.h>

#include "config.h"
#include "display/display.h"
#include "grid/grid.h"

typedef struct app_state {
    Display display;
    Grid grid;
    bool left_mouse_pressed;
    bool right_mouse_pressed;
} AppState;

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
    SDL_srand((Uint64)time(NULL));

    AppState *state = SDL_calloc(1, sizeof(AppState));
    if (!state) {
        SDL_Log("Couldn't allocate AppState.");
        return SDL_APP_FAILURE;
    }

    DisplayConfig config = {
        .title = DISPLAY_TITLE,
        .width = DISPLAY_WIDTH,
        .height = DISPLAY_HEIGHT,
        .window_flags = DISPLAY_WINDOW_FLAGS,
        .init_flags = DISPLAY_INIT_FLAGS,
        .presentation = DISPLAY_LOGICAL_PRESENTATION,
    };

    if (!display_initialize(&state->display, &config)) {
        SDL_Log("Couldn't initialize Display.");
        SDL_free(state);
        return SDL_APP_FAILURE;
    }

    if (!grid_initialize(&state->grid)) {
        SDL_Log("Couldn't initialize Grid.");
        display_cleanup(&state->display);
        SDL_free(state);
        return SDL_APP_FAILURE;
    }

    *appstate = state;
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    AppState *state = appstate;

    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }

    if (event->type == SDL_EVENT_KEY_DOWN) {
        if (event->key.key == SDLK_ESCAPE) {
            return SDL_APP_SUCCESS;
        }
    }

    if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        if (event->button.button == SDL_BUTTON_LEFT) {
            state->left_mouse_pressed = true;
        } else if (event->button.button == SDL_BUTTON_RIGHT) {
            state->right_mouse_pressed = true;
        }
    }

    if (event->type == SDL_EVENT_MOUSE_BUTTON_UP) {
        if (event->button.button == SDL_BUTTON_LEFT) {
            state->left_mouse_pressed = false;
        } else if (event->button.button == SDL_BUTTON_RIGHT) {
            state->right_mouse_pressed = false;
        }
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
    AppState *state = (AppState *)appstate;
    
    if (!state) {
        return SDL_APP_FAILURE;
    }

    if (!state->display.renderer) {
        return SDL_APP_FAILURE;
    }

    SDL_SetRenderDrawColor(state->display.renderer, 0, 0, 0, SDL_ALPHA_OPAQUE); 
    SDL_RenderClear(state->display.renderer);
    
    float mouse_x, mouse_y;
    SDL_GetMouseState(&mouse_x, &mouse_y);
    
    Coordinates coordinates = {
        .x = (int)(mouse_x / PARTICLE_SIZE),
        .y = (int)(mouse_y / PARTICLE_SIZE)
    };
    
    if (state->left_mouse_pressed && grid_is_in_bounds(coordinates)) {
        grid_place_particle(&state->grid, coordinates, SAND);
    }
    
    if (state->right_mouse_pressed && grid_is_in_bounds(coordinates)) {
        grid_place_particle(&state->grid, coordinates, ROCK);
    }
    
    grid_update(&state->grid);
    grid_render(&state->grid, &state->display);
    
    SDL_RenderPresent(state->display.renderer);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    AppState *state = appstate;
    if (state) {
        grid_cleanup(&state->grid);
        display_cleanup(&state->display);
        SDL_free(state);
    }
}
