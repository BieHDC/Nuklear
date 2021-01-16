/* Required by extra Examples */
#include <limits.h>
#include <time.h>

#include <SDL.h>
#include <SDL_mouse.h>
#include <SDL_keyboard.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_INCLUDE_SOFTWARE_FONT
#define NK_IMPLEMENTATION
#include "../../nuklear.h"
#define NK_SDLSURFACE_IMPLEMENTATION
#include "sdl2surface_rawfb.h"

/* ===============================================================
 *
 *                          EXAMPLE
 *
 * ===============================================================*/
/* This are some code examples to provide a small overview of what can be
 * done with this library. To try out an example uncomment the defines */
/*#define INCLUDE_ALL */
#define INCLUDE_STYLE
/*#define INCLUDE_CALCULATOR */
#define INCLUDE_OVERVIEW
/*#define INCLUDE_NODE_EDITOR */

#ifdef INCLUDE_ALL
  #define INCLUDE_STYLE
  #define INCLUDE_CALCULATOR
  #define INCLUDE_OVERVIEW
  #define INCLUDE_NODE_EDITOR
#endif

#ifdef INCLUDE_STYLE
  #include "../style.c"
#endif
#ifdef INCLUDE_CALCULATOR
  #include "../calculator.c"
#endif
#ifdef INCLUDE_OVERVIEW
  #include "../overview.c"
#endif
#ifdef INCLUDE_NODE_EDITOR
  #include "../node_editor.c"
#endif

/* ===============================================================
 *
 * HELPER UTILS
 *
 * ===============================================================*/
#define TIMER_INIT(x)   float ltime##x = SDL_GetTicks();  \
                        float ctime##x = SDL_GetTicks();

#define TIMER_GET(x, d) ctime##x = SDL_GetTicks();      \
                        d = ctime##x - ltime##x;        \
                        ltime##x = ctime##x;

/* Unlike TIMER_GET, this does not reset the timer */
#define TIMER_GET_ELAPSED(x, d) ctime##x = SDL_GetTicks();      \
                                d = ctime##x - ltime##x;

#define TIMER_RESET(x)  ctime##x = SDL_GetTicks();      \
                        ltime##x = ctime##x;

#define FPS_INIT(y)     float delaytime##y = 0;

