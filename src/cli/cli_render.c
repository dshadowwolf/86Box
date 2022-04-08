/*
 * 86Box	A hypervisor and IBM PC system emulator that specializes in
 *		running old operating systems and software designed for IBM
 *		PC systems and compatibles from 1981 through fairly recent
 *		system designs based on the PCI bus.
 *
 *		This file is part of the 86Box distribution.
 *
 *		ANSI rendering module for the command line interface.
 *
 *
 *
 * Author:	RichardG, <richardg867@gmail.com>
 *
 *		Copyright 2021 RichardG.
 */
#include <math.h>
#define PNG_DEBUG 0

#include <png.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wchar.h>
#ifdef _WIN32
#    include <fcntl.h>
#    include <windows.h>
#else
#    include <unistd.h>
#endif
#define HAVE_STDARG_H
#include <86box/86box.h>
#include <86box/cli.h>
#include <86box/plat.h>
#include <86box/plat_dynld.h>
#include <86box/timer.h>
#include <86box/video.h>

#define APPEND_SGR()              \
    if (!sgr_started) {           \
        sgr_started = 1;          \
        p += sprintf(p, "\033["); \
    } else {                      \
        p += sprintf(p, ";");     \
    }

#ifdef _WIN32
#    define LIBSIXELDLLAPI __stdcall
#else
#    define LIBSIXELDLLAPI
#endif

#ifdef _WIN32
#    define PATH_LIBSIXEL_DLL     "libsixel.1.dll"
#    define PATH_LIBSIXEL_DLL_ALT "libsixel.dll"
#elif defined(__APPLE__)
#    define PATH_LIBSIXEL_DLL     "libsixel.1.dylib"
#    define PATH_LIBSIXEL_DLL_ALT "libsixel.dylib"
#else
#    define PATH_LIBSIXEL_DLL     "libsixel.so.1"
#    define PATH_LIBSIXEL_DLL_ALT "libsixel.so"
#endif

enum {
    CLI_RENDER_BLANK = 0x00,
    CLI_RENDER_GFX   = 0x01,
    CLI_RENDER_CGA   = 0x10,
    CLI_RENDER_MDA   = 0x11,
};

typedef struct {
    uint16_t framebuffer[CLI_RENDER_FB_SIZE];
    char     buffer[CLI_RENDER_ANSIBUF_SIZE];
    uint8_t  invalidate, full_width, do_render, do_blink;
} cli_render_line_t;

typedef struct _cli_render_png_ {
    uint8_t                  buffer[3072];
    uint16_t                 size;
    struct _cli_render_png_ *next;
} cli_render_png_t;

typedef struct _cli_render_sixel_ {
    uint8_t set, render, sixmap[CLI_RENDER_GFXBUF_W];
    union {
        uint32_t rgb;
        uint8_t  r_g_b_f[4];
    };
} cli_render_sixel_t;

/* Lookup table for converting CGA colors to the ANSI palette. */
const uint8_t cga_ansi_palette[] = {
    0, 4, 2, 6, 1, 5, 3, 7,      /* regular */
    8, 12, 10, 14, 9, 13, 11, 15 /* bright */
};

/* Lookup table for converting code page 437 to UTF-8. */
static const char *cp437[] = {
    /* 00 */ " ", "\xE2\x98\xBA", "\xE2\x98\xBB", "\xE2\x99\xA5", "\xE2\x99\xA6", "\xE2\x99\xA3", "\xE2\x99\xA0", "\xE2\x80\xA2", "\xE2\x97\x98", "\xE2\x97\x8B", "\xE2\x97\x99", "\xE2\x99\x82", "\xE2\x99\x80", "\xE2\x99\xAA", "\xE2\x99\xAB", "\xE2\x98\xBC",
    /* 10 */ "\xE2\x96\xBA", "\xE2\x97\x84", "\xE2\x86\x95", "\xE2\x80\xBC", "\xC2\xB6", "\xC2\xA7", "\xE2\x96\xAC", "\xE2\x86\xA8", "\xE2\x86\x91", "\xE2\x86\x93", "\xE2\x86\x92", "\xE2\x86\x90", "\xE2\x88\x9F", "\xE2\x86\x94", "\xE2\x96\xB2", "\xE2\x96\xBC",
    /* 20 */ " ", "!", "\"", "#", "$", "%", "&", "'", "(", ")", "*", "+", ",", "-", ".", "/",
    /* 30 */ "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", ":", ";", "<", "=", ">", "?",
    /* 40 */ "@", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O",
    /* 50 */ "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "[", "\\", "]", "^", "_",
    /* 60 */ "`", "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o",
    /* 70 */ "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z", "{", "\xC2\xA6", "}", "~", "\xE2\x8C\x82",
    /* 80 */ "\xC3\x87", "\xC3\xBC", "\xC3\xA9", "\xC3\xA2", "\xC3\xA4", "\xC3\xA0", "\xC3\xA5", "\xC3\xA7", "\xC3\xAA", "\xC3\xAB", "\xC3\xA8", "\xC3\xAF", "\xC3\xAE", "\xC3\xAC", "\xC3\x84", "\xC3\x85",
    /* 90 */ "\xC3\x89", "\xC3\xA6", "\xC3\x86", "\xC3\xB4", "\xC3\xB6", "\xC3\xB2", "\xC3\xBB", "\xC3\xB9", "\xC3\xBF", "\xC3\x96", "\xC3\x9C", "\xC2\xA2", "\xC2\xA3", "\xC2\xA5", "\xE2\x82\xA7", "\xC6\x92",
    /* A0 */ "\xC3\xA1", "\xC3\xAD", "\xC3\xB3", "\xC3\xBA", "\xC3\xB1", "\xC3\x91", "\xC2\xAA", "\xC2\xBA", "\xC2\xBF", "\xE2\x8C\x90", "\xC2\xAC", "\xC2\xBD", "\xC2\xBC", "\xC2\xA1", "\xC2\xAB", "\xC2\xBB",
    /* B0 */ "\xE2\x96\x91", "\xE2\x96\x92", "\xE2\x96\x93", "\xE2\x94\x82", "\xE2\x94\xA4", "\xE2\x95\xA1", "\xE2\x95\xA2", "\xE2\x95\x96", "\xE2\x95\x95", "\xE2\x95\xA3", "\xE2\x95\x91", "\xE2\x95\x97", "\xE2\x95\x9D", "\xE2\x95\x9C", "\xE2\x95\x9B", "\xE2\x94\x90",
    /* C0 */ "\xE2\x94\x94", "\xE2\x94\xB4", "\xE2\x94\xAC", "\xE2\x94\x9C", "\xE2\x94\x80", "\xE2\x94\xBC", "\xE2\x95\x9E", "\xE2\x95\x9F", "\xE2\x95\x9A", "\xE2\x95\x94", "\xE2\x95\xA9", "\xE2\x95\xA6", "\xE2\x95\xA0", "\xE2\x95\x90", "\xE2\x95\xAC", "\xE2\x95\xA7",
    /* D0 */ "\xE2\x95\xA8", "\xE2\x95\xA4", "\xE2\x95\xA5", "\xE2\x95\x99", "\xE2\x95\x98", "\xE2\x95\x92", "\xE2\x95\x93", "\xE2\x95\xAB", "\xE2\x95\xAA", "\xE2\x94\x98", "\xE2\x94\x8C", "\xE2\x96\x88", "\xE2\x96\x84", "\xE2\x96\x8C", "\xE2\x96\x90", "\xE2\x96\x80",
    /* E0 */ "\xCE\xB1", "\xC3\x9F", "\xCE\x93", "\xCF\x80", "\xCE\xA3", "\xCF\x83", "\xC2\xB5", "\xCF\x84", "\xCE\xA6", "\xCE\x98", "\xCE\xA9", "\xCE\xB4", "\xE2\x88\x9E", "\xCF\x86", "\xCE\xB5", "\xE2\x88\xA9",
    /* F0 */ "\xE2\x89\xA1", "\xC2\xB1", "\xE2\x89\xA5", "\xE2\x89\xA4", "\xE2\x8C\xA0", "\xE2\x8C\xA1", "\xC3\xB7", "\xE2\x89\x88", "\xC2\xB0", "\xE2\x88\x99", "\xC2\xB7", "\xE2\x88\x9A", "\xE2\x81\xBF", "\xC2\xB2", "\xE2\x96\xA0", "\xC2\xA0"
};

