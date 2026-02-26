#include <SDL3/SDL.h>

#include "grid.h"

bool grid_initialize(Grid *grid) { 
    if (!grid_cleanup(grid)) 
        return false;
    
    grid->update_left_to_right = true;
    return true;
}

bool grid_cleanup(Grid *grid) {
    if (!grid) 
        return false;

    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            grid->particles[y][x] = (Particle){EMPTY, COLOR_EMPTY};
        }
    }

    return true;
}

void grid_update(Grid *grid) {
    if (!grid) 
        return;

    for (int y = GRID_HEIGHT - 1; y >= 0; y--) {
        if (grid->update_left_to_right) {
            for (int x = 0; x < GRID_WIDTH; x++) {
                particle_update_in_grid(grid, (Coordinates){x, y});
            }
        } else {
            for (int x = GRID_WIDTH - 1; x >= 0; x--) {
                particle_update_in_grid(grid, (Coordinates){x, y});
            }
        }
    }

    grid->update_left_to_right = !grid->update_left_to_right;
}

void grid_render(Grid *grid, Display *display) {
    if (!grid || !display || !display->renderer || !display->texture) 
        return;

    int pitch;
    void *pixels;
    if (!SDL_LockTexture(display->texture, NULL, &pixels, &pitch)) {
        SDL_Log("Couldn't lock texture: %s", SDL_GetError());
        return;
    }

    for (int y = 0; y < GRID_HEIGHT; y++) {
        Uint8 *row = (Uint8 *)pixels + y * pitch;
        for (int x = 0; x < GRID_WIDTH; x++) {
            SDL_Color color = grid->particles[y][x].color;
            Uint8 *pixel = row + x * 4;
            pixel[0] = color.r;
            pixel[1] = color.g;
            pixel[2] = color.b;
            pixel[3] = color.a;
        }
    }

    SDL_UnlockTexture(display->texture);
    SDL_RenderTexture(display->renderer, display->texture, NULL, NULL);
}

bool grid_set_particle(Grid *grid, Coordinates coordinates, const Particle *particle) {
    if (!grid || !grid_is_in_bounds(coordinates)) 
        return false;

    grid->particles[coordinates.y][coordinates.x] = *particle;
    return true;
}

bool grid_place_particle(Grid *grid, Coordinates coordinates, ParticleType type) {
    if (!grid || !grid_is_in_bounds(coordinates)) 
        return false;

    Particle particle = {type, particle_get_random_color_by_type(type)};
    return grid_set_particle(grid, coordinates, &particle);
}

const Particle *grid_get_particle(Grid *grid, Coordinates coordinates) {
    if (!grid || !grid_is_in_bounds(coordinates)) 
        return NULL;
        
    return &grid->particles[coordinates.y][coordinates.x];
}

bool grid_is_empty(Grid *grid) {
    if (!grid) 
        return false;

    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            if (grid->particles[y][x].type != EMPTY) return false;
        }
    }

    return true;
}

bool grid_is_particle_empty(Grid *grid, Coordinates coordinates) {
    if (!grid || !grid_is_in_bounds(coordinates)) 
        return false;

    return particle_is_empty(&grid->particles[coordinates.y][coordinates.x]);
}

bool grid_is_in_bounds(Coordinates coordinates) {
    return (coordinates.x >= 0 && coordinates.x < GRID_WIDTH && coordinates.y >= 0 && coordinates.y < GRID_HEIGHT);
}

void grid_apply_brush(Grid *grid, Coordinates center, int radius, ParticleType type) {
    if (!grid || !grid_is_in_bounds(center) || radius < 0)
        return;

    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
            if (dx * dx + dy * dy > radius * radius)
                continue;

            Coordinates pos = {center.x + dx, center.y + dy};
            if (!grid_is_in_bounds(pos))
                continue;

            grid_place_particle(grid, pos, type);
        }
    }
}
