#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

/* ── Mock redirections ───────────────────────────────────────────────── */

#define SDL_rand                    fake_SDL_rand
#define SDL_Log                     fake_SDL_Log

#include "particle/particle.h"
#include "particle/particle.c"

#undef SDL_rand
#undef SDL_Log

/* ── Fake state ──────────────────────────────────────────────────────── */

#define RAND_QUEUE_SIZE 64

typedef struct FakeState {
    int log_calls;
    int rand_calls;

    /* Controllable SDL_rand return queue */
    Sint32 rand_queue[RAND_QUEUE_SIZE];
    int    rand_queue_len;
    int    rand_queue_pos;
} FakeState;

static FakeState fake_state;

static void reset_fake_state(void) {
    memset(&fake_state, 0, sizeof(fake_state));
}

/* Push a sequence of values that fake_SDL_rand will return. */
static void push_rand_values(const Sint32 *values, int count) {
    assert(count <= RAND_QUEUE_SIZE);
    memcpy(fake_state.rand_queue, values, (size_t)count * sizeof(Sint32));
    fake_state.rand_queue_len = count;
    fake_state.rand_queue_pos = 0;
}

/* ── Fake implementations ────────────────────────────────────────────── */

void fake_SDL_Log(const char *fmt, ...) {
    (void)fmt;
    va_list args;
    va_start(args, fmt);
    va_end(args);
    fake_state.log_calls++;
}

Sint32 fake_SDL_rand(Sint32 n) {
    fake_state.rand_calls++;
    if (fake_state.rand_queue_pos < fake_state.rand_queue_len) {
        Sint32 val = fake_state.rand_queue[fake_state.rand_queue_pos++];
        /* Clamp to valid range [0, n) just like real SDL_rand */
        if (n > 0) { val = val % n; }
        return val;
    }
    /* Default: return centre of range */
    return n > 0 ? n / 2 : 0;
}

/*
 * Grid fakes: use faithful logic so physics tests run against real data.
 * This is intentional — these are trivial pure functions, and providing
 * real logic makes particle physics tests far more meaningful.
 */

/* ────────────────────────────────────────────────────────────────────── */
/*  clamp_color_component (static, accessed via #include particle.c)     */
/* ────────────────────────────────────────────────────────────────────── */

static void test_clamp_within_range(void) {
    assert(clamp_color_component(0)   == 0);
    assert(clamp_color_component(128) == 128);
    assert(clamp_color_component(255) == 255);
}

static void test_clamp_negative(void) {
    assert(clamp_color_component(-1)   == 0);
    assert(clamp_color_component(-100) == 0);
}

static void test_clamp_overflow(void) {
    assert(clamp_color_component(256) == 255);
    assert(clamp_color_component(500) == 255);
}

/* ────────────────────────────────────────────────────────────────────── */
/*  particle_get_random_sand_color / rock_color                          */
/* ────────────────────────────────────────────────────────────────────── */

static void test_random_sand_color_center(void) {
    reset_fake_state();
    /* SDL_rand called 3 times (r, g, b).
     * Range = SAND_COLOR_VARIATION * 2 + 1 = 25
     * Returning SAND_COLOR_VARIATION (=12) gives offset 0 */
    Sint32 vals[] = {SAND_COLOR_VARIATION, SAND_COLOR_VARIATION,
                     SAND_COLOR_VARIATION};
    push_rand_values(vals, 3);

    SDL_Color c = particle_get_random_color_by_type(SAND);
    assert(c.r == SAND_COLOR_BASE_R);
    assert(c.g == SAND_COLOR_BASE_G);
    assert(c.b == SAND_COLOR_BASE_B);
    assert(c.a == SAND_COLOR_BASE_A);
    assert(fake_state.rand_calls == 3);
}

static void test_random_sand_color_max_positive(void) {
    reset_fake_state();
    /* Returning (VARIATION * 2) → offset = +VARIATION */
    Sint32 vals[] = {SAND_COLOR_VARIATION * 2, SAND_COLOR_VARIATION * 2,
                     SAND_COLOR_VARIATION * 2};
    push_rand_values(vals, 3);

    SDL_Color c = particle_get_random_color_by_type(SAND);
    assert(c.r == clamp_color_component(SAND_COLOR_BASE_R + SAND_COLOR_VARIATION));
    assert(c.g == clamp_color_component(SAND_COLOR_BASE_G + SAND_COLOR_VARIATION));
    assert(c.b == clamp_color_component(SAND_COLOR_BASE_B + SAND_COLOR_VARIATION));
}

static void test_random_sand_color_max_negative(void) {
    reset_fake_state();
    /* Returning 0 → offset = -VARIATION */
    Sint32 vals[] = {0, 0, 0};
    push_rand_values(vals, 3);

    SDL_Color c = particle_get_random_color_by_type(SAND);
    assert(c.r == clamp_color_component(SAND_COLOR_BASE_R - SAND_COLOR_VARIATION));
    assert(c.g == clamp_color_component(SAND_COLOR_BASE_G - SAND_COLOR_VARIATION));
    assert(c.b == clamp_color_component(SAND_COLOR_BASE_B - SAND_COLOR_VARIATION));
}

static void test_random_rock_color_center(void) {
    reset_fake_state();
    Sint32 vals[] = {ROCK_COLOR_VARIATION, ROCK_COLOR_VARIATION,
                     ROCK_COLOR_VARIATION};
    push_rand_values(vals, 3);

    SDL_Color c = particle_get_random_color_by_type(ROCK);
    assert(c.r == ROCK_COLOR_BASE_R);
    assert(c.g == ROCK_COLOR_BASE_G);
    assert(c.b == ROCK_COLOR_BASE_B);
    assert(c.a == ROCK_COLOR_BASE_A);
}

