#include "config/color_config.h"
#include "particle/particle.h"

bool particle_is_type_solid(ParticleType type) {
    return type == ROCK;
}

bool particle_is_solid(const Particle* particle) {
    if (!particle)
        return false;
    return particle_is_type_solid(particle->type);
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
        case ROCK: return ROCK_BASE_COLOR;
        case SAND: return SAND_BASE_COLOR;
        default: return EMPTY_BASE_COLOR;
    }
}

SDL_Color particle_get_random_color_by_type(ParticleType type) {
    switch (type) {
        case ROCK: return particle_get_random_color_with_variation(ROCK_BASE_COLOR, ROCK_COLOR_VARIATION);
        case SAND: return particle_get_random_color_with_variation(SAND_BASE_COLOR, SAND_COLOR_VARIATION);
        default: return EMPTY_BASE_COLOR;
    }
}

bool particle_is_empty(const Particle* particle) {
    if (!particle)
        return false;
    return particle->type == EMPTY;
}