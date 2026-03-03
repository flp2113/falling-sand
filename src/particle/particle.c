#include "particle.h"
#include "../config.h"
#include "../grid/grid.h"

static void particle_swap_in_grid(Grid *grid, Coordinates source, Coordinates destination) {
    Particle temporary_particle = grid->particles[source.y][source.x];
    grid->particles[source.y][source.x] = grid->particles[destination.y][destination.x];
    grid->particles[destination.y][destination.x] = temporary_particle;
}

bool particle_is_type_solid(ParticleType type) {
    return type == ROCK;
}

bool particle_is_solid(const Particle *particle) {
    if (!particle)
        return false;
    return particle_is_type_solid(particle->type);
}

static void particle_update_sand(Grid *grid, Coordinates coordinates) {
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
    bool can_go_below_left = is_below_left_empty && !grid_particle_is_solid(grid, left);
    bool can_go_below_right = is_below_right_empty && !grid_particle_is_solid(grid, right);

    if (is_below_empty) {
        particle_swap_in_grid(grid, coordinates, below);
        return;
    }

    if (!can_go_below_left && !can_go_below_right) {
        return;
    }

    if (can_go_below_left && can_go_below_right) {
        bool go_left = SDL_rand(2);
        particle_swap_in_grid(grid, coordinates, go_left ? below_left : below_right);
        return;
    }

    if (can_go_below_left) {
        particle_swap_in_grid(grid, coordinates, below_left);
        return;
    }

    if (can_go_below_right) {
        particle_swap_in_grid(grid, coordinates, below_right);
        return;
    }
}

void particle_update_in_grid(Grid *grid, Coordinates coordinates) {
    if (!grid || !grid_is_in_bounds(coordinates))
        return;

    Particle *particle = &grid->particles[coordinates.y][coordinates.x];
    switch (particle->type) {
        case SAND: particle_update_sand(grid, coordinates); break;
        default: break;
    }
}

static Uint8 clamp_color_component(int value) {
    if (value < 0)
        return 0;

    if (value > 255)
        return 255;

    return (Uint8)value;
}

SDL_Color particle_get_random_color_with_variation(SDL_Color color_base, int variation) {
    if (variation == 0)
        return color_base;

    int range_variation = variation * 2 + 1;
    return (SDL_Color) {
        .r = clamp_color_component(color_base.r + SDL_rand(range_variation) - variation),
        .g = clamp_color_component(color_base.g + SDL_rand(range_variation) - variation),
        .b = clamp_color_component(color_base.b + SDL_rand(range_variation) - variation),
        .a = color_base.a
    };
}

SDL_Color particle_get_default_color_by_type(ParticleType type) {
    switch (type) {
        case ROCK: return ROCK_COLOR;
        case SAND: return SAND_COLOR;
        default: return EMPTY_COLOR;
    }
}

SDL_Color particle_get_random_color_by_type(ParticleType type) {
    switch (type) {
        case ROCK: return particle_get_random_color_with_variation(ROCK_COLOR, SAND_COLOR_VARIATION);
        case SAND: return particle_get_random_color_with_variation(SAND_COLOR, ROCK_COLOR_VARIATION);
        default: return EMPTY_COLOR;
    }
}

bool particle_is_empty(const Particle *particle) {
    if (!particle)
        return false;
    return particle->type == EMPTY;
}