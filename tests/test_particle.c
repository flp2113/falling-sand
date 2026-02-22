#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

/* ── Mock redirections ───────────────────────────────────────────────── */

#define SDL_rand                    fake_SDL_rand
#define SDL_Log                     fake_SDL_Log
#define SDL_SetRenderDrawColor      fake_SDL_SetRenderDrawColor
#define SDL_RenderFillRect          fake_SDL_RenderFillRect
#define grid_is_in_bounds           fake_grid_is_in_bounds
#define grid_is_particle_empty      fake_grid_is_particle_empty

#include "../src/particle/particle.h"
#include "../src/particle/particle.c"

#undef SDL_rand
#undef SDL_Log
#undef SDL_SetRenderDrawColor
#undef SDL_RenderFillRect
#undef grid_is_in_bounds
#undef grid_is_particle_empty

/* ── Fake state ──────────────────────────────────────────────────────── */

#define RAND_QUEUE_SIZE 64

typedef struct FakeState {
    int log_calls;
    int rand_calls;
    int set_color_calls;
    int fill_rect_calls;
    int is_in_bounds_calls;
    int is_particle_empty_calls;

    /* Controllable SDL_rand return queue */
    Sint32 rand_queue[RAND_QUEUE_SIZE];
    int    rand_queue_len;
    int    rand_queue_pos;

    /* Capture last SDL_SetRenderDrawColor arguments */
    Uint8  last_r, last_g, last_b, last_a;
    SDL_Renderer *last_renderer;

    /* Capture last SDL_RenderFillRect arguments */
    SDL_FRect last_rect;
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

bool fake_SDL_SetRenderDrawColor(SDL_Renderer *renderer, Uint8 r, Uint8 g,
                                  Uint8 b, Uint8 a) {
    fake_state.set_color_calls++;
    fake_state.last_renderer = renderer;
    fake_state.last_r = r;
    fake_state.last_g = g;
    fake_state.last_b = b;
    fake_state.last_a = a;
    return true;
}

bool fake_SDL_RenderFillRect(SDL_Renderer *renderer, const SDL_FRect *rect) {
    (void)renderer;
    fake_state.fill_rect_calls++;
    if (rect) { fake_state.last_rect = *rect; }
    return true;
}

/*
 * Grid fakes: use faithful logic so physics tests run against real data.
 * This is intentional — these are trivial pure functions, and providing
 * real logic makes particle physics tests far more meaningful.
 */

bool fake_grid_is_in_bounds(Coordinates coordinates) {
    fake_state.is_in_bounds_calls++;
    return coordinates.x >= 0 && coordinates.x < GRID_WIDTH &&
           coordinates.y >= 0 && coordinates.y < GRID_HEIGHT;
}

bool fake_grid_is_particle_empty(Grid *grid, Coordinates coordinates) {
    fake_state.is_particle_empty_calls++;
    if (!grid) { return false; }
    if (!fake_grid_is_in_bounds(coordinates)) { return false; }
    return grid->particles[coordinates.y][coordinates.x].type == EMPTY;
}

/* ── Helpers ─────────────────────────────────────────────────────────── */

static void init_grid_empty(Grid *grid) {
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            grid->particles[y][x] = (Particle){.type = EMPTY, .color = COLOR_EMPTY};
        }
    }
}

static void place(Grid *grid, int x, int y, ParticleType type) {
    grid->particles[y][x] = (Particle){
        .type = type,
        .color = (type == SAND ? COLOR_SAND : (type == ROCK ? COLOR_ROCK : COLOR_EMPTY))
    };
}

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

    SDL_Color c = particle_get_random_sand_color();
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

    SDL_Color c = particle_get_random_sand_color();
    assert(c.r == clamp_color_component(SAND_COLOR_BASE_R + SAND_COLOR_VARIATION));
    assert(c.g == clamp_color_component(SAND_COLOR_BASE_G + SAND_COLOR_VARIATION));
    assert(c.b == clamp_color_component(SAND_COLOR_BASE_B + SAND_COLOR_VARIATION));
}

