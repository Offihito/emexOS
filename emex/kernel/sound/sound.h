#pragma once

/*
 * kernel/sound/sound.h
 *
 * Kernel-side audio availability check — mirrors kernel/net/net.h
 *
 * Include this wherever kernel code needs to know whether audio
 * output is possible, without pulling in any hardware-specific headers.
 */

#include <drivers/sound/layer.h>

/*
 * Returns 1 if an audio driver is active, 0 otherwise.
 * Thin wrapper so callers never depend on drivers/sound/layer.h directly.
 */
int sound_available(void); /* returns audiodrv_available() */
