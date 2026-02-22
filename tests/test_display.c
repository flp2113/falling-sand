#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

/* ── Mock redirections ───────────────────────────────────────────────── */

#define SDL_InitSubSystem          fake_SDL_InitSubSystem
#define SDL_CreateWindowAndRenderer fake_SDL_CreateWindowAndRenderer
#define SDL_CreateTexture          fake_SDL_CreateTexture
#define SDL_SetTextureScaleMode    fake_SDL_SetTextureScaleMode
#define SDL_SetRenderLogicalPresentation fake_SDL_SetRenderLogicalPresentation
#define SDL_SetRenderVSync         fake_SDL_SetRenderVSync
#define SDL_DestroyRenderer        fake_SDL_DestroyRenderer
#define SDL_DestroyWindow          fake_SDL_DestroyWindow
#define SDL_DestroyTexture         fake_SDL_DestroyTexture
#define SDL_QuitSubSystem          fake_SDL_QuitSubSystem
#define SDL_Log                    fake_SDL_Log
#define SDL_GetError               fake_SDL_GetError

#include "../src/display/display.h"
#include "../src/display/display.c"

#undef SDL_InitSubSystem
#undef SDL_CreateWindowAndRenderer
#undef SDL_CreateTexture
#undef SDL_SetTextureScaleMode
#undef SDL_SetRenderLogicalPresentation
#undef SDL_SetRenderVSync
#undef SDL_DestroyRenderer
#undef SDL_DestroyWindow
#undef SDL_DestroyTexture
#undef SDL_QuitSubSystem
#undef SDL_Log
#undef SDL_GetError

/* ── Fake state ──────────────────────────────────────────────────────── */

typedef struct FakeSDLState {
    /* Return values (configurable per test) */
    bool init_return;
    bool create_return;
    bool texture_return;
    bool scale_return;
    bool logical_return;
    bool vsync_return;

    /* Call counters */
    int init_calls;
    int create_calls;
    int texture_calls;
    int scale_calls;
    int logical_calls;
    int vsync_calls;
    int destroy_renderer_calls;
    int destroy_window_calls;
    int destroy_texture_calls;
    int quit_calls;
    int log_calls;

    /* Captured arguments — InitSubSystem / QuitSubSystem */
    SDL_InitFlags last_init_flags;
    SDL_InitFlags last_quit_flags;

    /* Captured arguments — CreateWindowAndRenderer */
    const char *last_create_title;
    int last_create_width;
    int last_create_height;
    SDL_WindowFlags last_create_window_flags;

    /* Captured arguments — CreateTexture */
    SDL_Renderer *last_texture_renderer;
    SDL_PixelFormat last_texture_format;
    SDL_TextureAccess last_texture_access;
    int last_texture_width;
    int last_texture_height;

    /* Captured arguments — SetTextureScaleMode */
    SDL_Texture *last_scale_texture;
    SDL_ScaleMode last_scale_mode;

    /* Captured arguments — SetRenderLogicalPresentation */
    int last_logical_width;
    int last_logical_height;
    SDL_RendererLogicalPresentation last_presentation;

    /* Pointers handed back to the production code */
    SDL_Window   *created_window;
    SDL_Renderer *created_renderer;
    SDL_Texture  *created_texture;

    /* Pointers that cleanup received */
    SDL_Window   *last_destroyed_window;
    SDL_Renderer *last_destroyed_renderer;
    SDL_Texture  *last_destroyed_texture;
} FakeSDLState;

static FakeSDLState fake_sdl;

static void reset_fake_sdl(void) {
    memset(&fake_sdl, 0, sizeof(fake_sdl));
    fake_sdl.init_return    = true;
    fake_sdl.create_return  = true;
    fake_sdl.texture_return = true;
    fake_sdl.scale_return   = true;
    fake_sdl.logical_return = true;
    fake_sdl.vsync_return   = true;
    fake_sdl.created_window   = (SDL_Window   *)0x1;
    fake_sdl.created_renderer = (SDL_Renderer *)0x2;
    fake_sdl.created_texture  = (SDL_Texture  *)0x3;
}

/* ── Fake implementations ────────────────────────────────────────────── */

