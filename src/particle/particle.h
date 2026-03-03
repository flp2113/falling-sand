#ifndef FALLING_SAND_PARTICLE_H
#define FALLING_SAND_PARTICLE_H

#include <SDL3/SDL.h>
#include <stdbool.h>
#include "../types.h"

typedef struct grid Grid;
typedef struct display Display;
typedef enum particle_type {
    EMPTY, ROCK, SAND
} ParticleType;
typedef struct particle {
    ParticleType type;
    SDL_Color color;
} Particle;

void particle_update_in_grid(Grid *grid, Coordinates coordinates);

SDL_Color particle_get_default_color_by_type(ParticleType type);
SDL_Color particle_get_random_color_by_type(ParticleType type);

bool particle_is_empty(const Particle *particle);
bool particle_is_solid(const Particle *particle);
bool particle_is_type_solid(ParticleType type);

#endif