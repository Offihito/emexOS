/*
 * kernel/devices/audio/audio0.c
 *
 * /dev/audio0 device module — mirrors kernel/devices/net/eth0.c
 *
 * This module only calls audiodrv_* from drivers/sound/layer.h.
 * It never includes ac97.h or any other concrete driver header.
 * Replacing AC97 with HDA only requires changing drivers/sound/layer.c.
 */

#include "audio0.h"

#include <kernel/module/module.h>
#include <kernel/communication/serial.h>
#include <drivers/drivers.h>             /* pulls in devices.h → AUDIO0NAME/PATH/UNIVERSAL */
#include <kernel/sound/sound.h>          /* sound_available()   */
#include <drivers/sound/layer.h>         /* audiodrv_*          */
#include <kernel/kernel_processes/bootscreen/log.h>

/* ------------------------------------------------------------------ */
/* Module callbacks                                                    */
/* ------------------------------------------------------------------ */

/*
 * Called by module_register() → devfs_register_device().
 * Runs the layer init which probes hardware in priority order.
 */
static int audio0_init(void)
{
    log("[AUDIO0]", "init /dev/audio0\n", d);

    int ret = audiodrv_init();
    if (ret != 0) {
        log("[AUDIO0]", "no audio hardware found\n", warning);
        return -1;
    }

    log("[AUDIO0]", "audio layer ready\n", success);
    return 0;
}

static void audio0_fini(void) { /* nothing to tear down for now */ }

/*
 * open() — called when userland opens /dev/audio0.
 * Returns a non-NULL dummy handle if audio is available.
 */
static void *audio0_open(const char *path)
{
    (void)path;

    if (!sound_available())
        return NULL; /* no hardware — deny open */

    return (void *)1; /* any non-NULL handle works */
}

/*
 * read() — not meaningful for a write-only PCM sink.
 * Reserved for future capture / status reads.
 */
static int audio0_read(void *handle, void *buf, size_t count, u64 offset)
{
    (void)handle;
    (void)buf;
    (void)count;
    (void)offset;
    return 0;
}

/*
 * write() — userland writes raw PCM here.
 * Expected format: signed 16-bit stereo, little-endian, interleaved.
 * The layer (and hardware driver beneath it) defines the sample rate;
 * use ioctl / a control interface to change it (future work).
 */
static int audio0_write(void *handle, const void *buf, size_t count, u64 offset)
{
    (void)handle;
    (void)offset;

    if (!buf || !count)
        return -1;

    return audiodrv_write(buf, count);
}

/* ------------------------------------------------------------------ */
/* Module descriptor                                                   */
/* ------------------------------------------------------------------ */

driver_module audio0_module = {
    .name    = AUDIO0NAME,
    .mount   = AUDIO0PATH,
    .version = AUDIO0UNIVERSAL,
    .init    = audio0_init,
    .fini    = audio0_fini,
    .open    = audio0_open,
    .read    = audio0_read,
    .write   = audio0_write,
};
