#define SDL_MAIN_USE_CALLBACKS 1 /* use the callbacks instead of main() */

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <time.h>

#include "config/display_config.h"
#include "display/display.h"
#include "grid/grid.h"

typedef struct app_state {
    Display display;
    Grid grid;
    bool left_mouse_pressed;
    ParticleType particle_in_use;
    int brush_radius;
    Uint64 last_tick;
    double accumulator;
} AppState;

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
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
        display_destroy(&state->display);
        SDL_free(state);
        return SDL_APP_FAILURE;
    }

    state->brush_radius = DEFAULT_BRUSH_RADIUS;
    state->particle_in_use = SAND;
    state->last_tick = SDL_GetPerformanceCounter();
    state->accumulator = 0.0;

    SDL_srand((Uint64)time(NULL));

    *appstate = state;
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    AppState *state = appstate;

    if (event->type == SDL_EVENT_QUIT) 
        return SDL_APP_SUCCESS;

    if (event->type == SDL_EVENT_KEY_DOWN) {
        switch (event->key.key) {
            case SDLK_ESCAPE: return SDL_APP_SUCCESS;
            case SDLK_1: state->particle_in_use = SAND; break;
            case SDLK_2: state->particle_in_use = ROCK; break;
            case SDLK_3: state->particle_in_use = EMPTY; break;
            case SDLK_R: grid_reset(&state->grid); break;
            default: break;
        }
    } 

    if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        if (event->button.button == SDL_BUTTON_LEFT) 
            state->left_mouse_pressed = true;
    }

    if (event->type == SDL_EVENT_MOUSE_BUTTON_UP) {
        if (event->button.button == SDL_BUTTON_LEFT) 
            state->left_mouse_pressed = false;
    }

    if (event->type == SDL_EVENT_MOUSE_WHEEL) {
        if (event->wheel.y > 0) {
            if (state->brush_radius < MAX_BRUSH_RADIUS)
                state->brush_radius++;
        } else if (event->wheel.y < 0) {
            if (state->brush_radius > MIN_BRUSH_RADIUS)
                state->brush_radius--;
        }
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
    AppState *state = (AppState *)appstate;
    if (!state || !state->display.renderer) 
        return SDL_APP_FAILURE;

    Uint64 now = SDL_GetPerformanceCounter();
    double dt = (double)(now - state->last_tick) / (double)SDL_GetPerformanceFrequency();
    state->last_tick = now;
    state->accumulator += dt;

    float mouse_x, mouse_y;
    SDL_GetMouseState(&mouse_x, &mouse_y);
    Coordinates coordinates = {(int)(mouse_x / PARTICLE_SIZE), (int)(mouse_y / PARTICLE_SIZE)};
    if (state->left_mouse_pressed && grid_is_in_bounds(coordinates)) 
        grid_apply_brush(&state->grid, coordinates, state->brush_radius, state->particle_in_use);

    while (state->accumulator >= SIMULATION_TICK_RATE) {
        grid_update(&state->grid);
        state->accumulator -= SIMULATION_TICK_RATE;
    }

    grid_render(&state->grid, &state->display);
    SDL_RenderPresent(state->display.renderer);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    AppState *state = appstate;
    if (state) {
        display_destroy(&state->display);
        SDL_free(state);
    }
}
