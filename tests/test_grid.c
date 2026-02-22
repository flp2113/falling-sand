#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

/* ── Mock redirections ───────────────────────────────────────────────── */

#define SDL_Log                            fake_SDL_Log
#define SDL_SetRenderDrawColor             fake_SDL_SetRenderDrawColor
#define SDL_RenderFillRect                 fake_SDL_RenderFillRect
#define particle_update_in_grid            fake_particle_update_in_grid
#define particle_render                    fake_particle_render
#define particle_is_empty                  fake_particle_is_empty
#define particle_get_random_color_by_type  fake_particle_get_random_color_by_type

#include "../src/grid/grid.h"
#include "../src/grid/grid.c"

#undef SDL_Log
#undef SDL_SetRenderDrawColor
#undef SDL_RenderFillRect
#undef particle_update_in_grid
#undef particle_render
#undef particle_is_empty
#undef particle_get_random_color_by_type

/* ── Fake state ──────────────────────────────────────────────────────── */

typedef struct FakeState {
    int log_calls;
    int update_in_grid_calls;
    int render_particle_calls;
    int is_empty_calls;
    int random_color_calls;
    int set_color_calls;
    int fill_rect_calls;

    /* Last particle_is_empty result override */
    bool is_empty_return;

    /* Last particle_get_random_color_by_type capture */
    ParticleType last_random_color_type;
    SDL_Color    random_color_return;
} FakeState;

static FakeState fake_state;

static void reset_fake_state(void) {
    memset(&fake_state, 0, sizeof(fake_state));
    fake_state.is_empty_return = true;
    fake_state.random_color_return = (SDL_Color){100, 100, 100, 255};
}

/* ── Fake implementations ────────────────────────────────────────────── */

void fake_SDL_Log(const char *fmt, ...) {
    (void)fmt;
    va_list args;
    va_start(args, fmt);
    va_end(args);
    fake_state.log_calls++;
}

bool fake_SDL_SetRenderDrawColor(SDL_Renderer *renderer, Uint8 r, Uint8 g,
                                  Uint8 b, Uint8 a) {
    (void)renderer; (void)r; (void)g; (void)b; (void)a;
    fake_state.set_color_calls++;
    return true;
}

bool fake_SDL_RenderFillRect(SDL_Renderer *renderer, const SDL_FRect *rect) {
    (void)renderer; (void)rect;
    fake_state.fill_rect_calls++;
    return true;
}

void fake_particle_update_in_grid(Grid *grid, Coordinates coordinates) {
    (void)grid; (void)coordinates;
    fake_state.update_in_grid_calls++;
}

void fake_particle_render(Display *display, const Particle *particle,
                           Coordinates coordinates) {
    (void)display; (void)particle; (void)coordinates;
    fake_state.render_particle_calls++;
}

bool fake_particle_is_empty(const Particle *particle) {
    fake_state.is_empty_calls++;
    if (!particle) { return false; }
    return particle->type == EMPTY;
}

SDL_Color fake_particle_get_random_color_by_type(ParticleType type) {
    fake_state.random_color_calls++;
    fake_state.last_random_color_type = type;
    return fake_state.random_color_return;
}

/* ── Helper ──────────────────────────────────────────────────────────── */

static void fill_grid_with(Grid *grid, ParticleType type, SDL_Color color) {
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            grid->particles[y][x] = (Particle){.type = type, .color = color};
        }
    }
}

/* ────────────────────────────────────────────────────────────────────── */
/*  grid_initialize / grid_clear / grid_cleanup                          */
/* ────────────────────────────────────────────────────────────────────── */

static void test_initialize_null(void) {
    reset_fake_state();
    assert(!grid_initialize(NULL));
    assert(fake_state.log_calls == 1);
}

static void test_initialize_success(void) {
    static Grid grid;
    fill_grid_with(&grid, SAND, COLOR_SAND);
    reset_fake_state();

    assert(grid_initialize(&grid));
    assert(fake_state.log_calls == 0);

    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            assert(grid.particles[y][x].type == EMPTY);
        }
    }
}

static void test_clear_null(void) {
    reset_fake_state();
    assert(!grid_clear(NULL));
    assert(fake_state.log_calls == 1);
}

static void test_clear_success(void) {
    static Grid grid;
    fill_grid_with(&grid, ROCK, COLOR_ROCK);
    reset_fake_state();

    assert(grid_clear(&grid));

    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            assert(grid.particles[y][x].type == EMPTY);
        }
    }
}

