#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

/* ── Mock redirections ───────────────────────────────────────────────── */

#define SDL_Log                            fake_SDL_Log
#define SDL_LockTexture                    fake_SDL_LockTexture
#define SDL_UnlockTexture                  fake_SDL_UnlockTexture
#define SDL_RenderTexture                  fake_SDL_RenderTexture
#define SDL_GetError                       fake_SDL_GetError
#define particle_is_empty                  fake_particle_is_empty
#define particle_is_solid                  fake_particle_is_solid
#define particle_get_random_color_by_type  fake_particle_get_random_color_by_type

#include "grid/grid.h"
#include "grid/grid.c"

#undef SDL_Log
#undef SDL_LockTexture
#undef SDL_UnlockTexture
#undef SDL_RenderTexture
#undef SDL_GetError
#undef particle_is_empty
#undef particle_is_solid
#undef particle_get_random_color_by_type

/* ── Fake state ──────────────────────────────────────────────────────── */

typedef struct FakeState {
    int log_calls;
    int is_empty_calls;
    int random_color_calls;
    int lock_calls;
    int unlock_calls;
    int render_texture_calls;

    /* Last particle_is_empty result override */
    bool is_empty_return;

    /* Last particle_get_random_color_by_type capture */
    ParticleType last_random_color_type;
    SDL_Color    random_color_return;

    /* Texture lock behavior */
    bool lock_return;
    SDL_Texture *last_locked_texture;
    SDL_Texture *last_rendered_texture;
    SDL_Renderer *last_rendered_renderer;
} FakeState;

static FakeState fake_state;

static void reset_fake_state(void) {
    memset(&fake_state, 0, sizeof(fake_state));
    fake_state.is_empty_return = true;
    fake_state.random_color_return = (SDL_Color){100, 100, 100, 255};
    fake_state.lock_return = true;
}

static Uint8 fake_pixels[GRID_HEIGHT][GRID_WIDTH * 4];

/* ── Fake implementations ────────────────────────────────────────────── */

void fake_SDL_Log(const char *fmt, ...) {
    (void)fmt;
    va_list args;
    va_start(args, fmt);
    va_end(args);
    fake_state.log_calls++;
}

bool fake_SDL_LockTexture(SDL_Texture *texture, const SDL_Rect *rect,
                          void **pixels, int *pitch) {
    (void)rect;
    fake_state.lock_calls++;
    fake_state.last_locked_texture = texture;
    if (!fake_state.lock_return) {
        return false;
    }
    if (pixels) {
        *pixels = fake_pixels;
    }
    if (pitch) {
        *pitch = GRID_WIDTH * 4;
    }
    return true;
}

void fake_SDL_UnlockTexture(SDL_Texture *texture) {
    (void)texture;
    fake_state.unlock_calls++;
}

bool fake_SDL_RenderTexture(SDL_Renderer *renderer, SDL_Texture *texture,
                            const SDL_FRect *srcrect, const SDL_FRect *dstrect) {
    (void)srcrect;
    (void)dstrect;
    fake_state.render_texture_calls++;
    fake_state.last_rendered_renderer = renderer;
    fake_state.last_rendered_texture = texture;
    return true;
}

bool fake_particle_is_empty(const Particle *particle) {
    fake_state.is_empty_calls++;
    if (!particle) { return false; }
    return particle->type == EMPTY;
}

bool fake_particle_is_solid(const Particle *particle) {
    if (!particle) { return false; }
    return particle->type == ROCK;
}

SDL_Color fake_particle_get_random_color_by_type(ParticleType type) {
    fake_state.random_color_calls++;
    fake_state.last_random_color_type = type;
    return fake_state.random_color_return;
}

const char *fake_SDL_GetError(void) { return "fake sdl error"; }

/* ── Helper ──────────────────────────────────────────────────────────── */

static void fill_grid_with(Grid *grid, ParticleType type, SDL_Color color) {
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            grid->particles[y][x] = (Particle){.type = type, .color = color};
        }
    }
}

static SDL_Color get_pixel(int x, int y) {
    Uint8 *pixel = &fake_pixels[y][x * 4];
    SDL_Color color = {pixel[0], pixel[1], pixel[2], pixel[3]};
    return color;
}

/* Local helper — grid_is_empty was removed from the public API */
static bool test_helper_grid_is_empty(Grid *grid) {
    if (!grid) return false;
    for (int y = 0; y < GRID_HEIGHT; y++)
        for (int x = 0; x < GRID_WIDTH; x++)
            if (grid->particles[y][x].type != EMPTY) return false;
    return true;
}