/* Fallback ASCII-only code page 437 character set for non-UTF-8 terminals.
   On values >= 0x80, DEC Special Graphics should be used on the lower 7 bits. */
static const char cp437_fallback[] = {
    /* 00 */ ' ', 'o', 'o', 'o', 0xe0, '^', '^', '.', 'o', 'o', 'o', 'M', 'F', '8', '8', 'o',
    /* 10 */ '>', '<', '|', '!', 'P', 'S', '-', '|', '^', 'v', '>', '<', 'L', '-', '^', 'v',
    /* 20 */ ' ', '!', '"', '#', '$', '%', '&', '\'', '(', ')', '*', '+', ',', '-', '.', '/',
    /* 30 */ '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ':', ';', '<', '=', '>', '?',
    /* 40 */ '@', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
    /* 50 */ 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '[', '\\', ']', '^', '_',
    /* 60 */ '`', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
    /* 70 */ 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '{', '|', '}', '~', 'D',
    /* 80 */ 'C', 'u', 'e', 'a', 'a', 'a', 'a', 'c', 'e', 'e', 'e', 'i', 'i', 'i', 'A', 'A',
    /* 90 */ 'E', 'e', 'E', 'o', 'o', 'o', 'u', 'u', 'y', 'O', 'U', 'c', 0xfd, 'Y', 'P', 'f',
    /* A0 */ 'a', 'i', 'o', 'u', 'n', 'N', 'a', 0xe6, '?', '-', '-', '2', '4', '!', '<', '>',
    /* B0 */ 0xe8, 0xe1, 0xe1, 0xf8, 0xf5, 0xf5, 0xf5, 0xeb, 0xeb, 0xf5, 0xf8, 0xeb, 0xea, 0xea, 0xea, 0xeb,
    /* C0 */ 0xed, 0xf6, 0xf7, 0xf4, 0xf1, 0xee, 0xf4, 0xf4, 0xed, 0xec, 0xf6, 0xf7, 0xf4, 0xf1, 0xee, 0xf6,
    /* D0 */ 0xf6, 0xf7, 0xf7, 0xed, 0xed, 0xec, 0xec, 0xee, 0xee, 0xea, 0xec, 0xb0, 0xf3, '#', '#', 0xef,
    /* E0 */ 'a', 's', 'c', 0xfb, 'S', 's', 'm', 't', 'W', 'O', 'O', 'd', '8', 'p', 'e', '^',
    /* F0 */ '=', 0xe7, 0xfa, 0xf9, '|', '|', '/', '~', 0xe6, 0xfe, 0xfe, 'v', 'n', '2', '#', ' '
};

/* Lookup table for encoding images as base64. */
static const char base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/* libsixel dynamic loading imports. */
static void *(LIBSIXELDLLAPI *sixel_dither_get)(int builtin_dither);
static int(LIBSIXELDLLAPI *sixel_output_new)(void **output,
                                             int (*fn_write)(char *data, int size, void *priv),
                                             void *priv, void *allocator);
static void(LIBSIXELDLLAPI *sixel_output_destroy)(void *output);
static int(LIBSIXELDLLAPI *sixel_encode)(unsigned char *pixels, int width, int height,
                                         int depth, void *dither, void *context);
static int(LIBSIXELDLLAPI *sixel_encoder_encode)(void *encoder, const char *filename);

static dllimp_t libsixel_imports[] = {
    {"sixel_dither_get",      &sixel_dither_get    },
    { "sixel_output_new",     &sixel_output_new    },
    { "sixel_output_destroy", &sixel_output_destroy},
    { "sixel_encode",         &sixel_encode        },
    { "sixel_encoder_encode", &sixel_encoder_encode},
    { NULL,                   NULL                 }
};
static void *libsixel_handle = NULL,
            *libsixel_dither = NULL, *libsixel_output = NULL;

static char    infobox[256];
static uint8_t palette_4bit[16], palette_8bit[16],
    cursor_x = -1, cursor_y = -1;
static uint32_t            colors_8bit[256], palette_24bit[16];
static time_t              gfx_last = 0;
static int                 png_size = 0;
static cli_render_png_t   *png_first, *png_current;
static cli_render_sixel_t *sixel_colors = NULL;
static cli_render_line_t  *lines[CLI_RENDER_MAX_LINES];
static struct {
    thread_t *thread;
    event_t  *wake_render_thread, *render_complete;

    uint8_t mode, block, invalidate_all;

    uint8_t *fb, prev_mode, y, rowcount, prev_rowcount,
        do_render, do_blink, con;
    uint16_t ca;
    uint32_t fb_base, fb_mask, fb_step;
    int      xlimit, xinc;

    uint8_t   *blit_fb;
    png_bytep *blit_lines;
    int        blit_sx, blit_sy;

    uint8_t has_sideband;
    char    sideband_slots[RENDER_SIDEBAND_MAX][32];
    wchar_t title[200];

    char *infobox;
    int   infobox_sx, infobox_sy;
} render_data = { .prev_mode = -1, .y = CLI_RENDER_MAX_LINES + 1, .blit_sx = -1, .blit_sy = -1 };

#define ENABLE_CLI_RENDER_LOG 1
#ifdef ENABLE_CLI_RENDER_LOG
int cli_render_do_log = ENABLE_CLI_RENDER_LOG;

static void
cli_render_log(const char *fmt, ...)
{
    va_list ap;

    if (cli_render_do_log) {
        va_start(ap, fmt);
        pclog_ex(fmt, ap);
        va_end(ap);
    }
}
#else
#    define cli_render_log(fmt, ...)
#endif

void
cli_render_blank()
{
    if (render_data.block)
        return;
    thread_wait_event(render_data.render_complete, -1);
    thread_reset_event(render_data.render_complete);

    render_data.mode = CLI_RENDER_BLANK;

    thread_set_event(render_data.wake_render_thread);
}

