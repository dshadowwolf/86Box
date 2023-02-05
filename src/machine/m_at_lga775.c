/*
 * 86Box    A hypervisor and IBM PC system emulator that specializes in
 *          running old operating systems and software designed for IBM
 *          PC systems and compatibles from 1981 through fairly recent
 *          system designs based on the PCI bus.
 *
 *          This file is part of the 86Box distribution.
 *
 *          Implementation of LGA 775(Socket T) machines.
 *
 *
 *
 * Authors: Jasmine Iwanek, <jriwanek@gmail.com>
 *
 *          Copyright 2023 Jasmine Iwanek.
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <86box/86box.h>
#include <86box/mem.h>
#include <86box/io.h>
#include <86box/rom.h>
#include <86box/pci.h>
#include <86box/device.h>
#include <86box/chipset.h>
#include <86box/hdc.h>
#include <86box/hdc_ide.h>
#include <86box/keyboard.h>
#include <86box/flash.h>
#include <86box/sio.h>
#include <86box/hwm.h>
#include <86box/spd.h>
#include <86box/video.h>
#include "cpu.h"
#include <86box/machine.h>
#include <86box/clock.h>
#include <86box/sound.h>
#include <86box/snd_ac97.h>