static void test_cleanup_null(void) {
    reset_fake_state();
    assert(!grid_cleanup(NULL));
    assert(fake_state.log_calls == 1);
}

static void test_cleanup_clears(void) {
    static Grid grid;
    fill_grid_with(&grid, SAND, COLOR_SAND);
    reset_fake_state();

    assert(grid_cleanup(&grid));
    assert(grid.particles[0][0].type == EMPTY);
}

/* ────────────────────────────────────────────────────────────────────── */
/*  grid_is_in_bounds                                                    */
/* ────────────────────────────────────────────────────────────────────── */

static void test_in_bounds_corners(void) {
    assert(grid_is_in_bounds((Coordinates){0, 0}));
    assert(grid_is_in_bounds((Coordinates){GRID_WIDTH - 1, 0}));
    assert(grid_is_in_bounds((Coordinates){0, GRID_HEIGHT - 1}));
    assert(grid_is_in_bounds((Coordinates){GRID_WIDTH - 1, GRID_HEIGHT - 1}));
}

static void test_in_bounds_center(void) {
    assert(grid_is_in_bounds((Coordinates){GRID_WIDTH / 2, GRID_HEIGHT / 2}));
}

static void test_out_of_bounds_negative_x(void) {
    assert(!grid_is_in_bounds((Coordinates){-1, 0}));
}

static void test_out_of_bounds_negative_y(void) {
    assert(!grid_is_in_bounds((Coordinates){0, -1}));
}

static void test_out_of_bounds_overflow_x(void) {
    assert(!grid_is_in_bounds((Coordinates){GRID_WIDTH, 0}));
}

static void test_out_of_bounds_overflow_y(void) {
    assert(!grid_is_in_bounds((Coordinates){0, GRID_HEIGHT}));
}

static void test_out_of_bounds_both(void) {
    assert(!grid_is_in_bounds((Coordinates){GRID_WIDTH, GRID_HEIGHT}));
    assert(!grid_is_in_bounds((Coordinates){-1, -1}));
}

/* ────────────────────────────────────────────────────────────────────── */
/*  grid_is_empty                                                        */
/* ────────────────────────────────────────────────────────────────────── */

static void test_is_empty_null(void) {
    reset_fake_state();
    assert(!grid_is_empty(NULL));
    assert(fake_state.log_calls == 1);
}

static void test_is_empty_true(void) {
    static Grid grid;
    grid_initialize(&grid);
    reset_fake_state();

    assert(grid_is_empty(&grid));
}

static void test_is_empty_false_first_cell(void) {
    static Grid grid;
    grid_initialize(&grid);
    grid.particles[0][0].type = SAND;
    reset_fake_state();

    assert(!grid_is_empty(&grid));
}

static void test_is_empty_false_last_cell(void) {
    static Grid grid;
    grid_initialize(&grid);
    grid.particles[GRID_HEIGHT - 1][GRID_WIDTH - 1].type = ROCK;
    reset_fake_state();

    assert(!grid_is_empty(&grid));
}

/* ────────────────────────────────────────────────────────────────────── */
/*  grid_is_particle_empty                                               */
/* ────────────────────────────────────────────────────────────────────── */

static void test_particle_empty_null_grid(void) {
    reset_fake_state();
    assert(!grid_is_particle_empty(NULL, (Coordinates){0, 0}));
    assert(fake_state.log_calls == 1);
}

static void test_particle_empty_out_of_bounds(void) {
    static Grid grid;
    grid_initialize(&grid);
    reset_fake_state();

    assert(!grid_is_particle_empty(&grid, (Coordinates){-1, 0}));
    assert(!grid_is_particle_empty(&grid, (Coordinates){0, GRID_HEIGHT}));
}

static void test_particle_empty_true(void) {
    static Grid grid;
    grid_initialize(&grid);
    reset_fake_state();

    assert(grid_is_particle_empty(&grid, (Coordinates){5, 5}));
}

static void test_particle_empty_false(void) {
    static Grid grid;
    grid_initialize(&grid);
    grid.particles[5][5].type = SAND;
    reset_fake_state();

    assert(!grid_is_particle_empty(&grid, (Coordinates){5, 5}));
}

/* ────────────────────────────────────────────────────────────────────── */
/*  grid_set_particle                                                    */
/* ────────────────────────────────────────────────────────────────────── */

