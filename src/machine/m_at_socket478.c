/*
 * 86Box    A hypervisor and IBM PC system emulator that specializes in
 *          running old operating systems and software designed for IBM
 *          PC systems and compatibles from 1981 through fairly recent
 *          system designs based on the PCI bus.
 *
 *          This file is part of the 86Box distribution.
 *
 *          Implementation of Socket 478(mPGA478) machines.
 *
 *
 *
 * Authors: Jasmine Iwanek, <jriwanek@gmail.com>
 *          rueni97, <aesthcape@gmail.com>
 *
 *          Copyright 2023 Jasmine Iwanek.
 *          Copyright 2023 rueni97.
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
/*
 * ASRock P4i45D+
 *
 * North Bridge: Intel 815EP
 * Super I/O: Winbond W83627HF
 * BIOS: AMIBIOS 040201
 * Notes: None
 */
int
machine_at_p4i45d_init(const machine_t *model)
{
    int ret;

    ret = bios_load_linear("roms/machines/p4i45d/845D_P150.bin",
                           0x000c0000, 262144, 0);

    if (bios_only || !ret)
        return ret;

    machine_at_common_init_ex(model, 2);

    pci_init(PCI_CONFIG_TYPE_1);
    pci_register_bus_slot(0, 0x00, PCI_CARD_NORTHBRIDGE, 0, 0, 0, 0);
    pci_register_bus_slot(0, 0x01, PCI_CARD_AGPBRIDGE,   1, 2, 0, 0);
    pci_register_bus_slot(0, 0x1e, PCI_CARD_BRIDGE,      0, 0, 0, 0);
    pci_register_bus_slot(0, 0x1f, PCI_CARD_SOUTHBRIDGE, 1, 2, 8, 4);
    pci_register_bus_slot(1, 0x00, PCI_CARD_AGP,         1, 2, 0, 0);
    pci_register_bus_slot(2, 0x0a, PCI_CARD_NORMAL,      7, 8, 5, 6);
    pci_register_bus_slot(2, 0x0b, PCI_CARD_NORMAL,      8, 5, 6, 7);
    pci_register_bus_slot(2, 0x0c, PCI_CARD_NORMAL,      5, 6, 7, 8);
    pci_register_bus_slot(2, 0x0d, PCI_CARD_NORMAL,      6, 7, 8, 5);
    pci_register_bus_slot(2, 0x0e, PCI_CARD_NORMAL,      3, 4, 1, 2);

    device_add(&intel_845_device);             /* Intel 845 MCH */
    device_add(&intel_ich2_device);            /* Intel ICH2 */
    device_add(&w83627hf_device);              /* Winbond W83627HF */
    device_add(&winbond_flash_w29c020_device); /* Winbond 2Mbit Firmware Hub */
    ics9xxx_get(ICS9150_08);                   /* ICS Clock Chip */
#if 0
    spd_register(SPD_TYPE_DDR, 2, 1024);       /* SPD */
#endif

    return ret;
}