/* ────────────────────────────────────────────────────────────────────── */
/*  grid_initialize / grid_clear / grid_cleanup                          */
/* ────────────────────────────────────────────────────────────────────── */

static void test_initialize_null(void) {
    reset_fake_state();
    assert(!grid_initialize(NULL));
}

static void test_initialize_success(void) {
    static Grid grid;
    fill_grid_with(&grid, SAND, SAND_BASE_COLOR);
    reset_fake_state();

    assert(grid_initialize(&grid));
    assert(fake_state.log_calls == 0);

    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            assert(grid.particles[y][x].type == EMPTY);
        }
    }
}

static void test_cleanup_null(void) {
    reset_fake_state();
    assert(!grid_reset(NULL));
}

static void test_cleanup_success(void) {
    static Grid grid;
    fill_grid_with(&grid, ROCK, ROCK_BASE_COLOR);
    reset_fake_state();

    assert(grid_reset(&grid));

    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            assert(grid.particles[y][x].type == EMPTY);
        }
    }
}

static void test_cleanup_null_cleanup(void) {
    reset_fake_state();
    assert(!grid_reset(NULL));
}

static void test_cleanup_clears(void) {
    static Grid grid;
    fill_grid_with(&grid, SAND, SAND_BASE_COLOR);
    reset_fake_state();

    assert(grid_reset(&grid));
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
/*  test_helper_grid_is_empty (local helper)                             */
/* ────────────────────────────────────────────────────────────────────── */

static void test_is_empty_null(void) {
    reset_fake_state();
    assert(!test_helper_grid_is_empty(NULL));
}

static void test_is_empty_true(void) {
    static Grid grid;
    grid_initialize(&grid);
    reset_fake_state();

    assert(test_helper_grid_is_empty(&grid));
}

static void test_is_empty_false_first_cell(void) {
    static Grid grid;
    grid_initialize(&grid);
    grid.particles[0][0].type = SAND;
    reset_fake_state();

    assert(!test_helper_grid_is_empty(&grid));
}

static void test_is_empty_false_last_cell(void) {
    static Grid grid;
    grid_initialize(&grid);
    grid.particles[GRID_HEIGHT - 1][GRID_WIDTH - 1].type = ROCK;
    reset_fake_state();

    assert(!test_helper_grid_is_empty(&grid));
}

/* ────────────────────────────────────────────────────────────────────── */
/*  grid_is_particle_empty                                               */
/* ────────────────────────────────────────────────────────────────────── */

static void test_particle_empty_null_grid(void) {
    reset_fake_state();
    assert(!grid_is_particle_empty(NULL, (Coordinates){0, 0}));
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
    Particle p = {.type = SAND, .color = SAND_BASE_COLOR};
    reset_fake_state();

    assert(!grid_set_particle(NULL, (Coordinates){0, 0}, &p));
}

static void test_set_particle_out_of_bounds(void) {
    static Grid grid;
    grid_initialize(&grid);
    Particle p = {.type = SAND, .color = SAND_BASE_COLOR};
    reset_fake_state();

    assert(!grid_set_particle(&grid, (Coordinates){GRID_WIDTH, 0}, &p));

    assert(!grid_set_particle(&grid, (Coordinates){0, -1}, &p));
}

static void test_set_particle_success(void) {
    static Grid grid;
    grid_initialize(&grid);
    Particle p = {.type = ROCK, .color = ROCK_BASE_COLOR};
    Coordinates pos = {10, 20};
    reset_fake_state();

    assert(grid_set_particle(&grid, pos, &p));
    assert(grid.particles[pos.y][pos.x].type == ROCK);
    assert(grid.particles[pos.y][pos.x].color.r == ROCK_BASE_COLOR.r);
    assert(grid.particles[pos.y][pos.x].color.g == ROCK_BASE_COLOR.g);
    assert(grid.particles[pos.y][pos.x].color.b == ROCK_BASE_COLOR.b);
    assert(fake_state.log_calls == 0);
}

static void test_set_particle_overwrite(void) {
    static Grid grid;
    grid_initialize(&grid);
    Particle sand = {.type = SAND, .color = SAND_BASE_COLOR};
    Particle rock = {.type = ROCK, .color = ROCK_BASE_COLOR};
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
    Particle p = {.type = SAND, .color = SAND_BASE_COLOR};
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
}

static void test_place_particle_out_of_bounds(void) {
    static Grid grid;
    grid_initialize(&grid);
    reset_fake_state();

    assert(!grid_place_particle(&grid, (Coordinates){GRID_WIDTH, 0}, SAND));
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
}

static void test_get_particle_out_of_bounds(void) {
    static Grid grid;
    grid_initialize(&grid);
    reset_fake_state();

    assert(grid_get_particle(&grid, (Coordinates){-1, 0}) == NULL);
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
    /* Should not crash */
}

static void test_update_calls_particle_update(void) {
    static Grid grid;
    grid_initialize(&grid);
    /* Place a sand particle and verify it moves after update */
    grid.particles[0][5] = (Particle){.type = SAND, .color = SAND_BASE_COLOR};
    reset_fake_state();

    grid_update(&grid);

    /* Sand at row 0 should have fallen to row 1 */
    assert(grid.particles[0][5].type == EMPTY);
    assert(grid.particles[1][5].type == SAND);
}

static void test_update_alternates_direction(void) {
    static Grid grid;
    grid_initialize(&grid);
    reset_fake_state();

    assert(grid.update_left_to_right == true);
    grid_update(&grid);
    assert(grid.update_left_to_right == false);
    grid_update(&grid);
    assert(grid.update_left_to_right == true);
}

/* ────────────────────────────────────────────────────────────────────── */
/*  grid_render                                                          */
/* ────────────────────────────────────────────────────────────────────── */

static void test_render_null_grid(void) {
    Display display = {.renderer = (SDL_Renderer *)0x1, .texture = (SDL_Texture *)0x3};
    reset_fake_state();

    grid_render(NULL, &display);
    assert(fake_state.render_texture_calls == 0);
}

static void test_render_null_display(void) {
    static Grid grid;
    grid_initialize(&grid);
    reset_fake_state();

    grid_render(&grid, NULL);
    assert(fake_state.render_texture_calls == 0);
}

static void test_render_null_renderer(void) {
    static Grid grid;
    grid_initialize(&grid);
    Display display = {.texture = (SDL_Texture *)0x3};
    reset_fake_state();

    grid_render(&grid, &display);
    assert(fake_state.render_texture_calls == 0);
}

static void test_render_null_texture(void) {
    static Grid grid;
    grid_initialize(&grid);
    Display display = {.renderer = (SDL_Renderer *)0x1};
    reset_fake_state();

    grid_render(&grid, &display);
    assert(fake_state.render_texture_calls == 0);
}

static void test_render_empty_grid(void) {
    static Grid grid;
    grid_initialize(&grid);
    grid.dirty = true;
    Display display = {.renderer = (SDL_Renderer *)0x1, .texture = (SDL_Texture *)0x3};
    reset_fake_state();

    grid_render(&grid, &display);
    assert(fake_state.log_calls == 0);
    assert(fake_state.lock_calls == 1);
    assert(fake_state.unlock_calls == 1);
    assert(fake_state.render_texture_calls == 1);
    assert(fake_state.last_locked_texture == display.texture);
    assert(fake_state.last_rendered_texture == display.texture);
    assert(fake_state.last_rendered_renderer == display.renderer);
    assert(get_pixel(0, 0).r == EMPTY_BASE_COLOR.r);
}

static void test_render_single_particle(void) {
    static Grid grid;
    grid_initialize(&grid);
     grid.particles[0][0] = (Particle){.type = SAND, .color = SAND_BASE_COLOR};
    grid.dirty = true;
    Display display = {.renderer = (SDL_Renderer *)0x1, .texture = (SDL_Texture *)0x3};
    reset_fake_state();

    grid_render(&grid, &display);
    assert(fake_state.render_texture_calls == 1);
    assert(get_pixel(0, 0).r == SAND_BASE_COLOR.r);
    assert(get_pixel(0, 0).g == SAND_BASE_COLOR.g);
    assert(get_pixel(0, 0).b == SAND_BASE_COLOR.b);
    assert(get_pixel(0, 0).a == SAND_BASE_COLOR.a);
}

static void test_render_multiple_particles(void) {
    static Grid grid;
    grid_initialize(&grid);
       grid.particles[0][0]   = (Particle){.type = SAND, .color = SAND_BASE_COLOR};
    grid.particles[5][10]  = (Particle){.type = ROCK, .color = ROCK_BASE_COLOR};
    grid.particles[100][50] = (Particle){.type = SAND, .color = SAND_BASE_COLOR};
    grid.dirty = true;
    Display display = {.renderer = (SDL_Renderer *)0x1, .texture = (SDL_Texture *)0x3};
    reset_fake_state();

    grid_render(&grid, &display);
    assert(fake_state.render_texture_calls == 1);
    assert(get_pixel(0, 0).r == SAND_BASE_COLOR.r);
    assert(get_pixel(10, 5).r == ROCK_BASE_COLOR.r);
    assert(get_pixel(50, 100).r == SAND_BASE_COLOR.r);
}

static void test_render_full_grid(void) {
    static Grid grid;
    fill_grid_with(&grid, SAND, SAND_BASE_COLOR);
    grid.dirty = true;
    Display display = {.renderer = (SDL_Renderer *)0x1, .texture = (SDL_Texture *)0x3};
    reset_fake_state();

    grid_render(&grid, &display);
    assert(fake_state.render_texture_calls == 1);
    assert(fake_state.lock_calls == 1);
    assert(fake_state.unlock_calls == 1);
}

/* ────────────────────────────────────────────────────────────────────── */
/*  Integration: place → get round-trip                                  */
/* ────────────────────────────────────────────────────────────────────── */

static void test_render_not_dirty_skips_lock(void) {
    static Grid grid;
    grid_initialize(&grid);
    grid.dirty = false;
    Display display = {.renderer = (SDL_Renderer *)0x1, .texture = (SDL_Texture *)0x3};
    reset_fake_state();

    grid_render(&grid, &display);
    /* Should still present texture but skip lock/unlock */
    assert(fake_state.render_texture_calls == 1);
    assert(fake_state.lock_calls == 0);
    assert(fake_state.unlock_calls == 0);
}

/* ────────────────────────────────────────────────────────────────────── */
/*  Generation counter: double-step prevention                           */
/* ────────────────────────────────────────────────────────────────────── */

static void test_update_increments_gen(void) {
    static Grid grid;
    grid_initialize(&grid);
    reset_fake_state();

    Uint8 gen_before = grid.current_gen;
    grid_update(&grid);
    assert(grid.current_gen == (Uint8)(gen_before + 1));
    grid_update(&grid);
    assert(grid.current_gen == (Uint8)(gen_before + 2));
}

static void test_update_no_double_step(void) {
    /*
     * Verify the gen counter prevents a particle from being processed
     * twice in one frame. We manually stamp a sand particle with the
     * upcoming generation and confirm grid_update does NOT move it.
     */
    static Grid grid;
    grid_initialize(&grid);

    /* Sand at (5, 0) — would normally fall to (5, 1) */
    grid.particles[0][5] = (Particle){.type = SAND, .color = SAND_BASE_COLOR};

    /* Pre-stamp it with the next gen (current_gen + 1, since update increments first) */
    grid.particles[0][5].update_gen = (Uint8)(grid.current_gen + 1);
    reset_fake_state();

    grid_update(&grid);

    /* Sand should NOT have moved — gen guard skipped it */
    assert(grid.particles[0][5].type == SAND);
    assert(grid.particles[1][5].type == EMPTY);
}

static void test_swap_marks_destination_gen(void) {
    static Grid grid;
    grid_initialize(&grid);
    grid.current_gen = 5;

    grid.particles[0][0] = (Particle){.type = SAND, .color = SAND_BASE_COLOR, .update_gen = 0};
    grid.particles[1][0] = (Particle){.type = EMPTY, .color = EMPTY_BASE_COLOR, .update_gen = 0};

    grid_swap(&grid, (Coordinates){0, 0}, (Coordinates){0, 1});

    /* The moved particle at destination should be stamped with current_gen */
    assert(grid.particles[1][0].update_gen == 5);
    assert(grid.particles[1][0].type == SAND);
}

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

static void test_clearnup_after_fill(void) {
    static Grid grid;
    fill_grid_with(&grid, ROCK, ROCK_BASE_COLOR);
    for (int y = 0; y < GRID_HEIGHT; y++)
        for (int x = 0; x < GRID_WIDTH; x++)
            assert(grid.particles[y][x].type != EMPTY);

    grid_reset(&grid);
    for (int y = 0; y < GRID_HEIGHT; y++)
        for (int x = 0; x < GRID_WIDTH; x++)
            assert(grid.particles[y][x].type == EMPTY);
}

/* ── Runner ──────────────────────────────────────────────────────────── */

int main(void) {
    /* Initialize / Clear / Cleanup */
    test_initialize_null();
    test_initialize_success();
    test_cleanup_null();
    test_cleanup_success();
    test_cleanup_null_cleanup();
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
    test_render_null_texture();
    test_render_empty_grid();
    test_render_single_particle();
    test_render_multiple_particles();
    test_render_full_grid();
    test_render_not_dirty_skips_lock();

    /* Generation counter */
    test_update_increments_gen();
    test_update_no_double_step();
    test_swap_marks_destination_gen();

    /* Integration */
    test_place_then_get();
    test_clearnup_after_fill();

    return 0;
}