bool fake_SDL_InitSubSystem(SDL_InitFlags flags) {
    fake_sdl.init_calls++;
    fake_sdl.last_init_flags = flags;
    return fake_sdl.init_return;
}

bool fake_SDL_CreateWindowAndRenderer(const char *title, int width, int height,
                                      SDL_WindowFlags window_flags,
                                      SDL_Window **window,
                                      SDL_Renderer **renderer) {
    fake_sdl.create_calls++;
    fake_sdl.last_create_title        = title;
    fake_sdl.last_create_width        = width;
    fake_sdl.last_create_height       = height;
    fake_sdl.last_create_window_flags = window_flags;

    if (!fake_sdl.create_return) {
        return false;
    }
    if (window)   { *window   = fake_sdl.created_window;   }
    if (renderer) { *renderer = fake_sdl.created_renderer; }
    return true;
}

SDL_Texture *fake_SDL_CreateTexture(SDL_Renderer *renderer,
                                    SDL_PixelFormat format,
                                    SDL_TextureAccess access,
                                    int width, int height) {
    fake_sdl.texture_calls++;
    fake_sdl.last_texture_renderer = renderer;
    fake_sdl.last_texture_format = format;
    fake_sdl.last_texture_access = access;
    fake_sdl.last_texture_width = width;
    fake_sdl.last_texture_height = height;

    if (!fake_sdl.texture_return) {
        return NULL;
    }

    return fake_sdl.created_texture;
}

bool fake_SDL_SetTextureScaleMode(SDL_Texture *texture, SDL_ScaleMode scale_mode) {
    fake_sdl.scale_calls++;
    fake_sdl.last_scale_texture = texture;
    fake_sdl.last_scale_mode = scale_mode;
    return fake_sdl.scale_return;
}

bool fake_SDL_SetRenderLogicalPresentation(
        SDL_Renderer *renderer, int width, int height,
        SDL_RendererLogicalPresentation mode) {
    (void)renderer;
    fake_sdl.logical_calls++;
    fake_sdl.last_logical_width  = width;
    fake_sdl.last_logical_height = height;
    fake_sdl.last_presentation   = mode;
    return fake_sdl.logical_return;
}

bool fake_SDL_SetRenderVSync(SDL_Renderer *renderer, int vsync) {
    (void)renderer;
    (void)vsync;
    fake_sdl.vsync_calls++;
    return fake_sdl.vsync_return;
}

void fake_SDL_DestroyRenderer(SDL_Renderer *renderer) {
    fake_sdl.destroy_renderer_calls++;
    fake_sdl.last_destroyed_renderer = renderer;
}

void fake_SDL_DestroyWindow(SDL_Window *window) {
    fake_sdl.destroy_window_calls++;
    fake_sdl.last_destroyed_window = window;
}

void fake_SDL_DestroyTexture(SDL_Texture *texture) {
    fake_sdl.destroy_texture_calls++;
    fake_sdl.last_destroyed_texture = texture;
}

void fake_SDL_QuitSubSystem(SDL_InitFlags flags) {
    fake_sdl.quit_calls++;
    fake_sdl.last_quit_flags = flags;
}

void fake_SDL_Log(const char *fmt, ...) {
    (void)fmt;
    va_list args;
    va_start(args, fmt);
    va_end(args);
    fake_sdl.log_calls++;
}

const char *fake_SDL_GetError(void) { return "fake sdl error"; }

/* ── Helper assertions ───────────────────────────────────────────────── */

static void assert_no_sdl_calls(void) {
    assert(fake_sdl.init_calls == 0);
    assert(fake_sdl.create_calls == 0);
    assert(fake_sdl.texture_calls == 0);
    assert(fake_sdl.scale_calls == 0);
    assert(fake_sdl.logical_calls == 0);
    assert(fake_sdl.vsync_calls == 0);
    assert(fake_sdl.destroy_renderer_calls == 0);
    assert(fake_sdl.destroy_window_calls == 0);
    assert(fake_sdl.destroy_texture_calls == 0);
    assert(fake_sdl.quit_calls == 0);
}

static void assert_create_args(const DisplayConfig *cfg) {
    assert(fake_sdl.last_create_title == cfg->title);
    assert(fake_sdl.last_create_width == cfg->width);
    assert(fake_sdl.last_create_height == cfg->height);
    assert(fake_sdl.last_create_window_flags == cfg->window_flags);
}