static void test_random_sand_color_max_negative(void) {
    reset_fake_state();
    /* Returning 0 → offset = -VARIATION */
    Sint32 vals[] = {0, 0, 0};
    push_rand_values(vals, 3);

    SDL_Color c = particle_get_random_sand_color();
    assert(c.r == clamp_color_component(SAND_COLOR_BASE_R - SAND_COLOR_VARIATION));
    assert(c.g == clamp_color_component(SAND_COLOR_BASE_G - SAND_COLOR_VARIATION));
    assert(c.b == clamp_color_component(SAND_COLOR_BASE_B - SAND_COLOR_VARIATION));
}

static void test_random_rock_color_center(void) {
    reset_fake_state();
    Sint32 vals[] = {ROCK_COLOR_VARIATION, ROCK_COLOR_VARIATION,
                     ROCK_COLOR_VARIATION};
    push_rand_values(vals, 3);

    SDL_Color c = particle_get_random_rock_color();
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
    assert(fake_state.log_calls == 1);
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
/*  particle_swap_in_grid (static)                                       */
/* ────────────────────────────────────────────────────────────────────── */

static void test_swap_basic(void) {
    static Grid grid;
    init_grid_empty(&grid);
    place(&grid, 5, 5, SAND);
    reset_fake_state();

    Coordinates a = {5, 5};
    Coordinates b = {5, 6};
    particle_swap_in_grid(&grid, a, b);

    assert(grid.particles[5][5].type == EMPTY);
    assert(grid.particles[6][5].type == SAND);
    assert(fake_state.log_calls == 0);
}

static void test_swap_same_position(void) {
    static Grid grid;
    init_grid_empty(&grid);
    place(&grid, 3, 3, ROCK);
    reset_fake_state();

    particle_swap_in_grid(&grid, (Coordinates){3, 3}, (Coordinates){3, 3});
    assert(grid.particles[3][3].type == ROCK);
    assert(fake_state.log_calls == 0);
}

/* ────────────────────────────────────────────────────────────────────── */
/*  particle_update_sand (static) — physics behaviour                    */
/* ────────────────────────────────────────────────────────────────────── */

static void test_sand_falls_down(void) {
    static Grid grid;
    init_grid_empty(&grid);
    place(&grid, 10, 5, SAND);  /* sand at (10,5) */
    reset_fake_state();

    particle_update_sand(&grid, (Coordinates){10, 5});
    assert(grid.particles[5][10].type == EMPTY);   /* vacated */
    assert(grid.particles[6][10].type == SAND);     /* fell down */
}

static void test_sand_at_bottom_stays(void) {
    static Grid grid;
    init_grid_empty(&grid);
    int bottom = GRID_HEIGHT - 1;
    place(&grid, 10, bottom, SAND);
    reset_fake_state();

    particle_update_sand(&grid, (Coordinates){10, bottom});
    assert(grid.particles[bottom][10].type == SAND);
}

static void test_sand_blocked_below_stays(void) {
    static Grid grid;
    init_grid_empty(&grid);
    place(&grid, 10, 5, SAND);
    place(&grid, 10, 6, ROCK);   /* directly below */
    place(&grid, 9,  6, ROCK);   /* below-left */
    place(&grid, 11, 6, ROCK);   /* below-right */
    reset_fake_state();

    particle_update_sand(&grid, (Coordinates){10, 5});
    assert(grid.particles[5][10].type == SAND);  /* didn't move */
}

static void test_sand_slides_left_only(void) {
    static Grid grid;
    init_grid_empty(&grid);
    place(&grid, 10, 5, SAND);
    place(&grid, 10, 6, ROCK);   /* below blocked */
    /* below-left (9,6) is empty */
    place(&grid, 11, 6, ROCK);   /* below-right blocked */
    reset_fake_state();

    particle_update_sand(&grid, (Coordinates){10, 5});
    assert(grid.particles[5][10].type == EMPTY);
    assert(grid.particles[6][9].type == SAND);
}

static void test_sand_slides_right_only(void) {
    static Grid grid;
    init_grid_empty(&grid);
    place(&grid, 10, 5, SAND);
    place(&grid, 10, 6, ROCK);   /* below blocked */
    place(&grid, 9,  6, ROCK);   /* below-left blocked */
    /* below-right (11,6) is empty */
    reset_fake_state();

    particle_update_sand(&grid, (Coordinates){10, 5});
    assert(grid.particles[5][10].type == EMPTY);
    assert(grid.particles[6][11].type == SAND);
}

static void test_sand_random_slides_left(void) {
    static Grid grid;
    init_grid_empty(&grid);
    place(&grid, 10, 5, SAND);
    place(&grid, 10, 6, ROCK);   /* below blocked */
    /* both diagonals empty */
    reset_fake_state();
    /* SDL_rand(2): return 1 → go_left = true */
    Sint32 vals[] = {1};
    push_rand_values(vals, 1);

    particle_update_sand(&grid, (Coordinates){10, 5});
    assert(grid.particles[6][9].type == SAND);   /* went left */
}

static void test_sand_random_slides_right(void) {
    static Grid grid;
    init_grid_empty(&grid);
    place(&grid, 10, 5, SAND);
    place(&grid, 10, 6, ROCK);   /* below blocked */
    /* both diagonals empty */
    reset_fake_state();
    /* SDL_rand(2): return 0 → go_left = false */
    Sint32 vals[] = {0};
    push_rand_values(vals, 1);

    particle_update_sand(&grid, (Coordinates){10, 5});
    assert(grid.particles[6][11].type == SAND);  /* went right */
}

static void test_sand_left_edge_cannot_slide_left(void) {
    static Grid grid;
    init_grid_empty(&grid);
    place(&grid, 0, 5, SAND);
    place(&grid, 0, 6, ROCK);    /* below blocked */
    /* below-left (−1,6) is out of bounds → treated as non-empty */
    /* below-right (1,6) is empty */
    reset_fake_state();

    particle_update_sand(&grid, (Coordinates){0, 5});
    assert(grid.particles[5][0].type == EMPTY);
    assert(grid.particles[6][1].type == SAND);   /* slid right */
}

static void test_sand_right_edge_cannot_slide_right(void) {
    static Grid grid;
    init_grid_empty(&grid);
    int rightmost = GRID_WIDTH - 1;
    place(&grid, rightmost, 5, SAND);
    place(&grid, rightmost, 6, ROCK);             /* below blocked */
    /* below-right (GRID_WIDTH, 6) out of bounds */
    /* below-left (rightmost-1, 6) is empty */
    reset_fake_state();

    particle_update_sand(&grid, (Coordinates){rightmost, 5});
    assert(grid.particles[5][rightmost].type == EMPTY);
    assert(grid.particles[6][rightmost - 1].type == SAND);
}

static void test_sand_penultimate_row_falls(void) {
    static Grid grid;
    init_grid_empty(&grid);
    int row = GRID_HEIGHT - 2;
    place(&grid, 10, row, SAND);
    reset_fake_state();

    particle_update_sand(&grid, (Coordinates){10, row});
    assert(grid.particles[row][10].type == EMPTY);
    assert(grid.particles[row + 1][10].type == SAND);
}

/* ────────────────────────────────────────────────────────────────────── */
/*  particle_update_in_grid (public dispatch)                            */
/* ────────────────────────────────────────────────────────────────────── */

static void test_update_in_grid_null_grid(void) {
    reset_fake_state();
    particle_update_in_grid(NULL, (Coordinates){0, 0});
    assert(fake_state.log_calls == 1);
}

static void test_update_in_grid_sand_dispatches(void) {
    static Grid grid;
    init_grid_empty(&grid);
    place(&grid, 5, 5, SAND);
    reset_fake_state();

    particle_update_in_grid(&grid, (Coordinates){5, 5});
    /* Sand should have fallen */
    assert(grid.particles[5][5].type == EMPTY);
    assert(grid.particles[6][5].type == SAND);
}

static void test_update_in_grid_rock_noop(void) {
    static Grid grid;
    init_grid_empty(&grid);
    place(&grid, 5, 5, ROCK);
    reset_fake_state();

    particle_update_in_grid(&grid, (Coordinates){5, 5});
    assert(grid.particles[5][5].type == ROCK);  /* unchanged */
}

static void test_update_in_grid_empty_noop(void) {
    static Grid grid;
    init_grid_empty(&grid);
    reset_fake_state();

    particle_update_in_grid(&grid, (Coordinates){5, 5});
    assert(grid.particles[5][5].type == EMPTY); /* unchanged */
}

/* ────────────────────────────────────────────────────────────────────── */
/*  particle_render                                                      */
/* ────────────────────────────────────────────────────────────────────── */

static void test_render_sets_color(void) {
    Display display = {.renderer = (SDL_Renderer *)0xDEAD};
    Particle p = {.type = SAND, .color = {245, 227, 66, 255}};
    reset_fake_state();

    particle_render(&display, &p, (Coordinates){0, 0});
    assert(fake_state.set_color_calls == 1);
    assert(fake_state.last_r == 245);
    assert(fake_state.last_g == 227);
    assert(fake_state.last_b == 66);
    assert(fake_state.last_a == 255);
    assert(fake_state.last_renderer == (SDL_Renderer *)0xDEAD);
}

static void test_render_rect_position(void) {
    Display display = {.renderer = (SDL_Renderer *)0x1};
    Particle p = {.type = ROCK, .color = COLOR_ROCK};
    Coordinates pos = {10, 20};
    reset_fake_state();

    particle_render(&display, &p, pos);
    assert(fake_state.fill_rect_calls == 1);
    assert(fake_state.last_rect.x == (float)(10 * PARTICLE_SIZE));
    assert(fake_state.last_rect.y == (float)(20 * PARTICLE_SIZE));
    assert(fake_state.last_rect.w == (float)(PARTICLE_SIZE));
    assert(fake_state.last_rect.h == (float)(PARTICLE_SIZE));
}

static void test_render_origin(void) {
    Display display = {.renderer = (SDL_Renderer *)0x1};
    Particle p = {.type = SAND, .color = COLOR_SAND};
    reset_fake_state();

    particle_render(&display, &p, (Coordinates){0, 0});
    assert(fake_state.last_rect.x == 0.0f);
    assert(fake_state.last_rect.y == 0.0f);
}

static void test_render_max_corner(void) {
    Display display = {.renderer = (SDL_Renderer *)0x1};
    Particle p = {.type = SAND, .color = COLOR_SAND};
    Coordinates pos = {GRID_WIDTH - 1, GRID_HEIGHT - 1};
    reset_fake_state();

    particle_render(&display, &p, pos);
    assert(fake_state.last_rect.x == (float)((GRID_WIDTH - 1) * PARTICLE_SIZE));
    assert(fake_state.last_rect.y == (float)((GRID_HEIGHT - 1) * PARTICLE_SIZE));
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

    /* Swap */
    test_swap_basic();
    test_swap_same_position();

    /* Sand physics */
    test_sand_falls_down();
    test_sand_at_bottom_stays();
    test_sand_blocked_below_stays();
    test_sand_slides_left_only();
    test_sand_slides_right_only();
    test_sand_random_slides_left();
    test_sand_random_slides_right();
    test_sand_left_edge_cannot_slide_left();
    test_sand_right_edge_cannot_slide_right();
    test_sand_penultimate_row_falls();

    /* particle_update_in_grid dispatch */
    test_update_in_grid_null_grid();
    test_update_in_grid_sand_dispatches();
    test_update_in_grid_rock_noop();
    test_update_in_grid_empty_noop();

    /* Render */
    test_render_sets_color();
    test_render_rect_position();
    test_render_origin();
    test_render_max_corner();

    return 0;
}
