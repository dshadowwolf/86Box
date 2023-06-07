/*
 * 86Box    A hypervisor and IBM PC system emulator that specializes in
 *          running old operating systems and software designed for IBM
 *          PC systems and compatibles from 1981 through fairly recent
 *          system designs based on the PCI bus.
 *
 *          This file is part of the 86Box distribution.
 *
 *          Main include file for the application.
 *
 *
 *
 * Authors: Miran Grca, <mgrca8@gmail.com>
 *          Fred N. van Kempen, <decwiz@yahoo.com>
 *
 *          Copyright 2016-2020 Miran Grca.
 *          Copyright 2017-2020 Fred N. van Kempen.
 *          Copyright 2021 Laci bá'
 */
#ifndef EMU_86BOX_H
#define EMU_86BOX_H

/* Configuration values. */
#define SERIAL_MAX   4
#define PARALLEL_MAX 4
#define SCREEN_RES_X 640
#define SCREEN_RES_Y 480

/* Filename and pathname info. */
#define CONFIG_FILE     "86box.cfg"
#define NVR_PATH        "nvr"
#define SCREENSHOT_PATH "screenshots"

/* Recently used images */
#define MAX_PREV_IMAGES    4
#define MAX_IMAGE_PATH_LEN 256

/* Default language 0xFFFF = from system, 0x409 = en-US */
#define DEFAULT_LANGUAGE 0x0409

#define POSTCARDS_NUM 4
#define POSTCARD_MASK (POSTCARDS_NUM - 1)

#ifdef MIN
#    undef MIN
#endif
#ifdef MAX
#    undef MAX
#endif
#ifdef ABS
#    undef ABS
#endif
#ifdef ABSD
#    undef ABSD
#endif

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define ABS(x)    ((x) > 0 ? (x) : -(x))
#define ABSD(x)   ((x) > 0.0 ? (x) : -(x))
#define BCD8(x)   ((((x) / 10) << 4) | ((x) % 10))
#define BCD16(x)  ((((x) / 1000) << 12) | (((x) / 100) << 8) | BCD8(x))
#define BCD32(x)  ((((x) / 10000000) << 28) | (((x) / 1000000) << 24) | (((x) / 100000) << 20) | (((x) / 10000) << 16) | BCD16(x))

#if defined(__GNUC__) || defined(__clang__)
#    define UNLIKELY(x) __builtin_expect((x), 0)
#    define LIKELY(x)   __builtin_expect((x), 1)
#else
#    define UNLIKELY(x) (x)
#    define LIKELY(x)   (x)
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Global variables. */
extern uint32_t lang_sys; /* (-) system language code */

extern uint32_t dump_on_exit;        /* (O) dump regs on exit*/
extern uint32_t do_dump_config;      /* (O) dump cfg after load */
extern uint32_t start_in_fullscreen; /* (O) start in fullscreen */
#ifdef _WIN32
extern uint32_t force_debug; /* (O) force debug output */
#endif
#ifdef USE_WX
extern uint32_t video_fps; /* (O) render speed in fps */
#endif
extern uint32_t settings_only;     /* (O) show only the settings dialog */
extern uint32_t confirm_exit_cmdl; /* (O) do not ask for confirmation on quit if set to 0 */
#ifdef _WIN32
extern uint64_t unique_id;
extern uint64_t source_hwnd;
#endif
extern char rom_path[1024]; /* (O) full path to ROMs */
extern char log_path[1024]; /* (O) full path of logfile */
extern char vm_name[1024];  /* (O) display name of the VM */
#ifdef USE_INSTRUMENT
extern uint8_t  instru_enabled;
extern uint64_t instru_run_ms;
#endif

