/*
 * 86Box    A hypervisor and IBM PC system emulator that specializes in
 *          running old operating systems and software designed for IBM
 *          PC systems and compatibles from 1981 through fairly recent
 *          system designs based on the PCI bus.
 *
 *          This file is part of the 86Box distribution.
 *
 *          Definitions for the generic game port handlers.
 *
 *
 *
 * Authors: Miran Grca, <mgrca8@gmail.com>
 *          Sarah Walker, <https://pcem-emulator.co.uk/>
 *          RichardG, <richardg867@gmail.com>
 *
 *          Copyright 2016-2018 Miran Grca.
 *          Copyright 2008-2018 Sarah Walker.
 *          Copyright 2021 RichardG.
 */
#ifndef EMU_GAMEPORT_H
#define EMU_GAMEPORT_H

#define MAX_PLAT_JOYSTICKS  8
#define MAX_JOYSTICKS       4

#define JS_TYPE_NONE               0
#define JS_TYPE_2AXIS_4BUTTON      1
#define JS_TYPE_2AXIS_6BUTTON      2
#define JS_TYPE_2AXIS_8BUTTON      3
#define JS_TYPE_4AXIS_4BUTTON      4
#define JS_TYPE_CH_FLIGHTSTICK_PRO 5
#define JS_TYPE_SIDEWINDER_PAD     6
#define JS_TYPE_THRUSTMASTER_FCS   7


#define POV_X               0x80000000
#define POV_Y               0x40000000
#define SLIDER              0x20000000

#define AXIS_NOT_PRESENT    -99999

#define JOYSTICK_PRESENT(n) (joystick_state[n].plat_joystick_nr != 0)

#define GAMEPORT_SIO        0x1000000

typedef struct plat_joystick_t {
    char name[260];

    int a[8];
    int b[32];
    int p[4];
    int s[2];

    struct {
        char name[260];
        int  id;
    } axis[8];

    struct {
        char name[260];
        int  id;
    } button[32];

    struct {
        char name[260];
        int  id;
    } pov[4];

    struct {
        char name[260];
        int  id;
    } slider[2];

    int nr_axes;
    int nr_buttons;
    int nr_povs;
    int nr_sliders;
} plat_joystick_t;

typedef struct joystick_t {
    int axis[8];
    int button[32];
    int pov[4];

    int plat_joystick_nr;
    int axis_mapping[8];
    int button_mapping[32];
    int pov_mapping[4][2];
} joystick_t;

typedef struct joystick_if_t {
    const char *name;
    const char *internal_name;

    void   *(*init)(void);
    void    (*close)(void *priv);
    uint8_t (*read)(void *priv);
    void    (*write)(void *priv);
    int     (*read_axis)(void *priv, int axis);
    void    (*a0_over)(void *priv);

    int         axis_count;
    int         button_count;
    int         pov_count;
    int         max_joysticks;
    const char *axis_names[8];
    const char *button_names[32];
    const char *pov_names[4];
} joystick_if_t;

#ifdef __cplusplus
extern "C" {
#endif

#ifdef EMU_DEVICE_H
extern const device_t gameport_device;
extern const device_t gameport_201_device;
extern const device_t gameport_203_device;
extern const device_t gameport_205_device;
extern const device_t gameport_207_device;
extern const device_t gameport_208_device;
extern const device_t gameport_209_device;
extern const device_t gameport_20b_device;
extern const device_t gameport_20d_device;
extern const device_t gameport_20f_device;
extern const device_t gameport_tm_acm_device;
extern const device_t gameport_pnp_device;
extern const device_t gameport_pnp_6io_device;
extern const device_t gameport_sio_device;
extern const device_t gameport_sio_1io_device;

extern const device_t *standalone_gameport_type;
#endif
extern int             gameport_instance_id;
extern plat_joystick_t plat_joystick_state[MAX_PLAT_JOYSTICKS];
extern joystick_t      joystick_state[MAX_JOYSTICKS];
extern int             joysticks_present;

extern int joystick_type;

extern void joystick_init(void);
extern void joystick_close(void);
extern void joystick_process(void);

extern const char *joystick_get_name(int js);
extern const char *joystick_get_internal_name(int js);
extern int         joystick_get_from_internal_name(const char *s);
extern int         joystick_get_max_joysticks(int js);
extern int         joystick_get_axis_count(int js);
extern int         joystick_get_button_count(int js);
extern int         joystick_get_pov_count(int js);
extern const char *joystick_get_axis_name(int js, int id);
extern const char *joystick_get_button_name(int js, int id);
extern const char *joystick_get_pov_name(int js, int id);

extern void  gameport_update_joystick_type(void);
extern void  gameport_remap(void *priv, uint16_t address);
extern void *gameport_add(const device_t *gameport_type);

#ifdef __cplusplus
}
#endif

#endif /*EMU_GAMEPORT_H*/
