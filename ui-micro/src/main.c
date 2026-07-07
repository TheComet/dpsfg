#include "csfg/init.h"
#include "csfg/util/mem.h"
#include "microui.h"
#include "ui/args.h"
#include "ui/db.h"
#include "ui/project_browser.h"
#include "ui/renderer.h"
#include <SDL2/SDL.h>

static float bg[3] = {90, 95, 100};

static int
uint8_slider(mu_Context* ctx, unsigned char* value, int low, int high)
{
    int rc;
    float tmp;
    mu_push_id(ctx, &value, sizeof(value));
    tmp    = *value;
    rc     = mu_slider_ex(ctx, &tmp, low, high, 0, "%.0f", MU_OPT_ALIGNCENTER);
    *value = tmp;
    mu_pop_id(ctx);
    return rc;
}

static void style_window(mu_Context* ctx)
{
    static struct
    {
        const char* label;
        int idx;
    } colors[] = {
        {       "text:",        MU_COLOR_TEXT},
        {     "border:",      MU_COLOR_BORDER},
        {   "windowbg:",    MU_COLOR_WINDOWBG},
        {    "titlebg:",     MU_COLOR_TITLEBG},
        {  "titletext:",   MU_COLOR_TITLETEXT},
        {    "panelbg:",     MU_COLOR_PANELBG},
        {     "button:",      MU_COLOR_BUTTON},
        {"buttonhover:", MU_COLOR_BUTTONHOVER},
        {"buttonfocus:", MU_COLOR_BUTTONFOCUS},
        {       "base:",        MU_COLOR_BASE},
        {  "basehover:",   MU_COLOR_BASEHOVER},
        {  "basefocus:",   MU_COLOR_BASEFOCUS},
        { "scrollbase:",  MU_COLOR_SCROLLBASE},
        {"scrollthumb:", MU_COLOR_SCROLLTHUMB},
        {          NULL,                    0}
    };

    if (mu_begin_window_ex(
            ctx,
            "Style Editor",
            mu_rect(0, 0, 0, 0),
            MU_OPT_NOCLOSE | MU_OPT_AUTOSIZE))
    {
        int i;
        int widths[6];
        int sw    = mu_get_current_container(ctx)->body.w * 0.14;
        widths[0] = 80;
        widths[1] = sw, widths[2] = sw, widths[3] = sw, widths[4] = sw;
        widths[5] = -1;
        mu_layout_row(ctx, 6, widths, 0);
        for (i = 0; colors[i].label; i++)
        {
            mu_label(ctx, colors[i].label);
            uint8_slider(ctx, &ctx->style->colors[i].r, 0, 255);
            uint8_slider(ctx, &ctx->style->colors[i].g, 0, 255);
            uint8_slider(ctx, &ctx->style->colors[i].b, 0, 255);
            uint8_slider(ctx, &ctx->style->colors[i].a, 0, 255);
            mu_draw_rect(ctx, mu_layout_next(ctx), ctx->style->colors[i]);
        }
        mu_end_window(ctx);
    }
}

static void
process_frame(mu_Context* mu, struct project_browser* project_browser)
{
    mu_begin(mu);
    style_window(mu);
    project_browser_window(project_browser, mu);
    mu_end(mu);
}

static char button_map(uint8_t sdl_button)
{
    switch (sdl_button)
    {
        case SDL_BUTTON_LEFT  : return MU_MOUSE_LEFT;
        case SDL_BUTTON_RIGHT : return MU_MOUSE_RIGHT;
        case SDL_BUTTON_MIDDLE: return MU_MOUSE_MIDDLE;
    }
    return 0;
}

static char key_map(int sdl_key)
{
    switch (sdl_key)
    {
        case SDLK_LSHIFT   : return MU_KEY_SHIFT;
        case SDLK_RSHIFT   : return MU_KEY_SHIFT;
        case SDLK_LCTRL    : return MU_KEY_CTRL;
        case SDLK_RCTRL    : return MU_KEY_CTRL;
        case SDLK_LALT     : return MU_KEY_ALT;
        case SDLK_RALT     : return MU_KEY_ALT;
        case SDLK_RETURN   : return MU_KEY_RETURN;
        case SDLK_BACKSPACE: return MU_KEY_BACKSPACE;
    }
    return 0;
}

static int text_width(mu_Font font, const char* text, int len)
{
    (void)font;
    if (len == -1)
        len = strlen(text);
    return r_get_text_width(text, len);
}

