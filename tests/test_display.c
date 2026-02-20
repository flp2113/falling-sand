#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#define SDL_InitSubSystem fake_SDL_InitSubSystem
#define SDL_CreateWindowAndRenderer fake_SDL_CreateWindowAndRenderer
#define SDL_SetRenderLogicalPresentation fake_SDL_SetRenderLogicalPresentation
#define SDL_DestroyRenderer fake_SDL_DestroyRenderer
#define SDL_DestroyWindow fake_SDL_DestroyWindow
#define SDL_QuitSubSystem fake_SDL_QuitSubSystem
#define SDL_Log fake_SDL_Log
#define SDL_GetError fake_SDL_GetError

#include "../src/display/display.c"

#undef SDL_InitSubSystem
#undef SDL_CreateWindowAndRenderer
#undef SDL_SetRenderLogicalPresentation
#undef SDL_DestroyRenderer
#undef SDL_DestroyWindow
#undef SDL_QuitSubSystem
#undef SDL_Log
#undef SDL_GetError

typedef struct FakeSDLState {
    bool init_return;
    bool create_return;
    bool logical_return;
    int init_calls;
    int create_calls;
    int logical_calls;
    int destroy_renderer_calls;
    int destroy_window_calls;
    int quit_calls;
    int log_calls;
    SDL_InitFlags last_init_flags;
    SDL_InitFlags last_quit_flags;
    SDL_WindowFlags last_window_flags;
    SDL_RendererLogicalPresentation last_presentation;
    int last_width;
    int last_height;
    const char *last_title;
    SDL_Window *created_window;
    SDL_Renderer *created_renderer;
    SDL_Window *last_destroyed_window;
    SDL_Renderer *last_destroyed_renderer;
} FakeSDLState;

static FakeSDLState fake_sdl;

static void reset_fake_sdl(void) {
    memset(&fake_sdl, 0, sizeof(fake_sdl));
    fake_sdl.init_return = true;
    fake_sdl.create_return = true;
    fake_sdl.logical_return = true;
    fake_sdl.created_window = (SDL_Window *)0x1;
    fake_sdl.created_renderer = (SDL_Renderer *)0x2;
}

static void assert_common_args(const char *title, int width, int height,
                               SDL_WindowFlags window_flags,
                               SDL_InitFlags init_flags) {
    assert(fake_sdl.last_title == title);
    assert(fake_sdl.last_width == width);
    assert(fake_sdl.last_height == height);
    assert(fake_sdl.last_window_flags == window_flags);
    assert(fake_sdl.last_init_flags == init_flags);
}

static void assert_presentation(SDL_RendererLogicalPresentation mode) {
    assert(fake_sdl.last_presentation == mode);
}

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
    fake_sdl.last_title = title;
    fake_sdl.last_width = width;
    fake_sdl.last_height = height;
    fake_sdl.last_window_flags = window_flags;

    if (!fake_sdl.create_return) {
        return false;
    }

    if (window) {
        *window = fake_sdl.created_window;
    }

    if (renderer) {
        *renderer = fake_sdl.created_renderer;
    }

    return true;
}

bool fake_SDL_SetRenderLogicalPresentation(
    SDL_Renderer *renderer, int width, int height,
    SDL_RendererLogicalPresentation mode) {
    (void)renderer;
    fake_sdl.logical_calls++;
    fake_sdl.last_presentation = mode;
    fake_sdl.last_width = width;
    fake_sdl.last_height = height;
    return fake_sdl.logical_return;
}

void fake_SDL_DestroyRenderer(SDL_Renderer *renderer) {
    fake_sdl.destroy_renderer_calls++;
    fake_sdl.last_destroyed_renderer = renderer;
}

void fake_SDL_DestroyWindow(SDL_Window *window) {
    fake_sdl.destroy_window_calls++;
    fake_sdl.last_destroyed_window = window;
}

void fake_SDL_QuitSubSystem(SDL_InitFlags flags) {
    fake_sdl.quit_calls++;
    fake_sdl.last_quit_flags = flags;
}

void fake_SDL_Log(const char *fmt, ...) {
    va_list args;
    (void)fmt;
    va_start(args, fmt);
    va_end(args);
    fake_sdl.log_calls++;
}

const char *fake_SDL_GetError(void) { return "fake sdl error"; }

static void test_initialize_null_display(void) {
    reset_fake_sdl();
    assert(!display_initialize(NULL, "Title", 640, 480, 0, SDL_INIT_VIDEO,
                               (SDL_RendererLogicalPresentation)0));
    assert(fake_sdl.init_calls == 0);
    assert(fake_sdl.create_calls == 0);
    assert(fake_sdl.logical_calls == 0);
    assert(fake_sdl.quit_calls == 0);
    assert(fake_sdl.log_calls == 0);
}

static void test_initialize_init_failure(void) {
    Display display = {0};
    const char *title = "Title";
    const int width = 640;
    const int height = 480;
    const SDL_WindowFlags window_flags = 0;
    const SDL_InitFlags init_flags = SDL_INIT_VIDEO;
    const SDL_RendererLogicalPresentation mode =
        (SDL_RendererLogicalPresentation)0;

    reset_fake_sdl();
    fake_sdl.init_return = false;

    assert(!display_initialize(&display, title, width, height, window_flags,
                               init_flags, mode));
    assert(fake_sdl.init_calls == 1);
    assert(fake_sdl.create_calls == 0);
    assert(fake_sdl.logical_calls == 0);
    assert(fake_sdl.quit_calls == 0);
    assert(fake_sdl.log_calls == 1);
    assert(fake_sdl.last_init_flags == init_flags);
    assert(display.window == NULL);
    assert(display.renderer == NULL);
    assert(display.init_flags == init_flags);
}

