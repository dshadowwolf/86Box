#include <86box/scsi.h>
#include <86box/scsi_device.h>
#include <86box/cdrom.h>
#include <86box/scsi_cdrom.h>

static const mode_sense_pages_t scsi_cdrom_mode_sense_pages_default = {
    {{ 0, 0 },
     { GPMODE_R_W_ERROR_PAGE, 6, 0, 5, 0, 0, 0, 0 },
     { 0, 0 },
     { 0, 0 },
     { 0, 0 },
     { 0, 0 },
     { 0, 0 },
     { 0, 0 },
     { 0, 0 },
     { 0, 0 },
     { 0, 0 },
     { 0, 0 },
     { 0, 0 },
     { GPMODE_CDROM_PAGE, 6, 0, 1, 0, 60, 0, 75 },
     { GPMODE_CDROM_AUDIO_PAGE | 0x80, 0xE, 4, 0, 0, 0, 0, 75, 1, 255, 2, 255, 0, 0, 0, 0 },
     { 0x0F, 0x14, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
     { 0, 0 },
     { 0, 0 },
     { 0, 0 },
     { 0, 0 },
     { 0, 0 },
     { 0, 0 },
     { 0, 0 },
     { 0, 0 },
     { 0, 0 },
     { 0, 0 },
     { 0, 0 },
     { 0, 0 },
     { 0, 0 },
     { 0, 0 },
     { 0, 0 },
     { 0, 0 },
     { 0, 0 },
     { 0, 0 },
     { 0, 0 },
     { 0, 0 },
     { 0, 0 },
     { 0, 0 },
     { 0, 0 },
     { 0, 0 },
     { 0, 0 },
     { 0, 0 },
     { GPMODE_CAPABILITIES_PAGE, 0x12, 0, 0, 1, 0, 0, 0, 2, 0xC2, 1, 0, 0, 0, 2, 0xC2, 0, 0, 0, 0 }}
};

static const mode_sense_pages_t scsi_cdrom_mode_sense_pages_default_scsi = {
    {{ GPMODE_UNIT_ATN_PAGE, 6, 0, 0, 0, 0, 0, 0 }, /*Guess-work*/
     { GPMODE_R_W_ERROR_PAGE, 6, 0, 5, 0, 0, 0, 0 },
     { GPMODE_DISCONNECT_PAGE, 0x0e, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
     { GPMODE_FORMAT_DEVICE_PAGE, 0x16, 0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
     { 0, 0 },
     { 0, 0 },
     { 0, 0 },
     { 0, 0 },
     { 0, 0 },
     { 0, 0 },
     { 0, 0 },
     { 0, 0 },
     { 0, 0 },
     { GPMODE_CDROM_PAGE, 6, 0, 1, 0, 60, 0, 75 },
     { GPMODE_CDROM_AUDIO_PAGE | 0x80, 0xE, 5, 4, 0, 128, 0, 75, 1, 255, 2, 255, 0, 0, 0, 0 },
     { 0x0F, 0x14, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
     { 0, 0 },
     { 0, 0 },
     { 0, 0 },
     { 0, 0 },
     { 0, 0 },
     { 0, 0 },
     { 0, 0 },
     { 0, 0 },
     { 0, 0 },
     { 0, 0 },
     { 0, 0 },
     { 0, 0 },
     { 0, 0 },
     { 0, 0 },
     { 0, 0 },
     { 0, 0 },
     { 0, 0 },
     { 0, 0 },
     { 0, 0 },
     { 0, 0 },
     { 0, 0 },
     { 0, 0 },
     { 0, 0 },
     { 0, 0 },
     { 0, 0 },
     { 0, 0 },
     { GPMODE_CAPABILITIES_PAGE, 0x12, 0, 0, 1, 0, 0, 0, 2, 0xC2, 1, 0, 0, 0, 2, 0xC2, 0, 0, 0, 0 }}
};