static int text_height(mu_Font font)
{
    (void)font;
    return r_get_text_height();
}

static int main_loop(mu_Context* ctx, struct project_browser* project_browser)
{
    mu_Command* cmd;
    SDL_Event e;

    while (1)
    {
        while (SDL_PollEvent(&e))
        {
            switch (e.type)
            {
                case SDL_QUIT: return EXIT_SUCCESS;
                case SDL_MOUSEMOTION:
                    mu_input_mousemove(ctx, e.motion.x, e.motion.y);
                    break;
                case SDL_MOUSEWHEEL:
                    mu_input_scroll(ctx, 0, e.wheel.y * -30);
                    break;
                case SDL_MOUSEBUTTONDOWN: {
                    int b = button_map(e.button.button);
                    if (b)
                        mu_input_mousedown(ctx, e.button.x, e.button.y, b);
                    break;
                }
                case SDL_MOUSEBUTTONUP: {
                    int b = button_map(e.button.button);
                    if (b)
                        mu_input_mouseup(ctx, e.button.x, e.button.y, b);
                    break;
                }
                case SDL_KEYDOWN: {
                    int c = key_map(e.key.keysym.sym);
                    if (e.key.keysym.sym == SDLK_ESCAPE)
                        return EXIT_SUCCESS;
                    if (c)
                        mu_input_keydown(ctx, c);
                    break;
                }
                case SDL_KEYUP: {
                    int c = key_map(e.key.keysym.sym);
                    if (c)
                        mu_input_keyup(ctx, c);
                    break;
                }
                case SDL_TEXTINPUT: mu_input_text(ctx, e.text.text); break;
                case SDL_WINDOWEVENT:
                if (e.window.event == SDL_WINDOWEVENT_RESIZED) {
                    r_resize(e.window.data1, e.window.data2);
                }
            }
        }

        process_frame(ctx, project_browser);

        r_clear(mu_color(bg[0], bg[1], bg[2], 255));
        cmd = NULL;
        while (mu_next_command(ctx, &cmd))
        {
            switch (cmd->type)
            {
                case MU_COMMAND_TEXT:
                    r_draw_text(cmd->text.str, cmd->text.pos, cmd->text.color);
                    break;
                case MU_COMMAND_RECT:
                    r_draw_rect(cmd->rect.rect, cmd->rect.color);
                    break;
                case MU_COMMAND_ICON:
                    r_draw_icon(cmd->icon.id, cmd->icon.rect, cmd->icon.color);
                    break;
                case MU_COMMAND_CLIP: r_set_clip_rect(cmd->clip.rect); break;
            }
        }
        r_present();
    }
}

int main(int argc, char* argv[])
{
    struct args args;
    mu_Context* ctx;
    const struct db_interface* dbi;
    struct db* db_ctx;
    struct project_browser project_browser;
    int status = EXIT_FAILURE;

    if (csfg_init() != 0)
        goto csfg_init_failed;

    switch (args_parse(&args, argc, argv))
    {
        case 0 : break;
        case 1 : return 0;
        default: goto parse_args_break;
    }

    if (args.tests)
    {
        int run_tests(int*, char*[]);
        status = run_tests(&argc, argv);
        goto parse_args_break;
    }

    if (db_init() != 0)
        goto db_init_failed;
    dbi    = db("sqlite3");
    db_ctx = dbi->open("projects.db");
    if (db_ctx == NULL)
        goto open_db_failed;
    if (dbi->upgrade(db_ctx) != 0)
        goto migrate_db_failed;

    if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
        goto init_sdl_failed;
    r_init();

    ctx = mem_alloc(sizeof(mu_Context));
    if (ctx == NULL)
        goto alloc_mu_failed;
    mu_init(ctx);
    ctx->text_width  = text_width;
    ctx->text_height = text_height;

    project_browser_init(&project_browser);
    project_browser_load_from_db(&project_browser, dbi, db_ctx);

    status = main_loop(ctx, &project_browser);

    project_browser_deinit(&project_browser);

    mem_free(ctx);
alloc_mu_failed:
    SDL_Quit();
init_sdl_failed:
migrate_db_failed:
    dbi->close(db_ctx);
open_db_failed:
    db_deinit();
db_init_failed:
parse_args_break:
    csfg_deinit();
csfg_init_failed:
    return status;
}
