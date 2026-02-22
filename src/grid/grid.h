#ifndef FALLING_SAND_GRID_H
#define FALLING_SAND_GRID_H

#include <stdbool.h>
#include "../types.h"
#include "../config.h"
#include "../display/display.h"
#include "../particle/particle.h"

typedef struct grid {
    Particle particles[GRID_HEIGHT][GRID_WIDTH];
    bool update_left_to_right;
} Grid;

bool grid_clear(Grid *grid);
bool grid_cleanup(Grid* grid);
bool grid_initialize(Grid *grid);

void grid_update(Grid *grid);
void grid_render(Grid *grid, Display *display);

const Particle* grid_get_particle(Grid *grid, Coordinates coordinates);
bool grid_place_particle(Grid *grid, Coordinates coordinates, ParticleType type);
bool grid_set_particle(Grid *grid, Coordinates coordinates, const Particle *particle);

bool grid_is_empty(Grid *grid);
bool grid_is_in_bounds(Coordinates coordinates);
bool grid_is_particle_empty(Grid *grid, Coordinates coordinates);

#endif