static void test_initialize_create_failure(void) {
    Display display = {0};
    const char *title = "Title";
    const int width = 640;
    const int height = 480;
    const SDL_WindowFlags window_flags = SDL_WINDOW_FULLSCREEN;
    const SDL_InitFlags init_flags = SDL_INIT_VIDEO;
    const SDL_RendererLogicalPresentation mode =
        (SDL_RendererLogicalPresentation)1;

    reset_fake_sdl();
    fake_sdl.create_return = false;

    assert(!display_initialize(&display, title, width, height, window_flags,
                               init_flags, mode));
    assert(fake_sdl.init_calls == 1);
    assert(fake_sdl.create_calls == 1);
    assert(fake_sdl.logical_calls == 0);
    assert(fake_sdl.quit_calls == 1);
    assert(fake_sdl.log_calls == 1);
    assert(fake_sdl.last_quit_flags == SDL_INIT_VIDEO);
    assert_common_args(title, width, height, window_flags, init_flags);
    assert(display.window == NULL);
    assert(display.renderer == NULL);
}

static void test_initialize_logical_failure(void) {
    Display display = {0};
    const char *title = "Title";
    const int width = 640;
    const int height = 480;
    const SDL_WindowFlags window_flags = SDL_WINDOW_RESIZABLE;
    const SDL_InitFlags init_flags = SDL_INIT_VIDEO;
    const SDL_RendererLogicalPresentation mode =
        (SDL_RendererLogicalPresentation)2;

    reset_fake_sdl();
    fake_sdl.logical_return = false;

    assert(!display_initialize(&display, title, width, height, window_flags,
                               init_flags, mode));
    assert(fake_sdl.init_calls == 1);
    assert(fake_sdl.create_calls == 1);
    assert(fake_sdl.logical_calls == 1);
    assert(fake_sdl.destroy_renderer_calls == 1);
    assert(fake_sdl.destroy_window_calls == 1);
    assert(fake_sdl.quit_calls == 1);
    assert(fake_sdl.log_calls == 1);
    assert_common_args(title, width, height, window_flags, init_flags);
    assert_presentation(mode);
    assert(display.window == NULL);
    assert(display.renderer == NULL);
}

static void test_initialize_success(void) {
    Display display = {0};
    const char *title = "Title";
    const int width = 640;
    const int height = 480;
    const SDL_WindowFlags window_flags = SDL_WINDOW_MOUSE_GRABBED;
    const SDL_InitFlags init_flags = SDL_INIT_VIDEO;
    const SDL_RendererLogicalPresentation mode =
        (SDL_RendererLogicalPresentation)3;

    reset_fake_sdl();

    assert(display_initialize(&display, title, width, height, window_flags,
                              init_flags, mode));
    assert(fake_sdl.init_calls == 1);
    assert(fake_sdl.create_calls == 1);
    assert(fake_sdl.logical_calls == 1);
    assert(fake_sdl.destroy_renderer_calls == 0);
    assert(fake_sdl.destroy_window_calls == 0);
    assert(fake_sdl.quit_calls == 0);
    assert(fake_sdl.log_calls == 0);
    assert_common_args(title, width, height, window_flags, init_flags);
    assert_presentation(mode);
    assert(display.window == fake_sdl.created_window);
    assert(display.renderer == fake_sdl.created_renderer);
}

static void test_cleanup_null_display(void) {
    reset_fake_sdl();
    display_cleanup(NULL);
    assert(fake_sdl.destroy_renderer_calls == 0);
    assert(fake_sdl.destroy_window_calls == 0);
    assert(fake_sdl.quit_calls == 0);
}

static void test_cleanup_only_renderer(void) {
    Display display = {0};

    reset_fake_sdl();
    display.renderer = fake_sdl.created_renderer;
    display.init_flags = SDL_INIT_VIDEO;

    display_cleanup(&display);

    assert(fake_sdl.destroy_renderer_calls == 1);
    assert(fake_sdl.destroy_window_calls == 0);
    assert(fake_sdl.quit_calls == 1);
    assert(fake_sdl.last_quit_flags == SDL_INIT_VIDEO);
    assert(display.window == NULL);
    assert(display.renderer == NULL);
}

static void test_cleanup_only_window(void) {
    Display display = {0};

    reset_fake_sdl();
    display.window = fake_sdl.created_window;
    display.init_flags = SDL_INIT_VIDEO;

    display_cleanup(&display);

    assert(fake_sdl.destroy_renderer_calls == 0);
    assert(fake_sdl.destroy_window_calls == 1);
    assert(fake_sdl.quit_calls == 1);
    assert(fake_sdl.last_quit_flags == SDL_INIT_VIDEO);
    assert(display.window == NULL);
    assert(display.renderer == NULL);
}

static void test_cleanup_with_resources(void) {
    Display display = {0};

    reset_fake_sdl();
    display.window = fake_sdl.created_window;
    display.renderer = fake_sdl.created_renderer;
    display.init_flags = SDL_INIT_VIDEO;

    display_cleanup(&display);

    assert(fake_sdl.destroy_renderer_calls == 1);
    assert(fake_sdl.destroy_window_calls == 1);
    assert(fake_sdl.quit_calls == 1);
    assert(display.window == NULL);
    assert(display.renderer == NULL);
}

int main(void) {
    test_initialize_null_display();
    test_initialize_init_failure();
    test_initialize_create_failure();
    test_initialize_logical_failure();
    test_initialize_success();
    test_cleanup_null_display();
    test_cleanup_only_renderer();
    test_cleanup_only_window();
    test_cleanup_with_resources();
    return 0;
}