/* ────────────────────────────────────────────────────────────────────── */
/*  particle_get_default_color_by_type                                   */
/* ────────────────────────────────────────────────────────────────────── */

static void test_default_color_empty(void) {
    SDL_Color c = particle_get_default_color_by_type(EMPTY);
    assert(c.r == EMPTY_COLOR_BASE_R);
    assert(c.g == EMPTY_COLOR_BASE_G);
    assert(c.b == EMPTY_COLOR_BASE_B);
    assert(c.a == EMPTY_COLOR_BASE_A);
}

static void test_default_color_sand(void) {
    SDL_Color c = particle_get_default_color_by_type(SAND);
    assert(c.r == SAND_COLOR_BASE_R);
    assert(c.g == SAND_COLOR_BASE_G);
    assert(c.b == SAND_COLOR_BASE_B);
}

static void test_default_color_rock(void) {
    SDL_Color c = particle_get_default_color_by_type(ROCK);
    assert(c.r == ROCK_COLOR_BASE_R);
    assert(c.g == ROCK_COLOR_BASE_G);
    assert(c.b == ROCK_COLOR_BASE_B);
}

/* ────────────────────────────────────────────────────────────────────── */
/*  particle_get_random_color_by_type                                    */
/* ────────────────────────────────────────────────────────────────────── */

static void test_random_color_dispatches_sand(void) {
    reset_fake_state();
    Sint32 vals[] = {SAND_COLOR_VARIATION, SAND_COLOR_VARIATION,
                     SAND_COLOR_VARIATION};
    push_rand_values(vals, 3);

    SDL_Color c = particle_get_random_color_by_type(SAND);
    assert(c.r == SAND_COLOR_BASE_R);
    assert(c.g == SAND_COLOR_BASE_G);
    assert(fake_state.rand_calls == 3);
}

static void test_random_color_dispatches_rock(void) {
    reset_fake_state();
    Sint32 vals[] = {ROCK_COLOR_VARIATION, ROCK_COLOR_VARIATION,
                     ROCK_COLOR_VARIATION};
    push_rand_values(vals, 3);

    SDL_Color c = particle_get_random_color_by_type(ROCK);
    assert(c.r == ROCK_COLOR_BASE_R);
    assert(c.g == ROCK_COLOR_BASE_G);
    assert(fake_state.rand_calls == 3);
}

static void test_random_color_empty_no_rand(void) {
    reset_fake_state();
    SDL_Color c = particle_get_random_color_by_type(EMPTY);
    assert(c.r == EMPTY_COLOR_BASE_R);
    assert(fake_state.rand_calls == 0);
}

/* ────────────────────────────────────────────────────────────────────── */
/*  particle_is_empty                                                    */
/* ────────────────────────────────────────────────────────────────────── */

static void test_is_empty_null(void) {
    reset_fake_state();
    assert(!particle_is_empty(NULL));
}

static void test_is_empty_true(void) {
    Particle p = {.type = EMPTY};
    assert(particle_is_empty(&p));
}

static void test_is_empty_false_sand(void) {
    Particle p = {.type = SAND};
    assert(!particle_is_empty(&p));
}

static void test_is_empty_false_rock(void) {
    Particle p = {.type = ROCK};
    assert(!particle_is_empty(&p));
}

/* ────────────────────────────────────────────────────────────────────── */
/*  particle_is_solid / particle_is_type_solid                           */
/* ────────────────────────────────────────────────────────────────────── */

static void test_is_solid_null(void) {
    assert(!particle_is_solid(NULL));
}

static void test_is_solid_rock(void) {
    Particle p = {.type = ROCK};
    assert(particle_is_solid(&p));
}

static void test_is_solid_sand(void) {
    Particle p = {.type = SAND};
    assert(!particle_is_solid(&p));
}

static void test_is_solid_empty(void) {
    Particle p = {.type = EMPTY};
    assert(!particle_is_solid(&p));
}

static void test_is_type_solid_rock(void) {
    assert(particle_is_type_solid(ROCK));
}

static void test_is_type_solid_sand(void) {
    assert(!particle_is_type_solid(SAND));
}

static void test_is_type_solid_empty(void) {
    assert(!particle_is_type_solid(EMPTY));
}

/* ── Runner ──────────────────────────────────────────────────────────── */

int main(void) {
    /* Clamp */
    test_clamp_within_range();
    test_clamp_negative();
    test_clamp_overflow();

    /* Random colour generation */
    test_random_sand_color_center();
    test_random_sand_color_max_positive();
    test_random_sand_color_max_negative();
    test_random_rock_color_center();

    /* Default colours */
    test_default_color_empty();
    test_default_color_sand();
    test_default_color_rock();

    /* Random colour dispatch */
    test_random_color_dispatches_sand();
    test_random_color_dispatches_rock();
    test_random_color_empty_no_rand();

    /* particle_is_empty */
    test_is_empty_null();
    test_is_empty_true();
    test_is_empty_false_sand();
    test_is_empty_false_rock();

    /* particle_is_solid / particle_is_type_solid */
    test_is_solid_null();
    test_is_solid_rock();
    test_is_solid_sand();
    test_is_solid_empty();
    test_is_type_solid_rock();
    test_is_type_solid_sand();
    test_is_type_solid_empty();

    return 0;
}