static void assert_display_cleared(const Display *d) {
    assert(d->window == NULL);
    assert(d->renderer == NULL);
    assert(d->texture == NULL);
}

/* ── Initialization tests ────────────────────────────────────────────── */

static void test_initialize_null_display(void) {
    reset_fake_sdl();
    DisplayConfig config = {
        .title = "Title", .width = 640, .height = 480,
        .window_flags = 0, .init_flags = SDL_INIT_VIDEO,
        .presentation = (SDL_RendererLogicalPresentation)0
    };

    assert(!display_initialize(NULL, &config));
    assert_no_sdl_calls();
    assert(fake_sdl.log_calls == 1);
}

static void test_initialize_null_config(void) {
    reset_fake_sdl();
    Display display = {0};

    assert(!display_initialize(&display, NULL));
    assert_no_sdl_calls();
    assert(fake_sdl.log_calls == 1);
}

static void test_initialize_init_failure(void) {
    Display display = {0};
    DisplayConfig config = {
        .title = "Title", .width = 640, .height = 480,
        .window_flags = 0, .init_flags = SDL_INIT_VIDEO,
        .presentation = (SDL_RendererLogicalPresentation)0
    };

    reset_fake_sdl();
    fake_sdl.init_return = false;

    assert(!display_initialize(&display, &config));
    assert(fake_sdl.init_calls == 1);
    assert(fake_sdl.create_calls == 0);
    assert(fake_sdl.texture_calls == 0);
    assert(fake_sdl.scale_calls == 0);
    assert(fake_sdl.logical_calls == 0);
    assert(fake_sdl.vsync_calls == 0);
    assert(fake_sdl.quit_calls == 0);
    assert(fake_sdl.log_calls == 1);
    assert(fake_sdl.last_init_flags == config.init_flags);
    assert_display_cleared(&display);
    assert(display.init_flags == config.init_flags);
}

static void test_initialize_create_failure(void) {
    Display display = {0};
    DisplayConfig config = {
        .title = "Title", .width = 640, .height = 480,
        .window_flags = SDL_WINDOW_FULLSCREEN,
        .init_flags = SDL_INIT_VIDEO,
        .presentation = (SDL_RendererLogicalPresentation)1
    };

    reset_fake_sdl();
    fake_sdl.create_return = false;

    assert(!display_initialize(&display, &config));
    assert(fake_sdl.init_calls == 1);
    assert(fake_sdl.create_calls == 1);
    assert(fake_sdl.texture_calls == 0);
    assert(fake_sdl.scale_calls == 0);
    assert(fake_sdl.logical_calls == 0);
    assert(fake_sdl.vsync_calls == 0);
    assert(fake_sdl.quit_calls == 1);
    assert(fake_sdl.log_calls == 1);
    assert(fake_sdl.last_init_flags == config.init_flags);
    assert(fake_sdl.last_quit_flags == config.init_flags);
    assert_create_args(&config);
    assert_display_cleared(&display);
}

static void test_initialize_texture_failure(void) {
    Display display = {0};
    DisplayConfig config = {
        .title = "Title", .width = 640, .height = 480,
        .window_flags = SDL_WINDOW_FULLSCREEN,
        .init_flags = SDL_INIT_VIDEO,
        .presentation = (SDL_RendererLogicalPresentation)1
    };

    reset_fake_sdl();
    fake_sdl.texture_return = false;

    assert(!display_initialize(&display, &config));
    assert(fake_sdl.init_calls == 1);
    assert(fake_sdl.create_calls == 1);
    assert(fake_sdl.texture_calls == 1);
    assert(fake_sdl.scale_calls == 0);
    assert(fake_sdl.logical_calls == 0);
    assert(fake_sdl.vsync_calls == 0);
    assert(fake_sdl.destroy_renderer_calls == 1);
    assert(fake_sdl.destroy_window_calls == 1);
    assert(fake_sdl.destroy_texture_calls == 1);
    assert(fake_sdl.quit_calls == 2);
    assert(fake_sdl.log_calls == 1);
    assert(fake_sdl.last_quit_flags == config.init_flags);
    assert_create_args(&config);
    assert_display_cleared(&display);
}