int gx = 0;
void
cli_render_gfx(char *str)
{
    if (render_data.block)
        return;

    /* Perform an image render if this terminal supports graphics. */
    if (cli_term.gfx_level) {
        /* Initialize stuff if this mode was just switched into. */
        if (!cli_blit) {
            /* Tell video.c to start blitting to the image rendering buffer. */
            cli_blit = 1;

            /* Render on the first opportunity. */
            gfx_last = 0;
        }

        /* Render image if we have valid data. */
        if (cli_blit == 2) {
            if (render_data.mode != CLI_RENDER_GFX) {
                thread_wait_event(render_data.render_complete, -1);
                thread_reset_event(render_data.render_complete);
            }

            render_data.mode = CLI_RENDER_GFX;

            thread_set_event(render_data.wake_render_thread);
        }

        return;
    }

    /* Render infobox otherwise. */
    cli_render_gfx_box(str);
}

void
cli_render_gfx_blit(bitmap_t *bitmap, int x, int y, int w, int h)
{
    /* Don't overflow the image rendering buffer. */
    if (w >= CLI_RENDER_GFXBUF_W)
        w = CLI_RENDER_GFXBUF_W;
    if (h >= CLI_RENDER_GFXBUF_H)
        h = CLI_RENDER_GFXBUF_H;

    /* Allocate image rendering buffer and line pointer array if required. */
    if (!render_data.blit_fb)
        render_data.blit_fb = malloc(CLI_RENDER_GFXBUF_W * CLI_RENDER_GFXBUF_H * 3);
    if (!render_data.blit_lines)
        render_data.blit_lines = malloc(sizeof(png_bytep) * CLI_RENDER_GFXBUF_H);

    /* Blit lines to the image rendering buffer. */
    uint8_t  *p = render_data.blit_fb;
    uint32_t *q, temp;
    for (int dy = 0; dy < h; dy++) {
        /* Update line pointer array. */
        render_data.blit_lines[dy] = p;

        /* Blit line. */
        q = &bitmap->line[y++][x];
        for (int dx = 0; dx < w; dx++) {
            temp = *q++;
            *p++ = (temp >> 16) & 0xff;
            *p++ = (temp >> 8) & 0xff;
            *p++ = temp & 0xff;
        }
    }

    /* Set image render parameters. */
    render_data.blit_sx = w;
    render_data.blit_sy = h;

    /* Tell the main thread we have valid image data. */
    cli_blit = 2;
}

void
cli_render_gfx_box(char *str)
{
    if (render_data.block)
        return;
    thread_wait_event(render_data.render_complete, -1);
    thread_reset_event(render_data.render_complete);

    render_data.mode = CLI_RENDER_BLANK;

    render_data.infobox_sx = get_actual_size_x();
    render_data.infobox_sy = get_actual_size_y();
    strncpy(infobox, str, sizeof(infobox));
    infobox[sizeof(infobox) - 1] = '\0';
    render_data.infobox          = infobox;

    thread_set_event(render_data.wake_render_thread);
}

void
cli_render_cga(uint8_t y, uint8_t rowcount,
               int xlimit, int xinc,
               uint8_t *fb, uint32_t fb_base, uint32_t fb_mask, uint8_t fb_step,
               uint8_t do_render, uint8_t do_blink,
               uint32_t ca, uint8_t con)
{
    if (render_data.block)
        return;
    thread_wait_event(render_data.render_complete, -1);
    thread_reset_event(render_data.render_complete);

    render_data.mode      = CLI_RENDER_CGA;
    render_data.rowcount  = rowcount;
    render_data.xlimit    = xlimit;
    render_data.xinc      = xinc;
    render_data.fb        = fb;
    render_data.fb_base   = fb_base;
    render_data.fb_mask   = fb_mask;
    render_data.fb_step   = fb_step;
    render_data.do_render = do_render;
    render_data.do_blink  = do_blink;
    render_data.ca        = ca;
    render_data.con       = con;
    render_data.y         = y;

    thread_set_event(render_data.wake_render_thread);
}

void
cli_render_mda(int xlimit, uint8_t rowcount,
               uint8_t *fb, uint16_t fb_base,
               uint8_t do_render, uint8_t do_blink,
               uint16_t ca, uint8_t con)
{
    if (render_data.block)
        return;
    thread_wait_event(render_data.render_complete, -1);
    thread_reset_event(render_data.render_complete);

    render_data.mode      = CLI_RENDER_MDA;
    render_data.rowcount  = rowcount;
    render_data.xlimit    = xlimit;
    render_data.xinc      = 1;
    render_data.fb        = fb;
    render_data.fb_base   = fb_base;
    render_data.fb_mask   = 0xfff;
    render_data.fb_step   = 1;
    render_data.do_render = do_render;
    render_data.do_blink  = do_blink;
    render_data.ca        = ca;
    render_data.con       = con;
    render_data.y         = fb_base / xlimit;

    thread_set_event(render_data.wake_render_thread);
}

void
cli_render_write(int slot, char *s)
{
    /* Copy string to the specified slot. */
    int len                               = strlen(s);
    len                                   = MIN(len, sizeof(render_data.sideband_slots[slot]) - 1);
    render_data.sideband_slots[slot][len] = '\0'; /* avoid potential race conditions leading to unbounded strings */
    strncpy(render_data.sideband_slots[slot], s, len);

    /* Mark that we have side-band data on this slot. */
    render_data.has_sideband |= 1 << slot;

    thread_set_event(render_data.wake_render_thread);
}

void
cli_render_write_title(wchar_t *s)
{
    /* Copy title. */
    int len                = wcslen(s);
    len                    = MIN(len, sizeof(render_data.title) - 1);
    render_data.title[len] = '\0'; /* avoid potential race conditions leading to unbounded strings */
    wcsncpy(render_data.title, s, len);

    thread_set_event(render_data.wake_render_thread);
}

void
cli_render_monitorenter()
{
    thread_wait_event(render_data.render_complete, -1);
    thread_reset_event(render_data.render_complete);

    /* Block any further rendering. */
    render_data.block = 1;

    /* Set up terminal:
       - Reset formatting
       - Move cursor to top left corner
       - Clear screen
       - Show cursor
       - Switch to xterm's Main Screen Buffer */
    cursor_x = cursor_y = -1;
    fputs("\033[0m\033[1;1H\033[2J\033[3J\033[?25h\033[?1049l", CLI_RENDER_OUTPUT);

    thread_set_event(render_data.wake_render_thread);
    thread_wait_event(render_data.render_complete, -1); /* avoid race conditions */
}

void
cli_render_monitorexit()
{
    /* Set up terminal:
       - Switch to Alternate Screen Buffer
       - Enable ESC on Meta
       - Set cursor style to blinking underline */
    fputs("\033[?1049h\033[?1036h\033[3 q", CLI_RENDER_OUTPUT);

    /* Set terminal encoding to UTF-8. */
#ifdef _WIN32
    SetConsoleOutputCP(65001);
#else
    fputs("\033[%G", CLI_RENDER_OUTPUT);
#endif

    /* Clear and re-render the entire screen on the next rendering run. */
    render_data.invalidate_all = 1;

    /* If image rendering mode is currently active, then re-render on the first opportunity. */
    gfx_last = 0;

    /* Resume rendering. */
    render_data.block = 0;
}

static cli_render_line_t *
cli_render_getline(uint8_t y)
{
    /* Don't overflow the buffer. */
    if (y >= CLI_RENDER_MAX_LINES)
        return NULL;

    /* Allocate new structure if required. */
    cli_render_line_t *ret = lines[y];
    if (!ret) {
        ret = lines[y] = malloc(sizeof(cli_render_line_t));
        memset(ret, 0, sizeof(cli_render_line_t));
        ret->invalidate = 1;
    }

    return ret;
}