#define FPS_LOCK_MAX(y, d, fps) if ((delaytime##y += (1000/(fps))-d) > 0)   \
                                    if (delaytime##y > 0)                   \
                                        SDL_Delay(delaytime##y);            \


/* updatecnt describes how often the fps should be updated   */
/* 1 -> after 1sec, 2 -> every half sec, 10 -> every 100msec */
#define FPS_COUNT_INIT(z, uc)   float cycles##z = 0;      \
                                unsigned int count##z = 0;       \
                                float updatecnt##z = uc;

/* You gotta call this in your main loop and it returns the current fps */
#define FPS_COUNT_TICK(z, d, fps)   cycles##z += d;                             \
                                    count##z++;                                 \
                                    if (cycles##z > 1000*(1/updatecnt##z)) {    \
                                        cycles##z -= 1000*(1/updatecnt##z);     \
                                        fps = count##z*updatecnt##z;            \
                                        count##z = 0;                           \
                                    }

/* This is used to simulate min "FPS", so instead of lag,   */
/* it runs the game logic "slower"                          */
#define DELTA_MIN_STEP(d, fps)  if (d > 1000/fps)   \
                                    d = 1000/fps;   \

/* ===============================================================
 *
 *                          DEMO
 *
 * ===============================================================*/
int main(int argc, char **argv)
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        SDL_Log("Failed to initialize SDL: %s", SDL_GetError());
        return 1;
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

    SDL_Window *window = SDL_CreateWindow("Demo", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_RESIZABLE);
    if (!window)
    {
        SDL_Log("Failed to open window: %s", SDL_GetError());
        return 2;
    }

    SDL_Surface* screen = SDL_GetWindowSurface(window);
    SDL_SetSurfaceBlendMode(screen, SDL_BLENDMODE_NONE);

    struct sdlsurface_context *context = nk_sdlsurface_init(800, 600);
    struct nk_context *ctx = &(context->ctx);

    nk_sdl_font_stash_begin(context);
    context->atlas.default_font = nk_font_atlas_add_default(&context->atlas, 13.0f, 0);
    nk_sdl_font_stash_end(context);

    /* FPS Counter and Limiter */
    float delta = 0;
    unsigned int fps = 0;
    TIMER_INIT(dcnt);
    FPS_INIT(fpscontrol);
    FPS_COUNT_INIT(fpsdisp, 2.0f);

    struct nk_colorf bg;
    bg.r = 0.10f, bg.g = 0.18f, bg.b = 0.24f, bg.a = 1.0f;
    for(;;)
    {
        static int lockfps = nk_false;

        /* Input */
        nk_input_begin(ctx);
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
                goto quit;
            if ((event.type == SDL_WINDOWEVENT) && (event.window.windowID == SDL_GetWindowID(window)))
            {   /* If we got a window size changed event, we need to recreate the surface! */
                if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                {
                    nk_sdlsurface_update(context, event.window.data1, event.window.data2);
                    screen = SDL_GetWindowSurface(window);
                    SDL_SetSurfaceBlendMode(screen, SDL_BLENDMODE_NONE);
                }
            }
            nk_sdlsurface_handle_event(ctx, &event);
        }
        nk_input_end(ctx);
        /* GUI */
        if (nk_begin(ctx, "Demo", nk_rect(50, 50, 230, 250),
            NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|
            NK_WINDOW_MINIMIZABLE|NK_WINDOW_TITLE))
        {
            enum {EASY, HARD};
            static int op = EASY;
            static int property = 20;

            nk_layout_row_static(ctx, 30, 80, 1);

            nk_checkbox_label(ctx, "Lock FPS", &lockfps);

            if (nk_button_label(ctx, "button"))
                fprintf(stdout, "button pressed\n");
            nk_layout_row_dynamic(ctx, 30, 2);
            if (nk_option_label(ctx, "easy", op == EASY)) op = EASY;
            if (nk_option_label(ctx, "hard", op == HARD)) op = HARD;
            nk_layout_row_dynamic(ctx, 25, 1);
            nk_property_int(ctx, "Compression:", 0, &property, 100, 10, 1);

            nk_layout_row_dynamic(ctx, 20, 1);
            nk_label(ctx, "background:", NK_TEXT_LEFT);
            nk_layout_row_dynamic(ctx, 25, 1);
            if (nk_combo_begin_color(ctx, nk_rgb_cf(bg), nk_vec2(nk_widget_width(ctx),400))) {
                nk_layout_row_dynamic(ctx, 120, 1);
                bg = nk_color_picker(ctx, bg, NK_RGBA);
                nk_layout_row_dynamic(ctx, 25, 1);
                bg.r = nk_propertyf(ctx, "#R:", 0, bg.r, 1.0f, 0.01f,0.005f);
                bg.g = nk_propertyf(ctx, "#G:", 0, bg.g, 1.0f, 0.01f,0.005f);
                bg.b = nk_propertyf(ctx, "#B:", 0, bg.b, 1.0f, 0.01f,0.005f);
                bg.a = nk_propertyf(ctx, "#A:", 0, bg.a, 1.0f, 0.01f,0.005f);
                nk_combo_end(ctx);
            }

            #ifdef INCLUDE_STYLE
                static enum theme theme_selected = THEME_BLACK;
                static enum theme theme_last = THEME_BLACK;
                nk_layout_row_dynamic(&(context->ctx), 0, 2);
                if (nk_option_label(&(context->ctx), "THEME_BLACK", theme_selected == THEME_BLACK)) theme_selected = THEME_BLACK;
                if (nk_option_label(&(context->ctx), "THEME_WHITE", theme_selected == THEME_WHITE)) theme_selected = THEME_WHITE;
                if (nk_option_label(&(context->ctx), "THEME_RED",   theme_selected == THEME_RED))   theme_selected = THEME_RED;
                if (nk_option_label(&(context->ctx), "THEME_BLUE",  theme_selected == THEME_BLUE))  theme_selected = THEME_BLUE;
                if (nk_option_label(&(context->ctx), "THEME_DARK",  theme_selected == THEME_DARK))  theme_selected = THEME_DARK;

                if (theme_selected != theme_last) {
                    theme_last = theme_selected;
                    set_style(ctx, theme_selected);
                }
            #endif
        }
        nk_end(ctx);

        /* -------------- EXAMPLES ---------------- */
        #ifdef INCLUDE_CALCULATOR
          calculator(ctx);
        #endif
        #ifdef INCLUDE_OVERVIEW
          overview(ctx);
        #endif
        #ifdef INCLUDE_NODE_EDITOR
          node_editor(ctx);
        #endif
        /* ----------------------------------------- */

        /* Draw */
        nk_sdlsurface_render(context, (const struct nk_color){bg.r*255, bg.g*255, bg.b*255, bg.a*255}, 1);
        SDL_BlitSurface(context->fb, NULL, screen, NULL);
        SDL_UpdateWindowSurface(window);

        /* Get the delta */
        TIMER_GET(dcnt, delta);
        /* Minimal FPS */
        DELTA_MIN_STEP(delta, 30.0f);
        /* Limit FPS */
        if (lockfps) {
            FPS_LOCK_MAX(fpscontrol, delta, 60.0f);
        }
        /* Get current FPS */
        FPS_COUNT_TICK(fpsdisp, delta, fps);
        SDL_Log("delta: %f\t--\tfps: %u\t", delta, fps);
    }

    quit:
    nk_sdlsurface_shutdown(context);
    screen = NULL;
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}


