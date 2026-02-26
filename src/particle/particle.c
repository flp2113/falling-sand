#include "particle.h"
#include "../config.h"
#include "../grid/grid.h"

static void particle_swap_in_grid(Grid *grid, Coordinates source, Coordinates destination) {
    Particle temporary_particle = grid->particles[source.y][source.x];
    grid->particles[source.y][source.x] = grid->particles[destination.y][destination.x];
    grid->particles[destination.y][destination.x] = temporary_particle;
}

static bool is_particle_solid_at(Grid *grid, Coordinates coordinates) {
    if (!grid || !grid_is_in_bounds(coordinates))
        return true;
    return grid->particles[coordinates.y][coordinates.x].type == ROCK;
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
    bool can_go_below_left = is_below_left_empty && !is_particle_solid_at(grid, left);
    bool can_go_below_right = is_below_right_empty && !is_particle_solid_at(grid, right);

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
    case SAND:
        particle_update_sand(grid, coordinates);
        break;
    default:
        break;
    }
}

static Uint8 clamp_color_component(int value) {
    if (value < 0)
        return 0;

    if (value > 255)
        return 255;

    return (Uint8)value;
}

SDL_Color particle_get_random_sand_color(void) {
    int offset_r = (SDL_rand(SAND_COLOR_VARIATION * 2 + 1)) - SAND_COLOR_VARIATION;
    int offset_g = (SDL_rand(SAND_COLOR_VARIATION * 2 + 1)) - SAND_COLOR_VARIATION;
    int offset_b = (SDL_rand(SAND_COLOR_VARIATION * 2 + 1)) - SAND_COLOR_VARIATION;

    SDL_Color color = {.r = clamp_color_component(SAND_COLOR_BASE_R + offset_r),
                       .g = clamp_color_component(SAND_COLOR_BASE_G + offset_g),
                       .b = clamp_color_component(SAND_COLOR_BASE_B + offset_b),
                       .a = SAND_COLOR_BASE_A};

    return color;
}

SDL_Color particle_get_random_rock_color(void) {
    int offset_r = (SDL_rand(ROCK_COLOR_VARIATION * 2 + 1)) - ROCK_COLOR_VARIATION;
    int offset_g = (SDL_rand(ROCK_COLOR_VARIATION * 2 + 1)) - ROCK_COLOR_VARIATION;
    int offset_b = (SDL_rand(ROCK_COLOR_VARIATION * 2 + 1)) - ROCK_COLOR_VARIATION;

    SDL_Color color = {.r = clamp_color_component(ROCK_COLOR_BASE_R + offset_r),
                       .g = clamp_color_component(ROCK_COLOR_BASE_G + offset_g),
                       .b = clamp_color_component(ROCK_COLOR_BASE_B + offset_b),
                       .a = ROCK_COLOR_BASE_A};

    return color;
}

SDL_Color particle_get_default_color_by_type(ParticleType type) {
    switch (type) {
        case ROCK: return COLOR_ROCK;
        case SAND: return COLOR_SAND;
        default: return COLOR_EMPTY;
    }
}

SDL_Color particle_get_random_color_by_type(ParticleType type) {
    switch (type) {
        case ROCK: return particle_get_random_rock_color();
        case SAND: return particle_get_random_sand_color();
        default: return COLOR_EMPTY;
    }
}

bool particle_is_empty(const Particle *particle) {
    if (!particle)
        return false;

    return particle->type == EMPTY;
}