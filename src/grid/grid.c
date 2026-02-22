#include <SDL3/SDL.h>

#include "grid.h"

bool grid_initialize(Grid *grid) { return grid_clear(grid); }

bool grid_cleanup(Grid* grid) { return grid_clear(grid); }

bool grid_clear(Grid *grid) {
    if (!grid) {
        SDL_Log("Couldn't find grid at Grid cleaning.");
        return false;
    }

    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            grid->particles[y][x] = (Particle){.type = EMPTY, .color = COLOR_EMPTY};
        }
    }

    return true;
}

void grid_update(Grid *grid) {
    static bool left_to_right = true;

    if (!grid) {
        SDL_Log("Couldn't find Grid at updating Grid.");
        return;
    }

    for (int y = GRID_HEIGHT - 1; y >= 0; y--) {
        if (left_to_right) {
            for (int x = 0; x < GRID_WIDTH; x++) {
                particle_update_in_grid(grid, (Coordinates){x, y});
            }
        } else {
            for (int x = GRID_WIDTH - 1; x >= 0; x--) {
                particle_update_in_grid(grid, (Coordinates){x, y});
            }
        }
    }

    left_to_right = !left_to_right;
}

void grid_render(Grid *grid, Display *display) {
    if (!grid) {
        SDL_Log("Couldn't find Grid at rendering Grid.");
        return;
    }

    if (!display) {
        SDL_Log("Couldn't find Display at rendering Grid.");
        return;
    }

    if (!display->renderer) {
        SDL_Log("Couldn't find Renderer at rendering Grid.");
        return;
    }

    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            if (!particle_is_empty(&grid->particles[y][x])) {
                particle_render(display, &grid->particles[y][x], (Coordinates){x, y});
            }
        }
    }
}

bool grid_set_particle(Grid *grid, Coordinates coordinates,
                       const Particle *particle) {
    if (!grid) {
        SDL_Log("Couldn't find grid at setting particle.");
        return false;
    }

    if (!grid_is_in_bounds(coordinates)) {
        SDL_Log("Not proper grid X/Y coordinates at setting particle.");
        return false;
    }

    if (!particle) {
        SDL_Log("Couldn't find particle at setting particle.");
        return false;
    }

    grid->particles[coordinates.y][coordinates.x] = *particle;

    return true;
}

bool grid_place_particle(Grid *grid, Coordinates coordinates,
                         ParticleType type) {
    if (!grid) {
        SDL_Log("Couldn't find grid at placing particle.");
        return false;
    }

    if (!grid_is_in_bounds(coordinates)) {
        SDL_Log("Not proper grid X/Y position at placing particle.");
        return false;
    }

    Particle particle = {.type = type,
                         .color = particle_get_random_color_by_type(type)};

    return grid_set_particle(grid, coordinates, &particle);
}

const Particle *grid_get_particle(Grid *grid, Coordinates coordinates) {
    if (!grid) {
        SDL_Log("Couldn't find grid at getting particle.");
        return NULL;
    }

    if (!grid_is_in_bounds(coordinates)) {
        SDL_Log("Not proper grid X/Y position at getting particle.");
        return NULL;
    }

    return &grid->particles[coordinates.y][coordinates.x];
}

bool grid_is_empty(Grid *grid) {
    if (!grid) {
        SDL_Log("Couldn't find grid at checking if it's empty.");
        return false;
    }

    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            if (grid->particles[y][x].type != EMPTY) {
                return false;
            }
        }
    }

    return true;
}

bool grid_is_particle_empty(Grid *grid, Coordinates coordinates) {
    if (!grid) {
        SDL_Log("Couldn't find Grid at checking if particle is empty in Grid.");
        return false;
    } 

    if (!grid_is_in_bounds(coordinates)) {
        return false;
    }

    return grid->particles[coordinates.y][coordinates.x].type == EMPTY;
}

bool grid_is_in_bounds(Coordinates coordinates) {
    return (coordinates.x >= 0 && coordinates.x < GRID_WIDTH &&
            coordinates.y >= 0 && coordinates.y < GRID_HEIGHT);
}
