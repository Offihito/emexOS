/*
 * drivers/sound/layer.c
 *
 * Universal audio driver layer — mirrors drivers/net/layers.c
 *
 * Add new hardware drivers here.  Everything above this file
 * (kernel/devices/audio/audio0.c, userland) stays unchanged.
 */

#include "layer.h"
#include "ac97/ac97.h"

/* ------------------------------------------------------------------ */
/* Driver table                                                        */
/* ------------------------------------------------------------------ */

static audio_driver_t ac97_drv = {
    .init       = ac97_init,
    .write      = ac97_write,
    .set_rate   = ac97_set_rate,
    .set_volume = ac97_set_volume,
    .present    = ac97_present,
    .beep       = ac97_beep,
};

/*
 * To add another driver (e.g. Intel HDA):
 *
 *   #include "hda/hda.h"
 *
 *   static audio_driver_t hda_drv = {
 *       .init       = hda_init,
 *       .write      = hda_write,
 *       .set_rate   = hda_set_rate,
 *       .set_volume = hda_set_volume,
 *       .present    = hda_present,
 *   };
 *
 * Then add it to the probe list in audiodrv_init().
 */

/* ------------------------------------------------------------------ */
/* Active driver pointer — NULL until audiodrv_init() succeeds        */
/* ------------------------------------------------------------------ */

static audio_driver_t *active = NULL;

/* ------------------------------------------------------------------ */
/* Public API implementation                                           */
/* ------------------------------------------------------------------ */

/*
 * Probe drivers in priority order and latch the first that works.
 */
int audiodrv_init(void)
{
    /* AC97 — widely supported in QEMU and real hardware */
    if (ac97_drv.init() == 0) {
        active = &ac97_drv;
        return 0;
    }

    /* add further fallback drivers here, e.g.:
     *   if (hda_drv.init() == 0) { active = &hda_drv; return 0; }
     */

    /* no supported audio hardware found */
    return -1;
}

int audiodrv_write(const void *buf, size_t len)
{
    if (!active)
        return -1;

    return active->write(buf, len);
}

int audiodrv_set_rate(u32 hz)
{
    if (!active)
        return -1;

    return active->set_rate(hz);
}

void audiodrv_set_volume(u8 master_atten, u8 pcm_atten)
{
    if (!active)
        return;

    active->set_volume(master_atten, pcm_atten);
}

int audiodrv_available(void)
{
    return active != NULL;
}

int audiodrv_beep(u32 freq_hz, u32 duration_ms)
{
    if (!active || !active->beep)
        return -1;

    return active->beep(freq_hz, duration_ms);
}