static void test_set_particle_null_grid(void) {
    Particle p = {.type = SAND, .color = COLOR_SAND};
    reset_fake_state();

    assert(!grid_set_particle(NULL, (Coordinates){0, 0}, &p));
    assert(fake_state.log_calls == 1);
}

static void test_set_particle_out_of_bounds(void) {
    static Grid grid;
    grid_initialize(&grid);
    Particle p = {.type = SAND, .color = COLOR_SAND};
    reset_fake_state();

    assert(!grid_set_particle(&grid, (Coordinates){GRID_WIDTH, 0}, &p));
    assert(fake_state.log_calls == 1);

    assert(!grid_set_particle(&grid, (Coordinates){0, -1}, &p));
    assert(fake_state.log_calls == 2);
}

static void test_set_particle_success(void) {
    static Grid grid;
    grid_initialize(&grid);
    Particle p = {.type = ROCK, .color = COLOR_ROCK};
    Coordinates pos = {10, 20};
    reset_fake_state();

    assert(grid_set_particle(&grid, pos, &p));
    assert(grid.particles[pos.y][pos.x].type == ROCK);
    assert(grid.particles[pos.y][pos.x].color.r == COLOR_ROCK.r);
    assert(grid.particles[pos.y][pos.x].color.g == COLOR_ROCK.g);
    assert(grid.particles[pos.y][pos.x].color.b == COLOR_ROCK.b);
    assert(fake_state.log_calls == 0);
}

static void test_set_particle_overwrite(void) {
    static Grid grid;
    grid_initialize(&grid);
    Particle sand = {.type = SAND, .color = COLOR_SAND};
    Particle rock = {.type = ROCK, .color = COLOR_ROCK};
    Coordinates pos = {0, 0};

    grid_set_particle(&grid, pos, &sand);
    assert(grid.particles[0][0].type == SAND);

    reset_fake_state();
    assert(grid_set_particle(&grid, pos, &rock));
    assert(grid.particles[0][0].type == ROCK);
}

static void test_set_particle_corners(void) {
    static Grid grid;
    grid_initialize(&grid);
    Particle p = {.type = SAND, .color = COLOR_SAND};
    reset_fake_state();

    assert(grid_set_particle(&grid, (Coordinates){0, 0}, &p));
    assert(grid_set_particle(&grid, (Coordinates){GRID_WIDTH - 1, 0}, &p));
    assert(grid_set_particle(&grid, (Coordinates){0, GRID_HEIGHT - 1}, &p));
    assert(grid_set_particle(&grid, (Coordinates){GRID_WIDTH - 1, GRID_HEIGHT - 1}, &p));
    assert(fake_state.log_calls == 0);
}

/* ────────────────────────────────────────────────────────────────────── */
/*  grid_place_particle                                                  */
/* ────────────────────────────────────────────────────────────────────── */

static void test_place_particle_null_grid(void) {
    reset_fake_state();
    assert(!grid_place_particle(NULL, (Coordinates){0, 0}, SAND));
    assert(fake_state.log_calls == 1);
}

static void test_place_particle_out_of_bounds(void) {
    static Grid grid;
    grid_initialize(&grid);
    reset_fake_state();

    assert(!grid_place_particle(&grid, (Coordinates){GRID_WIDTH, 0}, SAND));
    assert(fake_state.log_calls == 1);
}

static void test_place_particle_sand(void) {
    static Grid grid;
    grid_initialize(&grid);
    Coordinates pos = {10, 20};
    reset_fake_state();

    assert(grid_place_particle(&grid, pos, SAND));
    assert(grid.particles[pos.y][pos.x].type == SAND);
    assert(fake_state.random_color_calls == 1);
    assert(fake_state.last_random_color_type == SAND);
    assert(fake_state.log_calls == 0);
}

static void test_place_particle_rock(void) {
    static Grid grid;
    grid_initialize(&grid);
    Coordinates pos = {50, 100};
    reset_fake_state();

    assert(grid_place_particle(&grid, pos, ROCK));
    assert(grid.particles[pos.y][pos.x].type == ROCK);
    assert(fake_state.random_color_calls == 1);
    assert(fake_state.last_random_color_type == ROCK);
}

static void test_place_particle_empty(void) {
    static Grid grid;
    grid_initialize(&grid);
    grid.particles[0][0].type = SAND;
    reset_fake_state();

    assert(grid_place_particle(&grid, (Coordinates){0, 0}, EMPTY));
    assert(grid.particles[0][0].type == EMPTY);
    assert(fake_state.random_color_calls == 1);
    assert(fake_state.last_random_color_type == EMPTY);
}

