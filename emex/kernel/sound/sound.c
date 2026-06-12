/*
 * kernel/sound/sound.c
 *
 * Kernel-side audio helpers — mirrors kernel/net/net.c
 *
 * Future home for a kernel audio stack (mixing, resampling, etc.).
 * For now it just re-exports the layer availability check so that
 * kernel subsystems only include <kernel/sound/sound.h>.
 */

#include "sound.h"

int sound_available(void)
{
    return audiodrv_available();
}
