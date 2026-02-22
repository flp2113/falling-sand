#ifndef FALLING_SAND_PARTICLE_H
#define FALLING_SAND_PARTICLE_H

#include <SDL3/SDL.h>
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

SDL_Color particle_get_random_sand_color(void);
SDL_Color particle_get_random_rock_color(void);
SDL_Color particle_get_default_color_by_type(ParticleType type);
SDL_Color particle_get_random_color_by_type(ParticleType type);

bool particle_is_empty(const Particle *particle);

void particle_update_in_grid(Grid *grid, Coordinates coordinates);
void particle_render(Display *display, const Particle *particle, Coordinates coordinates);

#endif