static void test_place_particle_uses_random_color(void) {
    static Grid grid;
    grid_initialize(&grid);
    Coordinates pos = {5, 5};
    reset_fake_state();
    fake_state.random_color_return = (SDL_Color){11, 22, 33, 44};

    assert(grid_place_particle(&grid, pos, SAND));
    assert(grid.particles[pos.y][pos.x].color.r == 11);
    assert(grid.particles[pos.y][pos.x].color.g == 22);
    assert(grid.particles[pos.y][pos.x].color.b == 33);
    assert(grid.particles[pos.y][pos.x].color.a == 44);
}

/* ────────────────────────────────────────────────────────────────────── */
/*  grid_get_particle                                                    */
/* ────────────────────────────────────────────────────────────────────── */

static void test_get_particle_null_grid(void) {
    reset_fake_state();
    assert(grid_get_particle(NULL, (Coordinates){0, 0}) == NULL);
    assert(fake_state.log_calls == 1);
}

static void test_get_particle_out_of_bounds(void) {
    static Grid grid;
    grid_initialize(&grid);
    reset_fake_state();

    assert(grid_get_particle(&grid, (Coordinates){-1, 0}) == NULL);
    assert(fake_state.log_calls == 1);
}

static void test_get_particle_success(void) {
    static Grid grid;
    grid_initialize(&grid);
    Coordinates pos = {10, 20};
    grid.particles[pos.y][pos.x].type = SAND;
    reset_fake_state();

    const Particle *p = grid_get_particle(&grid, pos);
    assert(p != NULL);
    assert(p->type == SAND);
    assert(p == &grid.particles[pos.y][pos.x]);
    assert(fake_state.log_calls == 0);
}

static void test_get_particle_returns_address(void) {
    static Grid grid;
    grid_initialize(&grid);
    Coordinates pos = {0, 0};

    const Particle *p = grid_get_particle(&grid, pos);
    assert(p == &grid.particles[0][0]);
}

/* ────────────────────────────────────────────────────────────────────── */
/*  grid_update                                                          */
/* ────────────────────────────────────────────────────────────────────── */

static void test_update_null(void) {
    reset_fake_state();
    grid_update(NULL);
    assert(fake_state.log_calls == 1);
    assert(fake_state.update_in_grid_calls == 0);
}

static void test_update_calls_particle_update(void) {
    static Grid grid;
    grid_initialize(&grid);
    reset_fake_state();

    grid_update(&grid);

    /* Every cell must be visited once per update */
    assert(fake_state.update_in_grid_calls == GRID_WIDTH * GRID_HEIGHT);
}

static void test_update_alternates_direction(void) {
    static Grid grid;
    grid_initialize(&grid);
    reset_fake_state();

    /* Two consecutive updates should both visit all cells */
    grid_update(&grid);
    assert(fake_state.update_in_grid_calls == GRID_WIDTH * GRID_HEIGHT);

    fake_state.update_in_grid_calls = 0;
    grid_update(&grid);
    assert(fake_state.update_in_grid_calls == GRID_WIDTH * GRID_HEIGHT);
}

/* ────────────────────────────────────────────────────────────────────── */
/*  grid_render                                                          */
/* ────────────────────────────────────────────────────────────────────── */

static void test_render_null_grid(void) {
    Display display = {.renderer = (SDL_Renderer *)0x1};
    reset_fake_state();

    grid_render(NULL, &display);
    assert(fake_state.log_calls == 1);
    assert(fake_state.render_particle_calls == 0);
}

static void test_render_null_display(void) {
    static Grid grid;
    grid_initialize(&grid);
    reset_fake_state();

    grid_render(&grid, NULL);
    assert(fake_state.log_calls == 1);
    assert(fake_state.render_particle_calls == 0);
}

static void test_render_null_renderer(void) {
    static Grid grid;
    grid_initialize(&grid);
    Display display = {0};
    reset_fake_state();

    grid_render(&grid, &display);
    assert(fake_state.log_calls == 1);
    assert(fake_state.render_particle_calls == 0);
}

