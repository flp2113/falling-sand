#ifndef PARTICLE_H
#define PARTICLE_H

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdlib.h>

typedef enum particle_type {
    EMPTY, ROCK, SAND
} ParticleType;

typedef struct particle {
    ParticleType type;
    SDL_FRect rect;
    SDL_Color color;
} Particle;

#endif