int
cli_render_setcolor_none(char *p, uint8_t index, uint8_t is_background)
{
    /* No color support. Most we can do is increase the intensity on a bright foreground.
       Use 75 (not sub/superscript) as a dummy SGR on background set operations to prevent a
       blank SGR string (;;) from resetting the foreground, due to the way APPEND_SGR works. */
    return sprintf(p, "%d", is_background ? 75 : ((index & 8) ? 1 : 22));
}

static int
cli_render_setcolor_3bit(char *p, uint8_t index, uint8_t is_background)
{
    /* Set an approximated color from the 3-color palette. */
    return sprintf(p, "%d", (is_background ? 40 : 30) + (palette_4bit[index] & 7));
}

static int
cli_render_setcolor_4bit(char *p, uint8_t index, uint8_t is_background)
{
    /* Set an approximated color from the 4-color palette. */
    uint8_t approx   = palette_4bit[index],
            pre_attr = 0, sgr = (is_background ? 40 : 30) + (approx & 7);

    if (approx & 8) {
        if (is_background) {
            pre_attr = sgr;
            sgr += 60; /* bright background: use non-standard SGR */
        } else {
            pre_attr = 1; /* bright foreground: increase intensity */
        }
    } else if (!is_background) {
        pre_attr = 22; /* regular foreground: decrease intensity */
    }

    if (pre_attr)
        return sprintf(p, "%d;%d", pre_attr, sgr);
    else
        return sprintf(p, "%d", sgr);
}

static int
cli_render_setcolor_8bit(char *p, uint8_t index, uint8_t is_background)
{
    /* Set an approximated color from the 256-color palette. */
    uint8_t approx = palette_8bit[index];
    if (approx < 8) /* save a little bit of bandwidth by using standard SGRs on 8-color palette colors */
        return sprintf(p, "%d", (is_background ? 40 : 30) + approx);
    else
        return sprintf(p, "%d;5;%d", is_background ? 48 : 38, approx);
}

static int
cli_render_setcolor_24bit(char *p, uint8_t index, uint8_t is_background)
{
    /* Set a full RGB color. */
    uint32_t color = palette_24bit[index];
    return sprintf(p, "%d;2;%d;%d;%d",
                   is_background ? 48 : 38,
                   (color >> 16) & 0xff,
                   (color >> 8) & 0xff,
                   color & 0xff);
}

void
cli_render_setcolorlevel()
{
    /* Set color functions and establish libsixel dither level. */
    int libsixel_dither_level;
    switch (cli_term.color_level) {
        case TERM_COLOR_3BIT:
            cli_term.setcolor     = cli_render_setcolor_3bit;
            libsixel_dither_level = 0x2; /* SIXEL_BUILTIN_XTERM16 */
            break;

        case TERM_COLOR_4BIT:
            cli_term.setcolor     = cli_render_setcolor_4bit;
            libsixel_dither_level = 0x2; /* SIXEL_BUILTIN_XTERM16 */
            break;

        case TERM_COLOR_8BIT:
            cli_term.setcolor     = cli_render_setcolor_8bit;
            libsixel_dither_level = 0x3; /* SIXEL_BUILTIN_XTERM256 */
            break;

        case TERM_COLOR_24BIT:
            cli_term.setcolor     = cli_render_setcolor_24bit;
            libsixel_dither_level = 0x3; /* SIXEL_BUILTIN_XTERM256 */
            break;

        default:
            cli_term.setcolor     = cli_render_setcolor_none;
            libsixel_dither_level = 0x0; /* SIXEL_BUILTIN_MONO_DARK */
            break;
    }

    /* Get libsixel dither object. */
    if (sixel_dither_get)
        libsixel_dither = sixel_dither_get(libsixel_dither_level);
}

void
cli_render_findclosest(uint32_t color, uint8_t *best_4bit, uint8_t *best_8bit)
{
    uint8_t  exact, rdif, gdif, bdif;
    uint32_t palette_color;
    double   candidate, best = INFINITY;

    /* Look through 16- and 256-color palettes for the closest color to the desired one. */
    for (int i = 0; i < 256; i++) {
        /* Get palette color. */
        palette_color = colors_8bit[i];

        /* Measure color distance. */
        exact = palette_color == color;
        if (exact) {
            /* Force best candidate if this is an exact match. */
            candidate = -INFINITY;
        } else {
            /* Controversial formula, but good enough? */
            rdif      = ((palette_color >> 16) & 0xff) - ((color >> 16) & 0xff);
            gdif      = ((palette_color >> 8) & 0xff) - ((color >> 8) & 0xff);
            bdif      = (palette_color & 0xff) - (color & 0xff);
            candidate = sqrt((rdif * rdif) + (gdif * gdif) + (bdif * bdif));
        }

        /* Check if this is the best candidate so far. */
        if (candidate < best) {
            /* Mark this as the best candidate for each applicable palette. */
            best = candidate;
            if (i < 16)
                *best_4bit = i;
            *best_8bit = i;

            /* Stop if we've found an exact match. */
            if (exact)
                break;
        }
    }
}

void
cli_render_setpal(uint8_t index, uint32_t color)
{
    /* Don't re-calculate if the color hasn't changed. */
    if (palette_24bit[index] == color)
        return;

    /* Look for the closest match. */
    cli_render_findclosest(color, &palette_4bit[index], &palette_8bit[index]);
    palette_24bit[index] = color;

    cli_render_log("CLI Render: setpal(%d, %06X) = %d/%d\n",
                   index, color, palette_4bit[index], palette_8bit[index]);
}

static char *
cli_render_clearbg(char *p)
{
    /* Use the black color for this terminal if applicable. */
    p += sprintf(p, "\033[0;");
    int i = 0;
    if ((render_data.mode < 0x10) || !(i = cli_term.setcolor(p, 0, 1)))
        *--p = '\0';
    p += i;
    p += sprintf(p, "m");
    return p;
}

static void
cli_render_updateline(char *buf, uint8_t y, uint8_t full_width, uint8_t new_cx, uint8_t new_cy)
{
    /* Get line. */
    cli_render_line_t *line = cli_render_getline(y);

    /* Update line if required and within the terminal's limit. */
    if ((y < cli_term.size_y) && buf && ((buf == line->buffer) || strcmp(buf, line->buffer))) {
        /* Copy line to buffer. */
        if (buf && (buf != line->buffer))
            strcpy(line->buffer, buf);

        /* Move to line, reset formatting and clear line if required. */
        char sgr[256], *p = sgr;
        p += sprintf(p, "\033[%d;1H", y + 1);
        p                = cli_render_clearbg(p);
        line->full_width = full_width;
        if (!full_width)
            strcpy(p, "\033[2K");
        fputs(sgr, CLI_RENDER_OUTPUT);

        /* Print line. */
        fputs(line->buffer, CLI_RENDER_OUTPUT);
        fputs("\033[0m", CLI_RENDER_OUTPUT);

        /* Force cursor update. */
        cursor_x = ~new_cx;
    }

    /* Update cursor if required. */
    if ((new_cx != cursor_x) || (new_cy != cursor_y)) {
        cursor_x = new_cx;
        cursor_y = new_cy;

        if ((cursor_x >= cli_term.size_x) || (cursor_y >= cli_term.size_y)) /* cursor disabled */
            fputs("\033[?25l", CLI_RENDER_OUTPUT);
        else /* cursor enabled */
            fprintf(CLI_RENDER_OUTPUT, "\033[%d;%dH\033[?25h", cursor_y + 1, cursor_x + 1);
    }

    /* Flush output. */
    fflush(CLI_RENDER_OUTPUT);
}