static void test_initialize_scale_failure(void) {
    Display display = {0};
    DisplayConfig config = {
        .title = "Title", .width = 640, .height = 480,
        .window_flags = SDL_WINDOW_RESIZABLE,
        .init_flags = SDL_INIT_VIDEO,
        .presentation = (SDL_RendererLogicalPresentation)2
    };

    reset_fake_sdl();
    fake_sdl.scale_return = false;

    assert(!display_initialize(&display, &config));
    assert(fake_sdl.init_calls == 1);
    assert(fake_sdl.create_calls == 1);
    assert(fake_sdl.texture_calls == 1);
    assert(fake_sdl.scale_calls == 1);
    assert(fake_sdl.logical_calls == 0);
    assert(fake_sdl.vsync_calls == 0);
    assert(fake_sdl.destroy_renderer_calls == 1);
    assert(fake_sdl.destroy_window_calls == 1);
    assert(fake_sdl.destroy_texture_calls == 1);
    assert(fake_sdl.quit_calls == 2);
    assert(fake_sdl.log_calls == 1);
    assert(fake_sdl.last_quit_flags == config.init_flags);
    assert(fake_sdl.last_scale_texture == fake_sdl.created_texture);
    assert(fake_sdl.last_scale_mode == SDL_SCALEMODE_NEAREST);
    assert_display_cleared(&display);
}

static void test_initialize_logical_failure(void) {
    Display display = {0};
    DisplayConfig config = {
        .title = "Title", .width = 640, .height = 480,
        .window_flags = SDL_WINDOW_RESIZABLE,
        .init_flags = SDL_INIT_VIDEO,
        .presentation = (SDL_RendererLogicalPresentation)2
    };

    reset_fake_sdl();
    fake_sdl.logical_return = false;

    assert(!display_initialize(&display, &config));
    assert(fake_sdl.init_calls == 1);
    assert(fake_sdl.create_calls == 1);
    assert(fake_sdl.texture_calls == 1);
    assert(fake_sdl.scale_calls == 1);
    assert(fake_sdl.logical_calls == 1);
    assert(fake_sdl.vsync_calls == 0);
    assert(fake_sdl.destroy_renderer_calls == 1);
    assert(fake_sdl.destroy_window_calls == 1);
    assert(fake_sdl.destroy_texture_calls == 1);
    assert(fake_sdl.quit_calls == 2);
    assert(fake_sdl.log_calls == 1);
    assert(fake_sdl.last_quit_flags == config.init_flags);
    assert(fake_sdl.last_presentation == config.presentation);
    assert(fake_sdl.last_destroyed_renderer == fake_sdl.created_renderer);
    assert(fake_sdl.last_destroyed_window == fake_sdl.created_window);
    assert(fake_sdl.last_destroyed_texture == fake_sdl.created_texture);
    assert_display_cleared(&display);
}

static void test_initialize_vsync_failure(void) {
    Display display = {0};
    DisplayConfig config = {
        .title = "Title", .width = 640, .height = 480,
        .window_flags = SDL_WINDOW_RESIZABLE,
        .init_flags = SDL_INIT_VIDEO,
        .presentation = (SDL_RendererLogicalPresentation)2
    };

    reset_fake_sdl();
    fake_sdl.vsync_return = false;

    assert(!display_initialize(&display, &config));
    assert(fake_sdl.init_calls == 1);
    assert(fake_sdl.create_calls == 1);
    assert(fake_sdl.texture_calls == 1);
    assert(fake_sdl.scale_calls == 1);
    assert(fake_sdl.logical_calls == 1);
    assert(fake_sdl.vsync_calls == 1);
    assert(fake_sdl.destroy_renderer_calls == 1);
    assert(fake_sdl.destroy_window_calls == 1);
    assert(fake_sdl.destroy_texture_calls == 1);
    assert(fake_sdl.quit_calls == 2);
    assert(fake_sdl.log_calls == 1);
    assert(fake_sdl.last_destroyed_renderer == fake_sdl.created_renderer);
    assert(fake_sdl.last_destroyed_window == fake_sdl.created_window);
    assert(fake_sdl.last_destroyed_texture == fake_sdl.created_texture);
    assert(fake_sdl.last_quit_flags == config.init_flags);
    assert_display_cleared(&display);
}

