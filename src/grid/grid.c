#include <SDL3/SDL.h>
#include <stdbool.h>

#include "config/color_config.h"
#include "config/simulation_config.h"
#include "particle/particle.h"
#include "grid/grid.h"

bool grid_initialize(Grid* grid) { 
    if (!grid) 
        return false;
    
    if (!grid_reset(grid))
        return false;

    grid->update_left_to_right = true;
    grid->dirty = false;
    grid->current_gen = 0;

    return true;
}

bool grid_reset(Grid* grid) {
    if (!grid) 
        return false;

    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            grid->particles[y][x] = (Particle){.type = EMPTY, .color = EMPTY_BASE_COLOR, .update_gen = 0};
        }
    }

    grid->dirty = true;

    return true;
}

void grid_swap(Grid* grid, Coordinates source, Coordinates destination) {
    if (!grid || !grid_is_in_bounds(source) || !grid_is_in_bounds(destination))
        return;

    Particle temporary_particle = grid->particles[source.y][source.x];
    grid->particles[source.y][source.x] = grid->particles[destination.y][destination.x];
    grid->particles[destination.y][destination.x] = temporary_particle;

    grid->particles[destination.y][destination.x].update_gen = grid->current_gen;
    grid->dirty = true;
}

static void particle_update_sand(Grid* grid, Coordinates coordinates) {
    if (coordinates.y + 1 >= GRID_HEIGHT)
        return;

    Coordinates below = {coordinates.x, coordinates.y + 1};
    Coordinates left = {coordinates.x - 1, coordinates.y};
    Coordinates right = {coordinates.x + 1, coordinates.y};
    Coordinates below_left = {coordinates.x - 1, coordinates.y + 1};
    Coordinates below_right = {coordinates.x + 1, coordinates.y + 1};

    bool is_below_empty = grid_is_particle_empty(grid, below);
    bool is_below_left_empty = grid_is_particle_empty(grid, below_left);
    bool is_below_right_empty = grid_is_particle_empty(grid, below_right);
    bool can_go_below_left = is_below_left_empty && !grid_is_particle_solid(grid, left);
    bool can_go_below_right = is_below_right_empty && !grid_is_particle_solid(grid, right);

    if (is_below_empty) {
        grid_swap(grid, coordinates, below);
        return;
    }

    if (!can_go_below_left && !can_go_below_right) {
        return;
    }

    if (can_go_below_left && can_go_below_right) {
        bool go_left = SDL_rand(2);
        grid_swap(grid, coordinates, go_left ? below_left : below_right);
        return;
    }

    if (can_go_below_left) {
        grid_swap(grid, coordinates, below_left);
        return;
    }

    if (can_go_below_right) {
        grid_swap(grid, coordinates, below_right);
        return;
    }
}

static void grid_update_particle(Grid* grid, Coordinates coordinates) {
    if (!grid || !grid_is_in_bounds(coordinates))
        return;

    Particle* p = &grid->particles[coordinates.y][coordinates.x];
    if (p->update_gen == grid->current_gen)
        return;

    switch (p->type) {
        case SAND: particle_update_sand(grid, coordinates); break;
        default: break;
    }
}

void grid_update(Grid* grid) {
    if (!grid) 
        return;

    grid->current_gen++;

    for (int y = GRID_HEIGHT - 1; y >= 0; y--) {
        if (grid->update_left_to_right) {
            for (int x = 0; x < GRID_WIDTH; x++) {
                grid_update_particle(grid, (Coordinates){x, y});
            }
        } else {
            for (int x = GRID_WIDTH - 1; x >= 0; x--) {
                grid_update_particle(grid, (Coordinates){x, y});
            }
        }
    }

    grid->update_left_to_right = !grid->update_left_to_right;
}

void grid_render(Grid* grid, Display *display) {
    if (!grid || !display || !display->renderer || !display->texture) 
        return;

    if (!grid->dirty) {
        SDL_RenderTexture(display->renderer, display->texture, NULL, NULL);
        return;
    }

    int pitch;
    void *pixels;
    if (!SDL_LockTexture(display->texture, NULL, &pixels, &pitch)) {
        SDL_Log("Couldn't lock texture: %s", SDL_GetError());
        return;
    }

    for (int y = 0; y < GRID_HEIGHT; y++) {
        Uint32* row = (Uint32*)((Uint8*)pixels + y * pitch);
        for (int x = 0; x < GRID_WIDTH; x++) {
            SDL_Color color = grid->particles[y][x].color;
            Uint32 packed;
            memcpy(&packed, &color, sizeof(Uint32));
            row[x] = packed;
        }
    }

    SDL_UnlockTexture(display->texture);
    SDL_RenderTexture(display->renderer, display->texture, NULL, NULL);
}

bool grid_set_particle(Grid* grid, Coordinates coordinates, const Particle* particle) {
    if (!grid || !particle || !grid_is_in_bounds(coordinates)) 
        return false;

    grid->particles[coordinates.y][coordinates.x] = *particle;
    grid->dirty = true;
    return true;
}

bool grid_place_particle(Grid* grid, Coordinates coordinates, ParticleType type) {
    if (!grid || !grid_is_in_bounds(coordinates)) 
        return false;
    return grid_set_particle(grid, coordinates, &(Particle){.type = type, .color = particle_get_random_color_by_type(type), .update_gen = 0});
}

const Particle* grid_get_particle(Grid* grid, Coordinates coordinates) {
    if (!grid || !grid_is_in_bounds(coordinates)) 
        return NULL;
    return &grid->particles[coordinates.y][coordinates.x];
}

bool grid_is_particle_empty(Grid* grid, Coordinates coordinates) {
    if (!grid || !grid_is_in_bounds(coordinates)) 
        return false;
    return particle_is_empty(grid_get_particle(grid, coordinates));
}

bool grid_is_particle_solid(Grid* grid, Coordinates coordinates) {
    if (!grid || !grid_is_in_bounds(coordinates))
        return false;
    return particle_is_solid(grid_get_particle(grid, coordinates));
}

bool grid_is_in_bounds(Coordinates coordinates) {
    return (coordinates.x >= 0 && coordinates.x < GRID_WIDTH && coordinates.y >= 0 && coordinates.y < GRID_HEIGHT);
}

static int particle_type_priority(ParticleType type) {
    switch (type) {
        case SAND: return 1;
        case ROCK: return 2;
        default: return 0;
    }
}

static bool grid_can_place_particle(Grid* grid, Coordinates pos, ParticleType type) {
    if (!grid || !grid_is_in_bounds(pos))
        return false;

    ParticleType existing_particle_type = grid_get_particle(grid, pos)->type;
    if (type == existing_particle_type)
        return false;

    if (type == EMPTY)
        return true;
    
    return particle_type_priority(type) > particle_type_priority(existing_particle_type); 
}

void grid_apply_brush(Grid* grid, Coordinates center, int radius, ParticleType type) {
    if (!grid || !grid_is_in_bounds(center) || radius < 0)
        return;

    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
            if (dx * dx + dy * dy > radius * radius)
                continue;

            Coordinates pos = {center.x + dx, center.y + dy};
            if (grid_can_place_particle(grid, pos, type)) 
                grid_place_particle(grid, pos, type);
        }
    }
}