void
cli_render_updatescreen()
{
    /* Invalidate all lines. */
    render_data.invalidate_all = 1;
}

void
cli_render_process_base64(uint8_t *buf, int len)
{
    char output_buf[257], *p = output_buf,
                          *limit = output_buf + (sizeof(output_buf) - 1);
    register uint32_t tri;
    while (len > 0) {
        tri = buf[0] << 16;
        if (len >= 2) {
            tri |= buf[1] << 8;
            if (len >= 3)
                tri |= buf[2];
        }
        *p++ = base64[tri >> 18];
        *p++ = base64[(tri >> 12) & 0x3f];
        *p++ = (len == 1) ? '=' : base64[(tri >> 6) & 0x3f],
        *p++ = (len <= 2) ? '=' : base64[tri & 0x3f];
        if (p == limit) {
            *p = '\0';
            fputs(output_buf, CLI_RENDER_OUTPUT);
            p = output_buf;
        }
        len -= 3;
        buf += 3;
    }
    *p = '\0';
    fputs(output_buf, CLI_RENDER_OUTPUT);
}

static void
cli_render_process_pngwrite(png_structp png_ptr, png_bytep data, size_t length)
{
    png_size += length;

    /* Append data to current buffer if there's room. */
    int chunk_len = sizeof(png_current->buffer) - png_current->size;
    if (chunk_len) {
        if (length < chunk_len)
            chunk_len = length;
        memcpy(png_current->buffer + png_current->size, data, chunk_len);
        png_current->size += chunk_len;
        data += chunk_len;
        length -= chunk_len;
    }

    /* Output any remaining data to new buffer(s). */
    while (length > 0) {
        png_current->next = malloc(sizeof(cli_render_png_t));
        png_current       = png_current->next;
        png_current->next = NULL;

        chunk_len = MIN(length, sizeof(png_current->buffer));
        memcpy(png_current->buffer, data, chunk_len);
        png_current->size = chunk_len;
        data += chunk_len;
        length -= chunk_len;
    }
}

static void
cli_render_process_pngflush(png_structp png_ptr)
{
}

static int
cli_render_process_sixelwrite(char *data, int size, void *priv)
{
    fwrite(data, size, 1, priv);
    return 0;
}

static void
cli_render_process_sixel(uint8_t *fb, int sx, int sy)
{
    char     ch, prev_ch;
    uint8_t *p = fb;
    union {
        uint32_t rgb;
        uint8_t  r_g_b_f[4];
    } color = { .rgb = 0 };
    int                 i, j, x, y;
    cli_render_sixel_t *color_entry;

    /* Render using libsixel instead if available. */
    if (libsixel_dither) {
        j = sixel_encode(fb, sx, sy, 24, libsixel_dither, libsixel_output);
        if (j)
            cli_render_log("CLI Render: libsixel encode failed (%04X)\n", j);
        else
            return;
    }

    /* Initialize palette array on the first use of sixel rendering. */
    if (!sixel_colors) {
        sixel_colors = malloc(sizeof(cli_render_sixel_t) * 1024);
        for (j = 0; j < 1024; j++) {
            color_entry = &sixel_colors[j];
            /* Initialize the grayscale palette as well. */
            if (j < 101)
                color_entry->r_g_b_f[0] = color_entry->r_g_b_f[1] = color_entry->r_g_b_f[2] = j;
            color_entry->render = 0;
            memset(color_entry->sixmap, 0, sizeof(color_entry->sixmap));
        }
    }

    /* Reset color state while clearing the color palette. */
    for (j = 0; j < 1024; j++) {
        color_entry             = &sixel_colors[j];
        color_entry->set        = 0;
        color_entry->r_g_b_f[3] = (j >= 101) << 7;
    }

    /* Start sixel output. */
    fputs("\033Pq", CLI_RENDER_OUTPUT);

    /* Render each sixel row, which corresponds to 6 pixel rows. */
    for (y = 0; (y < sy) && (y < (CLI_RENDER_GFXBUF_H - 6)); y += 6) {
        /* Go through columns on this sixel row, building the sixmap for each color. */
        for (i = 0; i < 6; i++) {
            for (x = 0; x < sx; x++) {
                /* Convert color to sixel scale. */
                color.r_g_b_f[0] = *p++ / 2.55;
                color.r_g_b_f[1] = *p++ / 2.55;
                color.r_g_b_f[2] = *p++ / 2.55;

                /* Check grayscale palette. */
                if ((color.r_g_b_f[0] == color.r_g_b_f[1]) && (color.r_g_b_f[0] == color.r_g_b_f[2])) {
                    color_entry = &sixel_colors[color.r_g_b_f[0]];
                    goto have_color;
                }

                /* Check color palette. */
                for (j = 101; j < 1024; j++) {
                    color_entry = &sixel_colors[j];
                    if (color_entry->rgb == color.rgb) {
                        goto have_color;
                    } else if (color_entry->rgb & 0x80000000) {
                        /* This palette entry is up for grabs. */
                        color_entry->rgb = color.rgb;
                        goto have_color;
                    }
                }

                /* Convert to grayscale. */
                if (video_graytype) {
                    if (video_graytype == 1)
                        j = ((54 * (uint32_t) * (p - 1)) + (183 * (uint32_t) * (p - 2)) + (18 * (uint32_t) * (p - 3))) / 650.25;
                    else
                        j = ((uint32_t) * (p - 1) + (uint32_t) * (p - 2) + (uint32_t) * (p - 3)) / 7.65;
                } else {
                    j = ((76 * (uint32_t) * (p - 1)) + (150 * (uint32_t) * (p - 2)) + (29 * (uint32_t) * (p - 3))) / 650.25;
                }
                color_entry = &sixel_colors[j];

                /* Set bit in sixmap, and mark this color for rendering. */
have_color:
                color_entry->sixmap[x] |= 1 << i;
                color_entry->render = 1;
            }
        }

        /* Render the sixmap for each color. */
        for (j = 0; j < 1024; j++) {
            /* Don't render colors not marked for rendering. */
            color_entry = &sixel_colors[j];
            if (!color_entry->render) {
                /* Stop if the palette has ended. */
                if (color_entry->rgb & 0x80000000)
                    break;
                else
                    continue;
            }

            /* Set color register if it wasn't already set. */
            if (!color_entry->set) {
                color_entry->set = 1;
                fprintf(CLI_RENDER_OUTPUT, "#%d;2;%d;%d;%d",
                        j,
                        color_entry->r_g_b_f[0],
                        color_entry->r_g_b_f[1],
                        color_entry->r_g_b_f[2]);
            }

            /* Activate color register. */
            fprintf(CLI_RENDER_OUTPUT, "#%d", j);

            /* Output sixels with RLE compression. */
            i       = 0;
            prev_ch = -1;
            for (x = 0; x < sx; x++) {
                ch = color_entry->sixmap[x];
                if (ch != prev_ch) {
                    if (i < 4) {
                        for (; i; i--)
                            fputc(63 + prev_ch, CLI_RENDER_OUTPUT);
                    } else {
                        fprintf(CLI_RENDER_OUTPUT, "!%d%c", i, 63 + prev_ch);
                        i = 0;
                    }
                    prev_ch = ch;
                }
                i++;
            }
            if (i < 4) {
                for (; i; i--)
                    fputc(63 + prev_ch, CLI_RENDER_OUTPUT);
            } else {
                fprintf(CLI_RENDER_OUTPUT, "!%d%c", i, 63 + prev_ch);
            }

            /* Rewind this sixel row. */
            fputc('$', CLI_RENDER_OUTPUT);

            /* Reset color state for the next sixel row. */
            color_entry->render = 0;
            memset(color_entry->sixmap, 0, sizeof(color_entry->sixmap));
        }

        /* Move on to the next sixel row. */
        fputc('-', CLI_RENDER_OUTPUT);
    }

    /* Finish sixel output. */
    fputs("\033\\", CLI_RENDER_OUTPUT);
}