static void test_initialize_success(void) {
    Display display = {0};
    DisplayConfig config = {
        .title = "Title", .width = 640, .height = 480,
        .window_flags = SDL_WINDOW_MOUSE_GRABBED,
        .init_flags = SDL_INIT_VIDEO,
        .presentation = (SDL_RendererLogicalPresentation)3
    };

    reset_fake_sdl();

    assert(display_initialize(&display, &config));
    assert(fake_sdl.init_calls == 1);
    assert(fake_sdl.create_calls == 1);
    assert(fake_sdl.texture_calls == 1);
    assert(fake_sdl.scale_calls == 1);
    assert(fake_sdl.logical_calls == 1);
    assert(fake_sdl.vsync_calls == 1);
    assert(fake_sdl.destroy_renderer_calls == 0);
    assert(fake_sdl.destroy_window_calls == 0);
    assert(fake_sdl.destroy_texture_calls == 0);
    assert(fake_sdl.quit_calls == 0);
    assert(fake_sdl.log_calls == 0);
    assert(fake_sdl.last_init_flags == config.init_flags);
    assert_create_args(&config);
    assert(fake_sdl.last_presentation == config.presentation);
    assert(fake_sdl.last_logical_width == config.width);
    assert(fake_sdl.last_logical_height == config.height);
    assert(fake_sdl.last_texture_renderer == fake_sdl.created_renderer);
    assert(fake_sdl.last_texture_format == SDL_PIXELFORMAT_RGBA32);
    assert(fake_sdl.last_texture_access == SDL_TEXTUREACCESS_STREAMING);
    assert(fake_sdl.last_texture_width == GRID_WIDTH);
    assert(fake_sdl.last_texture_height == GRID_HEIGHT);
    assert(fake_sdl.last_scale_texture == fake_sdl.created_texture);
    assert(fake_sdl.last_scale_mode == SDL_SCALEMODE_NEAREST);
    assert(display.window == fake_sdl.created_window);
    assert(display.renderer == fake_sdl.created_renderer);
    assert(display.texture == fake_sdl.created_texture);
    assert(display.init_flags == config.init_flags);
}

/* ── Cleanup tests ───────────────────────────────────────────────────── */

static void test_cleanup_null_display(void) {
    reset_fake_sdl();

    display_cleanup(NULL);

    assert(fake_sdl.destroy_renderer_calls == 0);
    assert(fake_sdl.destroy_window_calls == 0);
    assert(fake_sdl.destroy_texture_calls == 0);
    assert(fake_sdl.quit_calls == 0);
    assert(fake_sdl.log_calls == 1);
}

static void test_cleanup_zeroed_display(void) {
    Display display = {0};
    reset_fake_sdl();

    display_cleanup(&display);

    assert(fake_sdl.destroy_renderer_calls == 0);
    assert(fake_sdl.destroy_window_calls == 0);
    assert(fake_sdl.destroy_texture_calls == 0);
    assert(fake_sdl.quit_calls == 1);
    assert(fake_sdl.last_quit_flags == 0);
    assert_display_cleared(&display);
}

static void test_cleanup_only_renderer(void) {
    Display display = {0};
    reset_fake_sdl();
    display.renderer   = fake_sdl.created_renderer;
    display.init_flags = SDL_INIT_VIDEO;

    display_cleanup(&display);

    assert(fake_sdl.destroy_renderer_calls == 1);
    assert(fake_sdl.last_destroyed_renderer == (SDL_Renderer *)0x2);
    assert(fake_sdl.destroy_window_calls == 0);
    assert(fake_sdl.destroy_texture_calls == 0);
    assert(fake_sdl.quit_calls == 1);
    assert(fake_sdl.last_quit_flags == SDL_INIT_VIDEO);
    assert_display_cleared(&display);
}

