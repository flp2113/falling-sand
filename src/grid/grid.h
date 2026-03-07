#ifndef FALLING_SAND_GRID_H
#define FALLING_SAND_GRID_H

#include <stdbool.h>

#include "config/simulation_config.h"
#include "display/display.h"
#include "particle/particle.h"
#include "types.h"

typedef struct grid {
    Particle particles[GRID_HEIGHT][GRID_WIDTH];
    bool update_left_to_right;
    bool dirty;
    Uint8 current_gen;
} Grid;

bool grid_reset(Grid* grid);
bool grid_initialize(Grid *grid);

void grid_update(Grid* grid);

void grid_render(Grid* grid, Display* display);

void grid_swap(Grid* grid, Coordinates source, Coordinates destination);

const Particle* grid_get_particle(Grid* grid, Coordinates coordinates);
bool grid_place_particle(Grid* grid, Coordinates coordinates, ParticleType type);
bool grid_set_particle(Grid* grid, Coordinates coordinates, const Particle* particle);
void grid_apply_brush(Grid* grid, Coordinates center, int radius, ParticleType type);

bool grid_is_in_bounds(Coordinates coordinates);
bool grid_is_particle_empty(Grid* grid, Coordinates coordinates);
bool grid_is_particle_solid(Grid* grid, Coordinates coordinates);

#endif