static void test_render_empty_grid(void) {
    static Grid grid;
    grid_initialize(&grid);
    Display display = {.renderer = (SDL_Renderer *)0x1};
    reset_fake_state();

    grid_render(&grid, &display);
    assert(fake_state.render_particle_calls == 0);
    assert(fake_state.log_calls == 0);
}

static void test_render_single_particle(void) {
    static Grid grid;
    grid_initialize(&grid);
    grid.particles[0][0] = (Particle){.type = SAND, .color = COLOR_SAND};
    Display display = {.renderer = (SDL_Renderer *)0x1};
    reset_fake_state();

    grid_render(&grid, &display);
    assert(fake_state.render_particle_calls == 1);
}

static void test_render_multiple_particles(void) {
    static Grid grid;
    grid_initialize(&grid);
    grid.particles[0][0]   = (Particle){.type = SAND, .color = COLOR_SAND};
    grid.particles[5][10]  = (Particle){.type = ROCK, .color = COLOR_ROCK};
    grid.particles[100][50] = (Particle){.type = SAND, .color = COLOR_SAND};
    Display display = {.renderer = (SDL_Renderer *)0x1};
    reset_fake_state();

    grid_render(&grid, &display);
    assert(fake_state.render_particle_calls == 3);
}

static void test_render_full_grid(void) {
    static Grid grid;
    fill_grid_with(&grid, SAND, COLOR_SAND);
    Display display = {.renderer = (SDL_Renderer *)0x1};
    reset_fake_state();

    grid_render(&grid, &display);
    assert(fake_state.render_particle_calls == GRID_WIDTH * GRID_HEIGHT);
}

/* ────────────────────────────────────────────────────────────────────── */
/*  Integration: place → get round-trip                                  */
/* ────────────────────────────────────────────────────────────────────── */

static void test_place_then_get(void) {
    static Grid grid;
    grid_initialize(&grid);
    Coordinates pos = {42, 77};
    reset_fake_state();
    fake_state.random_color_return = (SDL_Color){1, 2, 3, 4};

    assert(grid_place_particle(&grid, pos, ROCK));
    const Particle *p = grid_get_particle(&grid, pos);
    assert(p != NULL);
    assert(p->type == ROCK);
    assert(p->color.r == 1);
    assert(p->color.g == 2);
    assert(p->color.b == 3);
    assert(p->color.a == 4);
}

static void test_clear_after_fill(void) {
    static Grid grid;
    fill_grid_with(&grid, ROCK, COLOR_ROCK);
    assert(!grid_is_empty(&grid));

    grid_clear(&grid);
    assert(grid_is_empty(&grid));
}

/* ── Runner ──────────────────────────────────────────────────────────── */

int main(void) {
    /* Initialize / Clear / Cleanup */
    test_initialize_null();
    test_initialize_success();
    test_clear_null();
    test_clear_success();
    test_cleanup_null();
    test_cleanup_clears();

    /* Bounds checking */
    test_in_bounds_corners();
    test_in_bounds_center();
    test_out_of_bounds_negative_x();
    test_out_of_bounds_negative_y();
    test_out_of_bounds_overflow_x();
    test_out_of_bounds_overflow_y();
    test_out_of_bounds_both();

    /* Grid empty */
    test_is_empty_null();
    test_is_empty_true();
    test_is_empty_false_first_cell();
    test_is_empty_false_last_cell();

    /* Particle empty in grid */
    test_particle_empty_null_grid();
    test_particle_empty_out_of_bounds();
    test_particle_empty_true();
    test_particle_empty_false();

    /* Set particle */
    test_set_particle_null_grid();
    test_set_particle_out_of_bounds();
    test_set_particle_success();
    test_set_particle_overwrite();
    test_set_particle_corners();

    /* Place particle */
    test_place_particle_null_grid();
    test_place_particle_out_of_bounds();
    test_place_particle_sand();
    test_place_particle_rock();
    test_place_particle_empty();
    test_place_particle_uses_random_color();

    /* Get particle */
    test_get_particle_null_grid();
    test_get_particle_out_of_bounds();
    test_get_particle_success();
    test_get_particle_returns_address();

    /* Update */
    test_update_null();
    test_update_calls_particle_update();
    test_update_alternates_direction();

    /* Render */
    test_render_null_grid();
    test_render_null_display();
    test_render_null_renderer();
    test_render_empty_grid();
    test_render_single_particle();
    test_render_multiple_particles();
    test_render_full_grid();

    /* Integration */
    test_place_then_get();
    test_clear_after_fill();

    return 0;
}