void
cli_render_process_sixel_png(char *fn)
{
    /* This requires libsixel. */
    if (!sixel_encoder_encode)
        return;

    /* Check if stdout is not redirected, because libsixel's weird API
       does not provide a way to change the encoder object's output fd... */
    if (!isatty(fileno(stdout)))
        return;

    /* Render image and flush stdout. */
    sixel_encoder_encode(NULL, (const char *) fn);
    fflush(stdout);
}

static void
cli_render_process(void *priv)
{
    char    buf[CLI_RENDER_ANSIBUF_SIZE], *p;
    uint8_t has_changed,
        chr, attr, attr77,
        sgr_started, sgr_blackout,
        sgr_ul, sgr_int, sgr_reverse,
        sgr_blink, sgr_bg, sgr_fg,
        prev_sgr_ul, prev_sgr_int, prev_sgr_reverse,
        prev_sgr_blink, prev_sgr_bg, prev_sgr_fg,
        in_decsg, new_cx, new_cy;
    uint16_t           chr_attr;
    int                i, w, x;
    cli_render_line_t *line;
    png_structp        png_ptr;
    png_infop          info_ptr;

    while (1) {
        thread_set_event(render_data.render_complete);

        thread_wait_event(render_data.wake_render_thread, -1);
        thread_reset_event(render_data.wake_render_thread);

        /* Output any requested title change. */
        if (render_data.title[0]) {
            p = buf;
            p += sprintf(p, "\033]0;");
            for (i = 0; render_data.title[i]; i++) /* really hacky wchar->ASCII conversion */
                *p++ = ((render_data.title[i] >= 0x20) && (render_data.title[i] <= 0x7e)) ? render_data.title[i] : ' ';
            *p++ = '\a';
            *p   = '\0';
            fputs(buf, CLI_RENDER_OUTPUT);
            render_data.title[0] = '\0';
        }

        /* Don't render anything if rendering is blocked. */
        if (render_data.block)
            continue;

        /* Output any requested side-band messages. */
        if (render_data.has_sideband) {
            for (i = 0; i < RENDER_SIDEBAND_MAX; i++) {
                if (!render_data.sideband_slots[i][0])
                    continue;
                fputs(render_data.sideband_slots[i], CLI_RENDER_OUTPUT);
                render_data.sideband_slots[i][0] = '\0';
                render_data.has_sideband &= ~(1 << i); /* FIXME: this is not atomic, there could be trouble... */
            }
        }

        /* Trigger invalidation on a mode transition. */
        if (render_data.mode != render_data.prev_mode) {
            if (render_data.prev_mode == CLI_RENDER_BLANK)
                render_data.infobox = NULL; /* invalidate infobox when exiting BLANK */
            else if (render_data.prev_mode == CLI_RENDER_GFX)
                cli_blit = 0; /* stop blitting when exiting GFX */
            render_data.prev_mode      = render_data.mode;
            render_data.invalidate_all = 1;
        }

        /* Invalidate all lines if requested. */
        if (render_data.invalidate_all) {
            render_data.invalidate_all = 0;

            /* Clear screen. */
            strcpy(cli_render_clearbg(buf), "\033[2J\033[3J");
            fputs(buf, CLI_RENDER_OUTPUT);

            /* Invalidate and redraw each line. */
            for (i = 0; i < CLI_RENDER_MAX_LINES; i++) {
                if (lines[i]) {
                    lines[i]->invalidate = 1;
                    cli_render_updateline(lines[i]->buffer, i, 1, cursor_x, cursor_y);
                }
            }
        }

        /* Render according to the current mode. */
        switch (render_data.mode) {
            case CLI_RENDER_BLANK:
                /* Render infobox if required. */
                if (render_data.infobox) {
                    /* Render middle line, while determining the box's width. */
                    p = buf;
                    p += sprintf(p, "\033[30;47m");
                    if (cli_term.can_utf8)
                        p += sprintf(p, "%s", cp437[0xba]);
                    else
                        p += sprintf(p, "\033(0\x78\033(B");
                    w = sprintf(p, render_data.infobox, render_data.infobox_sx, render_data.infobox_sy);
                    p += w;
                    if (cli_term.can_utf8)
                        sprintf(p, "%s", cp437[0xba]);
                    else
                        sprintf(p, "\033(0\x78\033(B");
                    cli_render_updateline(buf, 1, 0, -1, -1);

                    /* Render top line. */
                    if (cli_term.can_utf8) {
                        p = buf + 8;
                        p += sprintf(p, "%s", cp437[0xc9]);
                        for (i = 0; i < w; i++)
                            p += sprintf(p, "%s", cp437[0xcd]);
                        sprintf(p, "%s", cp437[0xbb]);
                    } else {
                        p    = buf + 11;
                        *p++ = '\x6C';
                        for (i = 0; i < w; i++)
                            *p++ = '\x71';
                        sprintf(p, "\x6B\033(B");
                    }
                    cli_render_updateline(buf, 0, 0, -1, -1);

                    /* Render bottom line. */
                    if (cli_term.can_utf8) {
                        p = buf + 8;
                        memcpy(p, cp437[0xc8], 3);
                        p += 3;
                        p += 3 * w;
                        memcpy(p, cp437[0xbc], 3);
                    } else {
                        p    = buf + 11;
                        *p++ = '\x6D';
                        p += w;
                        *p = '\x6A';
                    }
                    cli_render_updateline(buf, 2, 0, -1, -1);

                    i = 3;
                } else {
                    i = 0;
                }

                /* Render blank lines where the infobox is not needed. */
                buf[0] = '\0';
                x      = MIN(cli_term.size_y, CLI_RENDER_MAX_LINES);
                for (; i < x; i++) {
                    if (lines[i])
                        cli_render_updateline(buf, i, 0, -1, -1);
                }
                break;

            case CLI_RENDER_CGA:
            case CLI_RENDER_MDA:
                /* Set initial state. */
                p      = NULL;
                new_cx = render_data.con ? cursor_x : -1;
                new_cy = cursor_y;

                /* Get line structure. */
                line = cli_render_getline(render_data.y);
                if (!line)
                    goto next;

                /* Handle changes in text line count. */
                w = get_actual_size_y() / render_data.rowcount;
                if (w < render_data.prev_rowcount) {
                    /* Reset background color. */
                    cli_render_clearbg(buf);
                    fputs(buf, CLI_RENDER_OUTPUT);

                    /* Blank all lines beyond the new screen limits. */
                    x = MIN(cli_term.size_y, CLI_RENDER_MAX_LINES);
                    x = MIN(render_data.prev_rowcount, x);
                    for (i = w; i <= x; i++) {
                        if (lines[i])
                            lines[i]->invalidate = 0;
                        fprintf(CLI_RENDER_OUTPUT, "\033[%d;1H\033[2K", i);
                    }
                } else if (w > render_data.prev_rowcount) {
                    /* Redraw all lines beyond the previous screen limits. */
                    x = MIN(w, CLI_RENDER_MAX_LINES);
                    for (i = render_data.prev_rowcount - 1; i <= x; i++) {
                        if (lines[i]) {
                            lines[i]->invalidate = 1;
                            cli_render_updateline(lines[i]->buffer, i, 0, new_cx, new_cy);
                        }
                    }
                }
                render_data.prev_rowcount = w;

                /* Determine if this line was invalidated and should be re-rendered. */
                has_changed = 0;
                if (line->invalidate) {
                    line->invalidate = 0;
                    has_changed      = 1;
                }
                if (render_data.do_render != line->do_render) {
                    line->do_render = render_data.do_render;
                    has_changed     = 1;
                }
                if (render_data.do_blink != line->do_blink) {
                    line->do_blink = render_data.do_blink;
                    has_changed    = 1;
                }

                /* Copy framebuffer while determining whether or not
                   it has changed, as well as the cursor position. */
                for (i = x = 0; (i < render_data.xlimit) && (x < cli_term.size_x); i += render_data.xinc, x++) {
                    /* Compare and copy 16-bit character+attribute value. */
                    chr_attr = *((uint16_t *) &render_data.fb[(render_data.fb_base << 1) & render_data.fb_mask]);
                    if (chr_attr != line->framebuffer[x]) {
                        has_changed          = 1;
                        line->framebuffer[x] = chr_attr;
                    }

                    /* If the cursor is on this location, set it as the new cursor position. */
                    if ((render_data.fb_base == render_data.ca) && render_data.con) {
                        new_cx = x;
                        new_cy = render_data.y;
                    }

                    render_data.fb_base += render_data.fb_step;
                }

                /* Don't render if the framebuffer hasn't changed. */
                if (!has_changed)
                    goto next;

                /* Start with a fresh state. */
                p           = buf;
                *p          = '\0';
                sgr_started = prev_sgr_blink = prev_sgr_bg = prev_sgr_fg = prev_sgr_ul = prev_sgr_int = prev_sgr_reverse = 0;
                in_decsg                                                                                                 = 0;
                sgr_blackout                                                                                             = -1;

                /* Render each character. */
                for (i = x = 0; (i < render_data.xlimit) && (x < cli_term.size_x); i += render_data.xinc, x++) {
                    if (render_data.do_render) {
                        chr_attr = line->framebuffer[x];
                        chr      = chr_attr & 0xff;
                        attr     = chr_attr >> 8;
                    } else {
                        chr = attr = 0;
                    }

                    if (render_data.mode == CLI_RENDER_CGA) {
                        /* Set foreground color. */
                        sgr_fg = cga_ansi_palette[attr & 15];
                        if ((x == 0) || (sgr_fg != prev_sgr_fg)) {
                            APPEND_SGR();
                            p += cli_term.setcolor(p, sgr_fg, 0);
                            prev_sgr_fg = sgr_fg;
                        }

                        /* If blinking is enabled, use the top bit for that instead of bright background. */
                        if (render_data.do_blink) {
                            sgr_blink = attr & 0x80;
                            attr &= 0x7f;
                        } else
                            sgr_blink = 0;

                        /* Set background color. */
                        sgr_bg = cga_ansi_palette[attr >> 4];
                        if ((x == 0) || (sgr_bg != prev_sgr_bg)) {
                            APPEND_SGR();
                            p += cli_term.setcolor(p, sgr_bg, 1);
                            prev_sgr_bg = sgr_bg;
                        }

                        /* Set blink. */
                        if ((x == 0) || (sgr_blink != prev_sgr_blink)) {
                            APPEND_SGR();
                            p += sprintf(p, sgr_blink ? ((cli_term.ctl_level & TERM_CTL_RAPIDBLINK) ? "6" : "5") : "25");
                            prev_sgr_blink = sgr_blink;
                        }
                    } else {
                        attr77 = attr & 0x77;
                        if (!attr77) {
                            /* Create a blank space by discarding all attributes then printing a space. */
                            if (!sgr_blackout) {
                                APPEND_SGR();
                                p += sprintf(p, "0");
                                sgr_blackout = 1;
                                prev_sgr_ul = prev_sgr_int = prev_sgr_blink = prev_sgr_reverse = 0;
                            }
                            chr = 0;
                        } else {
                            sgr_blackout = 0;

                            /* Set reverse. */
                            sgr_reverse = (attr77 == 0x70);
                            if (sgr_reverse != prev_sgr_reverse) {
                                APPEND_SGR();
                                p += sprintf(p, sgr_reverse ? "7" : "27");
                                prev_sgr_reverse = sgr_reverse;
                            }

                            /* Set underline. Cannot co-exist with reverse. */
                            sgr_ul = ((attr & 0x07) == 1) && !sgr_reverse;
                            if (sgr_ul != prev_sgr_ul) {
                                APPEND_SGR();
                                p += sprintf(p, sgr_ul ? "4" : "24");
                                prev_sgr_ul = sgr_ul;
                            }

                            /* Set blink, if enabled. */
                            sgr_blink = (attr & 0x80) && render_data.do_blink;
                            if (sgr_blink != prev_sgr_blink) {
                                APPEND_SGR();
                                p += sprintf(p, sgr_blink ? ((cli_term.ctl_level & TERM_CTL_RAPIDBLINK) ? "6" : "5") : "25");
                                prev_sgr_blink = sgr_blink;
                            }

                            /* Set intense. Cannot co-exist with both reverse and blink simultaneously. */
                            sgr_int = (attr & 0x08) && !(sgr_reverse && sgr_blink);
                            if (sgr_int != prev_sgr_int) {
                                APPEND_SGR();
                                p += sprintf(p, sgr_int ? "1" : "22");
                                prev_sgr_int = sgr_int;
                            }
                        }
                    }

                    /* Finish any SGRs we may have started. */
                    if (sgr_started) {
                        sgr_started = 0;
                        p += sprintf(p, "m");
                    }

                    /* Add character. */
                    if (cli_term.can_utf8) {
                        p += sprintf(p, "%s", cp437[chr]);
                    } else if (cp437_fallback[chr] & 0x80) {
                        /* Enter DEC Special Graphics. */
                        if (!in_decsg) {
                            p += sprintf(p, "\033(0");
                            in_decsg = 1;
                        }
                        *p++ = cp437_fallback[chr] & 0x7f;
                    } else {
                        /* Exit DEC Special Graphics. */
                        if (in_decsg) {
                            p += sprintf(p, "\033(B");
                            in_decsg = 0;
                        }
                        *p++ = cp437_fallback[chr];
                    }
                }

                /* Exit DEC Special Graphics. */
                if (in_decsg)
                    p += sprintf(p, "\033(B");

                /* Output rendered line. */
                *p = '\0';
                p  = buf;
next:
                cli_render_updateline(p, render_data.y, 1, new_cx, new_cy);

                /* Don't re-render if the next thread call is
                   just for text output with no rendering tasks. */
                render_data.y = CLI_RENDER_MAX_LINES + 1;
                break;

            case CLI_RENDER_GFX:
                /* Make sure we have a framebuffer, and it's time to render. */
                if (!render_data.blit_fb || !render_data.blit_lines || !(time(NULL) - gfx_last)) /* render at ~1 fps minus rendering time */
                    break;

                /* Reset formatting and move cursor to top left corner. */
                sprintf(cli_render_clearbg(buf), "\033[1;1H");
                fputs(buf, CLI_RENDER_OUTPUT);

                if (cli_term.gfx_level & (TERM_GFX_PNG | TERM_GFX_PNG_KITTY)) {
                    /* Initialize PNG data. */
                    png_ptr   = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
                    png_size  = 0;
                    png_first = png_current = malloc(sizeof(cli_render_png_t));
                    png_first->size         = 0;
                    png_first->next         = NULL;

                    /* Set write function. */
                    png_set_write_fn(png_ptr, NULL, cli_render_process_pngwrite, cli_render_process_pngflush);

                    /* Output PNG to data buffer. */
                    info_ptr = png_create_info_struct(png_ptr);
                    png_set_IHDR(png_ptr, info_ptr, render_data.blit_sx, render_data.blit_sy,
                                 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
                                 PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
                    png_write_info(png_ptr, info_ptr);
                    png_write_image(png_ptr, render_data.blit_lines);
                    png_write_end(png_ptr, NULL);

                    /* Output PNG from data buffer according to the terminal's capabilities. */
                    if (cli_term.gfx_level & TERM_GFX_PNG) {
                        /* Output header. */
                        fprintf(CLI_RENDER_OUTPUT, "\033]1337;File=name=aS5wbmc=;size=%d:", png_size); /* i.png */

                        /* Output image. */
                        while (png_first) {
                            /* Output chunk data as base64. */
                            cli_render_process_base64(png_first->buffer, png_first->size);

                            /* Move on to the next chunk. */
                            png_current = png_first;
                            png_first   = png_first->next;
                            free(png_current);
                        }

                        /* Output terminator. */
                        fputc('\a', CLI_RENDER_OUTPUT);
                    } else if (cli_term.gfx_level & TERM_GFX_PNG_KITTY) {
                        /* Output image in chunks of up to 4096
                           base64-encoded bytes (3072 real bytes). */
                        i = 1;
                        while (png_first) {
                            /* Output header. */
                            fputs("\033_G", CLI_RENDER_OUTPUT);
                            if (i) {
                                i = 0;
                                fputs("a=T,f=100,q=1,", CLI_RENDER_OUTPUT);
                            }
                            fprintf(CLI_RENDER_OUTPUT, "m=%d;", !!png_first->next);

                            /* Output chunk data as base64. */
                            cli_render_process_base64(png_first->buffer, png_first->size);

                            /* Output terminator. */
                            fputs("\033\\", CLI_RENDER_OUTPUT);

                            /* Move on to the next chunk. */
                            png_current = png_first;
                            png_first   = png_first->next;
                            free(png_current);
                        }
                    }
                } else if (cli_term.gfx_level & TERM_GFX_SIXEL) {
                    cli_render_process_sixel(render_data.blit_fb, render_data.blit_sx, render_data.blit_sy);
                }

                /* Flush output. */
                fflush(CLI_RENDER_OUTPUT);

                /* Update render time to keep track of framerate. */
                gfx_last = time(NULL);
                break;
        }
    }
}

void
cli_render_init()
{
    /* Try loading libsixel. */
    int i;
    libsixel_handle = dynld_module(PATH_LIBSIXEL_DLL, libsixel_imports);
    if (!libsixel_handle)
        libsixel_handle = dynld_module(PATH_LIBSIXEL_DLL_ALT, libsixel_imports);
    if (libsixel_handle) {
        /* Create output object. */
        if (sixel_output_new && sixel_encode) {
            sixel_output_new(&libsixel_output, cli_render_process_sixelwrite, CLI_RENDER_OUTPUT, NULL);
        } else {
            sixel_dither_get = NULL; /* disable libsixel if we're somehow missing functions */
            goto no_libsixel;
        }

        cli_render_log("CLI Render: libsixel loaded successfully\n");
    } else {
no_libsixel:
        cli_render_log("CLI Render: libsixel not loaded\n");
    }

    /* Perform initial terminal setup. */
    cli_render_monitorexit();

    /* Load RGB color values for the 256-color palette.
       Algorithm based on Linux's vt.c */
    uint32_t palette_color;
    for (i = 0; i < 256; i++) {
        if (i < 16) { /* 16-color ANSI */
            palette_color = (i & 8) ? 0x555555 : 0x000000;
            if (i & 1)
                palette_color |= 0xaa0000;
            if (i & 2)
                palette_color |= 0x00aa00;
            if (i & 4)
                palette_color |= 0x0000aa;
        } else if (i < 232) { /* color cube */
            palette_color = (uint8_t) ((i - 16) / 36 * 85 / 2) << 16;
            palette_color |= (uint8_t) ((i - 16) / 6 % 6 * 85 / 2) << 8;
            palette_color |= (uint8_t) ((i - 16) % 6 * 85 / 2);
        } else { /* grayscale ramp */
            palette_color = (uint8_t) (i * 10 - 2312);
            palette_color |= palette_color << 8;
            palette_color |= palette_color << 8;
        }
        colors_8bit[i] = palette_color;
    }

    /* Load standard CGA palette. */
    for (i = 0; i < 16; i++) {
        palette_color = (i & 8) ? 0x555555 : 0x000000;
        if (i & 1)
            palette_color |= 0x0000aa;
        if (i & 2)
            palette_color |= (i == 6) ? 0x005500 : 0x00aa00; /* account for brown */
        if (i & 4)
            palette_color |= 0xaa0000;
        palette_24bit[i] = ~palette_color; /* force processing */
        cli_render_setpal(i, palette_color);
    }

    /* Start rendering thread. */
    render_data.wake_render_thread = thread_create_event();
    render_data.render_complete    = thread_create_event();
    render_data.thread             = thread_create(cli_render_process, NULL);
}

void
cli_render_close()
{
    /* Wait for the rendering thread to finish. */
    thread_wait_event(render_data.render_complete, -1);
    thread_set_event(render_data.render_complete); /* to avoid deadlocks just in case */

    /* Clean up. There shouldn't be any race conditions with
       the blit thread, as this is called after video_close. */
    if (render_data.blit_lines)
        free(render_data.blit_lines);
    if (render_data.blit_fb)
        free(render_data.blit_fb);
    if (sixel_colors)
        free(sixel_colors);
    if (libsixel_handle) {
        if (libsixel_output)
            sixel_output_destroy(libsixel_output);
        dynld_close(libsixel_handle);
    }

    /* Reset terminal and switch back to xterm's Main Screen Buffer. */
    fputs("\033[0m\033[999;1H\033[?25h\033[?1049l", CLI_RENDER_OUTPUT);
}