static void test_cleanup_only_window(void) {
    Display display = {0};
    reset_fake_sdl();
    display.window     = fake_sdl.created_window;
    display.init_flags = SDL_INIT_VIDEO;

    display_cleanup(&display);

    assert(fake_sdl.destroy_renderer_calls == 0);
    assert(fake_sdl.destroy_window_calls == 1);
    assert(fake_sdl.last_destroyed_window == (SDL_Window *)0x1);
    assert(fake_sdl.destroy_texture_calls == 0);
    assert(fake_sdl.quit_calls == 1);
    assert(fake_sdl.last_quit_flags == SDL_INIT_VIDEO);
    assert_display_cleared(&display);
}

static void test_cleanup_only_texture(void) {
    Display display = {0};
    reset_fake_sdl();
    display.texture   = fake_sdl.created_texture;
    display.init_flags = SDL_INIT_VIDEO;

    display_cleanup(&display);

    assert(fake_sdl.destroy_renderer_calls == 0);
    assert(fake_sdl.destroy_window_calls == 0);
    assert(fake_sdl.destroy_texture_calls == 1);
    assert(fake_sdl.last_destroyed_texture == (SDL_Texture *)0x3);
    assert(fake_sdl.quit_calls == 1);
    assert(fake_sdl.last_quit_flags == SDL_INIT_VIDEO);
    assert_display_cleared(&display);
}

static void test_cleanup_with_resources(void) {
    Display display = {0};
    reset_fake_sdl();
    display.window     = fake_sdl.created_window;
    display.renderer   = fake_sdl.created_renderer;
    display.texture    = fake_sdl.created_texture;
    display.init_flags = SDL_INIT_VIDEO;

    display_cleanup(&display);

    assert(fake_sdl.destroy_renderer_calls == 1);
    assert(fake_sdl.destroy_window_calls == 1);
    assert(fake_sdl.destroy_texture_calls == 1);
    assert(fake_sdl.quit_calls == 1);
    assert(fake_sdl.last_quit_flags == SDL_INIT_VIDEO);
    assert_display_cleared(&display);
}

static void test_cleanup_double_call(void) {
    Display display = {0};
    reset_fake_sdl();
    display.window     = fake_sdl.created_window;
    display.renderer   = fake_sdl.created_renderer;
    display.texture    = fake_sdl.created_texture;
    display.init_flags = SDL_INIT_VIDEO;

    display_cleanup(&display);
    display_cleanup(&display);

    /* Second call should not destroy already-NULL pointers again */
    assert(fake_sdl.destroy_renderer_calls == 1);
    assert(fake_sdl.destroy_window_calls == 1);
    assert(fake_sdl.destroy_texture_calls == 1);
    assert(fake_sdl.quit_calls == 2);
    assert_display_cleared(&display);
}

/* ── Integration test ────────────────────────────────────────────────── */

static void test_full_lifecycle(void) {
    Display display = {0};
    DisplayConfig config = {
        .title = "Life", .width = 800, .height = 600,
        .window_flags = 0, .init_flags = SDL_INIT_VIDEO,
        .presentation = (SDL_RendererLogicalPresentation)0
    };

    reset_fake_sdl();

    assert(display_initialize(&display, &config));
    assert(display.window != NULL);
    assert(display.renderer != NULL);

    display_cleanup(&display);

    assert(fake_sdl.destroy_renderer_calls == 1);
    assert(fake_sdl.destroy_window_calls == 1);
    assert(fake_sdl.destroy_texture_calls == 1);
    assert(fake_sdl.quit_calls == 1);
    assert(fake_sdl.last_quit_flags == config.init_flags);
    assert_display_cleared(&display);
}

/* ── Runner ──────────────────────────────────────────────────────────── */

int main(void) {
    /* Initialization */
    test_initialize_null_display();
    test_initialize_null_config();
    test_initialize_init_failure();
    test_initialize_create_failure();
    test_initialize_texture_failure();
    test_initialize_scale_failure();
    test_initialize_logical_failure();
    test_initialize_vsync_failure();
    test_initialize_success();

    /* Cleanup */
    test_cleanup_null_display();
    test_cleanup_zeroed_display();
    test_cleanup_only_renderer();
    test_cleanup_only_window();
    test_cleanup_only_texture();
    test_cleanup_with_resources();
    test_cleanup_double_call();

    /* Integration */
    test_full_lifecycle();

    return 0;
}