#define window_x monitor_settings[0].mon_window_x
#define window_y monitor_settings[0].mon_window_y
#define window_w monitor_settings[0].mon_window_w
#define window_h monitor_settings[0].mon_window_h
extern uint32_t window_remember;
extern uint32_t vid_resize;                 /* (C) allow resizing */
extern uint32_t invert_display;             /* (C) invert the display */
extern uint32_t suppress_overscan;          /* (C) suppress overscans */
extern uint32_t lang_id;                    /* (C) language code identifier */
extern char     icon_set[256];              /* (C) iconset identifier */
extern uint32_t scale;                      /* (C) screen scale factor */
extern uint32_t dpi_scale;                  /* (C) DPI scaling of the emulated screen */
extern uint32_t vid_api;                    /* (C) video renderer */
extern uint32_t vid_cga_contrast;           /* (C) video */
extern uint32_t video_fullscreen;           /* (C) video */
extern uint32_t video_fullscreen_first;     /* (C) video */
extern uint32_t video_fullscreen_scale;     /* (C) video */
extern uint32_t enable_overscan;            /* (C) video */
extern uint32_t force_43;                   /* (C) video */
extern int32_t  video_filter_method;        /* (C) video */
extern uint32_t video_vsync;                /* (C) video */
extern uint32_t video_framerate;            /* (C) video */
extern uint32_t gfxcard[2];                 /* (C) graphics/video card */
extern char     video_shader[512];          /* (C) video */
extern uint32_t bugger_enabled;             /* (C) enable ISAbugger */
extern uint32_t postcard_enabled;           /* (C) enable POST card */
extern uint32_t isamem_type[];              /* (C) enable ISA mem cards */
extern uint32_t isartc_type;                /* (C) enable ISA RTC card */
extern uint32_t sound_is_float;             /* (C) sound uses FP values */
extern uint32_t voodoo_enabled;             /* (C) video option */
extern uint32_t ibm8514_standalone_enabled; /* (C) video option */
extern uint32_t xga_standalone_enabled;     /* (C) video option */
extern uint32_t mem_size;                   /* (C) memory size (Installed on system board) */
extern uint32_t isa_mem_size;               /* (C) memory size (ISA Memory Cards) */
extern uint32_t cpu;                        /* (C) cpu type */
extern uint32_t cpu_use_dynarec;            /* (C) cpu uses/needs Dyna */
extern uint32_t fpu_type;                   /* (C) fpu type */
extern uint32_t fpu_softfloat;              /* (C) fpu uses softfloat */
extern uint32_t time_sync;                  /* (C) enable time sync */
extern uint32_t hdd_format_type;            /* (C) hard disk file format */
extern uint32_t confirm_reset;              /* (C) enable reset confirmation */
extern uint32_t confirm_exit;               /* (C) enable exit confirmation */
extern uint32_t confirm_save;               /* (C) enable save confirmation */
extern uint32_t enable_discord;             /* (C) enable Discord integration */

extern uint32_t fixed_size_x;
extern uint32_t fixed_size_y;
extern double   mouse_sensitivity;          /* (C) Mouse sensitivity scale */
#ifdef _Atomic
extern _Atomic double mouse_x_error;        /* Mouse error accumulator - Y */
extern _Atomic double mouse_y_error;        /* Mouse error accumulator - Y */
#endif
extern uint32_t pit_mode;                   /* (C) force setting PIT mode */
extern uint32_t fm_driver;                  /* (C) select FM sound driver */

extern char      exe_path[2048];    /* path (dir) of executable */
extern char      usr_path[1024];    /* path (dir) of user data */
extern char      cfg_path[1024];    /* full path of config file */
extern uint32_t  open_dir_usr_path; /* default file open dialog directory of usr_path */
#ifndef USE_NEW_DYNAREC
extern FILE     *stdlog; /* file to log output to */
#endif
extern uint32_t  config_changed; /* config has changed */

/* Function prototypes. */
#ifdef HAVE_STDARG_H
extern void pclog_ex(const char *fmt, va_list);
extern void fatal_ex(const char *fmt, va_list);
#endif
extern void pclog_toggle_suppr(void);
extern void pclog(const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));
extern void fatal(const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));
extern void set_screen_size(uint32_t x, uint32_t y);
extern void set_screen_size_monitor(uint32_t x, uint32_t y, uint32_t monitor_index);
extern void reset_screen_size(void);
extern void reset_screen_size_monitor(uint32_t monitor_index);
extern void set_screen_size_natural(void);
extern void update_mouse_msg(void);
#if 0
extern void pc_reload(wchar_t *fn);
#endif
extern uint32_t pc_init_modules(void);
extern uint32_t pc_init(int32_t argc, char *argv[]);
extern void     pc_close(void *threadid);
extern void     pc_reset_hard_close(void);
extern void     pc_reset_hard_init(void);
extern void     pc_reset_hard(void);
extern void     pc_full_speed(void);
extern void     pc_speed_changed(void);
extern void     pc_send_cad(void);
extern void     pc_send_cae(void);
extern void     pc_send_cab(void);
extern void     pc_run(void);
extern void     pc_start(void);
extern void     pc_onesec(void);

extern uint16_t get_last_addr(void);

/* This is for external subtraction of cycles;
   should be in cpu.c but I put it here to avoid
   having to include cpu.h everywhere. */
extern void sub_cycles(int c);
extern void resub_cycles(int old_cycles);

extern double   isa_timing;
extern uint32_t io_delay;
extern uint32_t framecountx;

extern volatile uint32_t cpu_thread_run;
extern          uint8_t  postcard_codes[POSTCARDS_NUM];

#ifdef __cplusplus
}
#endif

#endif /*EMU_86BOX_H*/
