#define SDL_MAIN_USE_CALLBACKS 1 /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <time.h>

#include "config.h"
#include "display/display.h"
#include "grid/grid.h"

static Display display;
static Grid grid;
static bool left_mouse_pressed = false;
static bool right_mouse_pressed = false;

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
    SDL_srand((Uint64)time(NULL));

    DisplayConfig config = {
        .title = DISPLAY_TITLE,
        .width = DISPLAY_WIDTH,
        .height = DISPLAY_HEIGHT,
        .window_flags = DISPLAY_WINDOW_FLAGS,
        .init_flags = DISPLAY_INIT_FLAGS,
        .presentation = DISPLAY_LOGICAL_PRESENTATION,
    };

    if (!display_initialize(&display, &config)) {
        SDL_Log("Couldn't initialize Display.");
        return SDL_APP_FAILURE;
    }

    if (!grid_initialize(&grid)) {
        SDL_Log("Couldn't initialize Grid.");
        return SDL_APP_FAILURE;
    }

    return SDL_APP_CONTINUE;
}

/* This function runs when a new event (mouse input, keypresses, etc) occurs. */
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
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
            left_mouse_pressed = true;
        } else if (event->button.button == SDL_BUTTON_RIGHT) {
            right_mouse_pressed = true;
        }
    }

    if (event->type == SDL_EVENT_MOUSE_BUTTON_UP) {
        if (event->button.button == SDL_BUTTON_LEFT) {
            left_mouse_pressed = false;
        } else if (event->button.button == SDL_BUTTON_RIGHT) {
            right_mouse_pressed = false;
        }
    }

    return SDL_APP_CONTINUE;
}

/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void *appstate) {
    SDL_SetRenderDrawColor(display.renderer, 0, 0, 0, SDL_ALPHA_OPAQUE); 
    SDL_RenderClear(display.renderer);
    
    // Handle mouse input for placing particles
    float mouse_x, mouse_y;
    SDL_GetMouseState(&mouse_x, &mouse_y);
    
    // Convert screen coordinates to grid coordinates
    Coordinates grid_pos = {
        .x = (int)(mouse_x / PARTICLE_SIZE),
        .y = (int)(mouse_y / PARTICLE_SIZE)
    };
    
    // Place sand if left mouse button is held
    if (left_mouse_pressed && grid_is_in_bounds(grid_pos)) {
        grid_place_particle(&grid, grid_pos, SAND);
    }
    
    // Place rock if right mouse button is held
    if (right_mouse_pressed && grid_is_in_bounds(grid_pos)) {
        grid_place_particle(&grid, grid_pos, ROCK);
    }
    
    grid_update(&grid);
    grid_render(&grid, &display);
    
    SDL_RenderPresent(display.renderer);

    return SDL_APP_CONTINUE;
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    grid_cleanup(&grid);
    display_cleanup(&display);
}
