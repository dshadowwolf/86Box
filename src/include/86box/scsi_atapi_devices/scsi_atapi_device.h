#include <inttypes.h>
#include <86box/scsi.h>
#include <86box/scsi_device.h>
#include <86box/cdrom.h>
#include <86box/scsi_cdrom.h>

typedef void(*scsci_basic_callback_t)(scsi_device_t *);
typedef void(*scsi_common_basic_callback_t)(scsi_common_t *);
typedef uint32_t(*scsi_unsigned_callback_t)(scsi_device_t *, int);
typedef uint8_t(*scsi_uint8_callback_long_t)(scsi_device_t *, uint8_t, uint8_t, uint8_t);
typedef uint8_t(*scsi_uint8_callback_long_alt_t)(scsi_device_t *, uint8_t*, uint8_t, uint8_t);
typedef void(*scsi_common_extended_callback_t)(scsi_common_t *, uint8_t*, uint8_t);
typedef void(*scsi_common_command_callback_t)(scsi_common_t *, uint8_t*);

#pragma pack(push, 1)
/*
various command and response packets get defined here so we don't have to manually build them
*/
#pragma pack(pop)



typedef struct scsi_atapi_device_s {
    const mode_sense_pages_t const * const mode_sense_pages;
    const scsci_basic_callback_t *const device_init;
    const scsi_basic_callback_t *const current_mode;
    const scsi_unsigned_callback_t *const get_channel;
    const scsi_unsigned_callback_t *const get_volume;
    const scsi_basic_callback_t *const mode_sense_load;
    const scsi_basic_callback_t *const mode_sense_save;
    const scsi_basic_callback_t *const drive_status_load;
    const scsi_uint8_callback_long_t * const drive_status_read;
    const scsi_uint8_callback_long_alt_t * const drive_status;
    const scsi_uint8_callback_long_t * const mode_sense_read;
    const scsi_uint8_callback_long_alt_t * const mode_sense;
    const scsi_common_basic_callback_t * const reset;
    const scsi_common_extended_callback_t * const request_sense;
    const scsi_common_command_callback_t * const command_handler;
} scsi_atapi_device_t;
